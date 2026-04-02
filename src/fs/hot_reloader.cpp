#include <hexes/fs/hot_reloader.hpp>

namespace hexes::fs {

FileWatcher::FileWatcher() {
  watch_list_.reserve(16); // Avoid initial resizes for small watch counts.
                           //
  watcher_threads_.reserve(thread_count_);
}

void FileWatcher::initialize_watchers() {
  for (int i = 0; i < thread_count_; ++i) {
    watcher_threads_.emplace_back([this]() {
      while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
  running_.store(true, std::memory_order_release);
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

HotReloader FileWatcher::watch_file(const std::filesystem::path &path,
                                    ReloadCallback reload_callback) {
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

  return HotReloader(path, reload_callback);
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
