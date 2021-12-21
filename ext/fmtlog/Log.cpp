#include <filesystem>
#include <fmtlog/Log.hpp>

const char* logLevelToString(LogLevel l) {
  switch (l) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warning:
      return "WARNING";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Fatal:
      return "FATAL";
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
  if (level == LogLevel::Warning || level == LogLevel::Error) {
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
    fmt::print("[{}] {} @ {}: ", logLevelToString(level), getModuleName(file), line);
  } else {
    fmt::print("[{}] {}: ", logLevelToString(level), getModuleName(file));
  }
  
  fmt::vprint(format, args);

  fmt::print("\n");
}