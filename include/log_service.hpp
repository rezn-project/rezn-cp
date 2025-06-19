// log_service.hpp — minimal in‑process ring‑buffer logger (C++23)
// -----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <mutex>
#include <chrono>

/**
 * LogEntry — plain struct so UI widgets can format as they wish.
 */
struct LogEntry
{
    std::chrono::system_clock::time_point ts;
    std::string level; // "INFO", "WARN", ...
    std::string msg;   // already formatted text
};

/**
 * LogService
 * ----------
 * Fixed‑capacity **circular buffer**: push is O(1) with no allocations after
 * the first `cap` entries, perfect for high‑frequency logging inside a TUI.
 * Thread‑safe (single `std::mutex`).
 */
class LogService
{
public:
    explicit LogService(std::size_t cap = 2048) : cap_{cap}, buf_(cap) {}

    template <typename S>
    void push(std::string_view lvl, S &&str)
    {
        std::lock_guard lock{mtx_};

        const std::size_t idx = (head_ + size_) % cap_;
        buf_[idx] = {std::chrono::system_clock::now(), std::string(lvl),
                     std::forward<S>(str)};

        if (size_ < cap_)
        {
            ++size_; // still filling initial capacity
        }
        else
        {
            head_ = (head_ + 1) % cap_; // buffer full → advance head
        }
    }

    /**
     * Snapshot in logical order (oldest → newest).  Copying `size_` strings is
     * fine for a 2‑4 k buffer and isolates the consumer thread.
     */
    [[nodiscard]] std::vector<LogEntry> snapshot() const
    {
        std::lock_guard lock{mtx_};
        std::vector<LogEntry> out;
        out.reserve(size_);
        for (std::size_t i = 0; i < size_; ++i)
        {
            const std::size_t idx = (head_ + i) % cap_;
            out.push_back(buf_[idx]);
        }
        return out;
    }

    /** Current number of stored entries (≤ cap). */
    [[nodiscard]] std::size_t size() const noexcept
    {
        std::lock_guard lock{mtx_};
        return size_;
    }

private:
    const std::size_t cap_;

    mutable std::mutex mtx_;
    std::vector<LogEntry> buf_; // pre‑allocated storage
    std::size_t head_{};        // index of the oldest entry
    std::size_t size_{};        // number of valid records in buf_
};

// One global instance for simplicity; inject or wrap if you prefer.
inline LogService gLog;
