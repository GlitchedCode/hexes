#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>
#include <unordered_map>

namespace hexes::fs {

class HotReloader;
using ReloadCallback = std::function<void(HotReloader &hr)>;

// Watches a file for changes using a background polling thread.
class HotReloader {
  friend class FileWatcher;
  explicit HotReloader(std::filesystem::path script_path,
                       ReloadCallback reload_callback = {});

public:
  ~HotReloader();

  HotReloader(const HotReloader &) = delete;
  HotReloader &operator=(const HotReloader &) = delete;

  HotReloader(HotReloader &&) = delete;
  HotReloader &operator=(HotReloader &&) = delete;

  // Returns true and atomically resets the flag if a reload is pending.
  // Must be called from the same thread that owns the sol::state.
  [[nodiscard]] bool poll_and_reset() noexcept;

  [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
  std::filesystem::path script_path_;
  std::atomic<bool> reload_requested_{false};

  ReloadCallback reload_callback_;
};

// singleton class that manages its own watcher thread(s) and signals the
// HotReloader instances when the underlying file changes.

class FileWatcher {

public:
  static FileWatcher &instance();
  FileWatcher(const FileWatcher &) = delete;
  FileWatcher &operator=(const FileWatcher &) = delete;

  HotReloader watch_file(const std::filesystem::path &path,
                         ReloadCallback reload_callback = {});
  void initialize_watchers();

private:
  struct WatchEntry {
    std::filesystem::path path;
    std::filesystem::file_time_type last_write{};
    std::vector<std::weak_ptr<HotReloader>> watchers;
    std::atomic<bool> reload_requested_{false};
  };
  FileWatcher();
  ~FileWatcher();

  int thread_count_ = 1;
  std::vector<std::thread> watcher_threads_;
  std::chrono::milliseconds poll_interval = std::chrono::milliseconds{250};
  std::atomic<bool> running_{true};
  std::mutex mutex_;
  std::unordered_map<std::filesystem::path, WatchEntry> watch_list_;
};

} // namespace hexes::fs
