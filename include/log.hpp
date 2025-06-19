#ifndef CP_LOG_HPP
#define CP_LOG_HPP

#define LOG_INFO(msg) ::gLog.push("INFO", (msg))
#define LOG_WARN(msg) ::gLog.push("WARN", (msg))
#define LOG_ERROR(msg) ::gLog.push("ERROR", (msg))

#endif
