#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace ledgr
{
    struct HostDescriptor
    {
        std::string id;
        std::string name;
        std::string host;
    };
}

namespace ledgr
{

    inline void to_json(nlohmann::json &j, const HostDescriptor &h)
    {
        j = nlohmann::json{{"id", h.id}, {"name", h.name}, {"host", h.host}};
    }
    inline void from_json(const nlohmann::json &j, HostDescriptor &h)
    {
        h.id = j.value("id", "");
        h.name = j.value("name", "");
        h.host = j.value("host", "");
    }
}