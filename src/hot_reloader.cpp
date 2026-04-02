#include <hexes/hot_reloader.hpp>

namespace hexes::fs {

HotReloader::HotReloader(
    std::filesystem::path     script_path,
    std::chrono::milliseconds poll_interval
)
    : script_path_(std::move(script_path))
{
    namespace fs = std::filesystem;

    if (fs::exists(script_path_)) {
        last_write_ = fs::last_write_time(script_path_);
    }

    watcher_thread_ = std::thread([this, poll_interval] {
        namespace fs = std::filesystem;

        while (running_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(poll_interval);

            try {
                if (!fs::exists(script_path_)) continue;

                auto current = fs::last_write_time(script_path_);
                if (current != last_write_) {
                    last_write_ = current;
                    reload_requested_.store(true, std::memory_order_release);
                }
            } catch (const fs::filesystem_error&) {
                // Transient error (file being written, renamed, etc.) — skip tick.
            }
        }
    });
}

HotReloader::~HotReloader() {
    running_.store(false, std::memory_order_relaxed);
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
}

bool HotReloader::poll_and_reset() noexcept {
    return reload_requested_.exchange(false, std::memory_order_acq_rel);
}

const std::filesystem::path& HotReloader::path() const noexcept {
    return script_path_;
}

} // namespace hexes
