#pragma once
#include <fmt/format.h>

void vlog(const char* file, int line, fmt::string_view format, fmt::format_args args);

template <typename S, typename... Args>
void log(const char* file, int line, const S& format, Args&&... args) {
  vlog(file, line, format,
      fmt::make_args_checked<Args...>(format, args...));
}

#define MY_LOG(format, ...) \
  log(__FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)