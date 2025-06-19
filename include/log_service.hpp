#ifndef CP_LOG_SERVICE_HPP
#define CP_LOG_SERVICE_HPP

#include <vector>
#include <mutex>
#include <string>
#include <chrono>

struct LogEntry
{
    std::chrono::system_clock::time_point ts;
    std::string level;
    std::string msg;
};

class LogService
{
public:
    explicit LogService(size_t cap = 2048) : cap_{cap} {}

    template <typename S>
    void push(std::string_view lvl, S &&str)
    {
        std::lock_guard lock{mtx_};
        if (buf_.size() == cap_)
            buf_.erase(buf_.begin());
        buf_.push_back(
            {std::chrono::system_clock::now(), std::string(lvl), std::forward<S>(str)});
    }

    [[nodiscard]] std::vector<LogEntry> snapshot() const
    {
        std::lock_guard lock{mtx_};
        return buf_; // copy-out; cheap (2 KB) for 2 k lines
    }

private:
    const size_t cap_;
    mutable std::mutex mtx_;
    std::vector<LogEntry> buf_;
};

inline LogService gLog;

#endif
