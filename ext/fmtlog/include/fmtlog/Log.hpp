#pragma once

#include <fmt/format.h>
#include <exception>
#include <iostream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

const char* logLevelToString(LogLevel l);

void vlog(const LogLevel level, const char* file, int line, fmt::string_view format, fmt::format_args args);

template <typename S, typename... Args>
void log(const LogLevel level, const char* file, int line, const S& format, Args&&... args) {
  vlog(level, file, line, format,
      fmt::make_args_checked<Args...>(format, args...));
}

#define LOG_D(_f, ...) \
  log(LogLevel::DEBUG, __FILE__, __LINE__, FMT_STRING(_f), __VA_ARGS__)

#define LOG_I(_f, ...) \
  log(LogLevel::INFO, __FILE__, __LINE__, FMT_STRING(_f), __VA_ARGS__)

#define LOG_W(_f, ...) \
  log(LogLevel::WARNING, __FILE__, __LINE__, FMT_STRING(_f), __VA_ARGS__)

#define LOG_E(_f, ...) \
  log(LogLevel::ERROR, __FILE__, __LINE__, FMT_STRING(_f), __VA_ARGS__)

#define LOG_F(_f, ...) \
  log(LogLevel::FATAL, __FILE__, __LINE__, FMT_STRING(_f), __VA_ARGS__); \
  std::cout << std::endl << std::flush; \
  std::cerr << std::endl << std::flush; \
  throw std::runtime_error(fmt::format(_f, __VA_ARGS__))
