// stats_model.hpp
#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

struct Stats
{
    std::optional<double> cpu_avg;   // f64  -> double
    std::optional<uint64_t> max_mem; // u64  -> uint64_t
};

struct TimestampedStats
{
    Stats stats;
    uint64_t timestamp; // u64  -> uint64_t
};

// key = container ID (string), value = TimestampedStats
using StatsMap = std::map<std::string, TimestampedStats>; // BTreeMap -> std::map
