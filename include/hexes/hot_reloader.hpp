#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

namespace hexes::fs {

// Watches a Lua script file for changes using a background polling thread.
//
// Thread-safety: this class does NOT call into sol::state. When a change is
// detected the internal flag is set atomically. Call poll_and_reset() from
// the main thread (e.g. once per frame) to check and clear the flag, then
// perform the actual Lua reload there.
class HotReloader {
public:
    explicit HotReloader(
        std::filesystem::path       script_path,
        std::chrono::milliseconds   poll_interval = std::chrono::milliseconds{250}
    );

    ~HotReloader();

    HotReloader(const HotReloader&)            = delete;
    HotReloader& operator=(const HotReloader&) = delete;

    HotReloader(HotReloader&&)            = delete;
    HotReloader& operator=(HotReloader&&) = delete;

    // Returns true and atomically resets the flag if a reload is pending.
    // Must be called from the same thread that owns the sol::state.
    [[nodiscard]] bool poll_and_reset() noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
    std::filesystem::path           script_path_;
    std::filesystem::file_time_type last_write_{};
    std::atomic<bool>               running_{true};
    std::atomic<bool>               reload_requested_{false};
    std::thread                     watcher_thread_;
};

} // namespace hexes
