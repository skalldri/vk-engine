#pragma once

#include <fmt/format.h>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

const char* logLevelToString(LogLevel l);

void vlog(const LogLevel level, const char* file, int line, fmt::string_view format, fmt::format_args args);

template <typename S, typename... Args>
void log(const LogLevel level, const char* file, int line, const S& format, Args&&... args) {
  vlog(level, file, line, format,
      fmt::make_args_checked<Args...>(format, args...));
}

#define LOG_D(format, ...) \
  log(LogLevel::DEBUG, __FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)

#define LOG_I(format, ...) \
  log(LogLevel::INFO, __FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)

#define LOG_W(format, ...) \
  log(LogLevel::WARNING, __FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)

#define LOG_E(format, ...) \
  log(LogLevel::ERROR, __FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)
