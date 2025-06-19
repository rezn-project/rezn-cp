#pragma once
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <chrono>
#include <memory>

class LedgerClient
{
public:
    explicit LedgerClient(const std::string &socket_path,
                          std::chrono::seconds timeout = std::chrono::seconds{5});
    ~LedgerClient();

    nlohmann::json send_request(const nlohmann::json &req);

private:
    struct CurlDeleter
    {
        void operator()(CURL *c) const
        {
            if (c)
                curl_easy_cleanup(c);
        }
    };

    std::unique_ptr<CURL, CurlDeleter> curl_;
    std::string socket_path_;
    long timeout_sec_;

    static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata);
    void apply_invariants();
};
