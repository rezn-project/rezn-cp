// stats_json.hpp
#pragma once
#include "stats_model.hpp"
#include <nlohmann/json.hpp>

// ---- Stats ----
inline void to_json(nlohmann::json &j, const Stats &s)
{
    j = nlohmann::json{
        {"cpu_avg", s.cpu_avg},
        {"max_mem", s.max_mem}};
}
inline void from_json(const nlohmann::json &j, Stats &s)
{
    if (j.contains("cpu_avg") && !j["cpu_avg"].is_null())
        s.cpu_avg = j["cpu_avg"].get<double>();
    if (j.contains("max_mem") && !j["max_mem"].is_null())
        s.max_mem = j["max_mem"].get<uint64_t>();
}

// ---- TimestampedStats ----
inline void to_json(nlohmann::json &j, const TimestampedStats &ts)
{
    j = nlohmann::json{
        {"stats", ts.stats},
        {"timestamp", ts.timestamp}};
}
inline void from_json(const nlohmann::json &j, TimestampedStats &ts)
{
    j.at("stats").get_to(ts.stats);
    j.at("timestamp").get_to(ts.timestamp);
}
