#include "hexes/logger.hpp"
#include <chrono>
#include <hexes/fs/hot_reloader.hpp>

namespace hexes::fs {

// Helper to convert file_time to system time for logging
static std::string format_file_time(std::filesystem::file_time_type ftime) {
  // Convert file_time to system_clock time_point
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ftime - std::filesystem::file_time_type::clock::now() +
      std::chrono::system_clock::now());
  auto time_t_val = std::chrono::system_clock::to_time_t(sctp);

  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                std::localtime(&time_t_val));
  return std::string(buffer);
}

FileWatcher::FileWatcher() {
  watch_list_.reserve(16); // Avoid initial resizes for small watch counts.
                           //
  watcher_threads_.reserve(thread_count_);
}

void FileWatcher::initialize_watchers() {
  running_.store(true, std::memory_order_release);

  for (int i = 0; i < thread_count_; ++i) {
    watcher_threads_.emplace_back([this]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      while (true) {
        const auto running = running_.load(std::memory_order_acquire);
        if (!running) {
          break;
        }
        std::this_thread::sleep_for(poll_interval);

        std::lock_guard lock(mutex_);
        for (auto &[path, entry] : watch_list_) {
          try {
            auto last_write = std::filesystem::last_write_time(path);
            if (last_write != entry.last_write) {
              entry.last_write = last_write;
              entry.reload_requested_.store(true, std::memory_order_release);

              // Notify all watchers of this file.
              for (auto &weak_watcher : entry.watchers) {
                if (auto watcher = weak_watcher.lock()) {
                  Logger::instance().info("File change detected: {}",
                                          path.string());
                  watcher->reload_requested_.store(true,
                                                   std::memory_order_release);
                  if (watcher->reload_callback_) {
                    watcher->reload_callback_(*watcher);
                  }
                }
              }
            }
          } catch (const std::filesystem::filesystem_error &) {
            // File doesn't exist or is otherwise inaccessible — ignore until it
            // reappears.
          }
        }
      }
    });
  }

  for (auto &thread : watcher_threads_) {
    if (thread.joinable()) {
      thread.detach();
    }
  }
}

FileWatcher &FileWatcher::instance() {
  static FileWatcher instance;
  return instance;
}

FileWatcher::~FileWatcher() {
  running_.store(false, std::memory_order_relaxed);
  for (auto &thread : watcher_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

std::shared_ptr<HotReloader>
FileWatcher::watch_file(const std::filesystem::path &path,
                        ReloadCallback reload_callback) {
  auto reloader =
      std::shared_ptr<HotReloader>(new HotReloader(path, reload_callback));

  std::lock_guard lock(mutex_);

  auto &entry = watch_list_[path];
  if (entry.watchers.empty()) {
    try {
      entry.last_write = std::filesystem::last_write_time(path);
    } catch (const std::filesystem::filesystem_error &) {
      // File doesn't exist or is otherwise inaccessible — start with epoch
      // time.
      entry.last_write = std::filesystem::file_time_type::min();
    }

    entry.path = path;
  } else if (entry.path != path) {
    throw std::runtime_error("Internal error: watch list key mismatch");
  }

  entry.watchers.push_back(reloader);

  return reloader;
}

HotReloader::HotReloader(std::filesystem::path script_path,
                         ReloadCallback reload_callback)
    : script_path_(std::move(script_path)), reload_callback_(reload_callback) {}

HotReloader::~HotReloader() {}

bool HotReloader::poll_and_reset() noexcept {
  return reload_requested_.exchange(false, std::memory_order_acq_rel);
}

const std::filesystem::path &HotReloader::path() const noexcept {
  return script_path_;
}

} // namespace hexes::fs
