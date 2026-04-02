#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace hexes {

class Logger {
public:
  static Logger &instance();

  // Delete copy and move
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;

  std::shared_ptr<spdlog::logger> get() { return logger_; }

  // Convenience methods
  template <typename... Args>
  void trace(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->trace(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->debug(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->info(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->warn(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->error(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void critical(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    logger_->critical(fmt, std::forward<Args>(args)...);
  }

private:
  Logger();
  ~Logger() = default;

  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace hexes

