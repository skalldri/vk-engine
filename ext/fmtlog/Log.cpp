#include <filesystem>
#include <fmtlog/Log.hpp>

const char* logLevelToString(LogLevel l) {
  switch (l) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
  }

  return "UNKNOWN";
}

std::string getModuleName(const char* file) {
  // TODO: use a std::map to cache this translation?
  // Would also allow users to override the module name
  std::filesystem::path sourceFilePath(file);
  return sourceFilePath.stem().generic_string();
}

bool shouldPrintLineNumber(const char* file, LogLevel level) {
  // TODO: add more fancy stuff later (like chaning log-level per module)
  if (level == LogLevel::WARNING || level == LogLevel::ERROR) {
    return true;
  } else {
    return false;
  }
}

bool isLogLevelActive(const LogLevel level, const char* file) {
  // All log levels are active initially
  return true;
}

void vlog(const LogLevel level, const char* file, int line, fmt::string_view format,
          fmt::format_args args) {

  if (shouldPrintLineNumber(file, level)) {
    fmt::print("{} @ {}: ", getModuleName(file), line);
  } else {
    fmt::print("{}: ", getModuleName(file));
  }
  
  fmt::vprint(format, args);

  fmt::print("\n");
}