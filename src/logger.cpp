#include <hexes/logger.hpp>

namespace hexes {

Logger::Logger() {
  // Create a file logger that writes to app.log in the current working directory
  logger_ = spdlog::basic_logger_mt("hexes", "app.log");

  // Set default log level to info
  logger_->set_level(spdlog::level::info);

  // Set pattern: [timestamp] [level] message
  logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

  // Flush on every log (ensures nothing is lost on crash)
  logger_->flush_on(spdlog::level::trace);
}

Logger &Logger::instance() {
  static Logger instance;
  return instance;
}

} // namespace hexes

