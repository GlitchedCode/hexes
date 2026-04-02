#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

namespace hexes {

// Watches a Lua script file for changes using a background polling thread.
//
// Thread-safety: this class does NOT call into sol::state. When a change is
// detected the internal flag is set atomically. Call poll_and_reset() from
// the main thread (e.g. once per frame) to check and clear the flag, then
// perform the actual Lua reload there.
class LuaHotReloader {
public:
    explicit LuaHotReloader(
        std::filesystem::path       script_path,
        std::chrono::milliseconds   poll_interval = std::chrono::milliseconds{250}
    );

    ~LuaHotReloader();

    LuaHotReloader(const LuaHotReloader&)            = delete;
    LuaHotReloader& operator=(const LuaHotReloader&) = delete;

    LuaHotReloader(LuaHotReloader&&)            = delete;
    LuaHotReloader& operator=(LuaHotReloader&&) = delete;

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
