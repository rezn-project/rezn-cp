#ifndef CP_LOG_HPP
#define CP_LOG_HPP

#include <format>

#define LOG_DEBUG(fmt, ...) ::gLog.push("DEBUG", std::format((fmt), __VA_ARGS__))
#define LOG_INFO(fmt, ...) ::gLog.push("INFO", std::format((fmt), __VA_ARGS__))
#define LOG_WARN(fmt, ...) ::gLog.push("WARN", std::format((fmt), __VA_ARGS__))
#define LOG_ERROR(fmt, ...) ::gLog.push("ERROR", std::format((fmt), __VA_ARGS__))

#endif
