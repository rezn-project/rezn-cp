#include "client.hpp"
#include <sstream>

namespace
{
    struct CurlGlobalGuard
    {
        CurlGlobalGuard()
        {
            CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (rc != CURLE_OK)
                throw std::runtime_error(
                    std::string("curl_global_init failed: ") + curl_easy_strerror(rc));
        }
        ~CurlGlobalGuard()
        {
            curl_global_cleanup();
        }
    };

    /* Static instance: constructed before main(), destroyed on exit */
    static CurlGlobalGuard curl_global_guard;
} // namespace

LedgerClient::LedgerClient(const std::string &socket_path,
                           std::chrono::seconds timeout)
    : curl_{curl_easy_init()},
      socket_path_{socket_path},
      timeout_sec_{static_cast<long>(timeout.count())}
{
    if (!curl_)
        throw std::runtime_error("curl_easy_init() failed");

    apply_invariants();
}

void LedgerClient::apply_invariants()
{
    curl_easy_setopt(curl_.get(), CURLOPT_UNIX_SOCKET_PATH, socket_path_.c_str());
    curl_easy_setopt(curl_.get(), CURLOPT_URL, "http://localhost/");
    curl_easy_setopt(curl_.get(), CURLOPT_WRITEFUNCTION, &LedgerClient::write_cb);
    curl_easy_setopt(curl_.get(), CURLOPT_TIMEOUT, timeout_sec_);
    curl_easy_setopt(curl_.get(), CURLOPT_NOSIGNAL, 1L);
}

LedgerClient::~LedgerClient() = default;

size_t LedgerClient::write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *buf = static_cast<std::string *>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

nlohmann::json LedgerClient::send_request(const nlohmann::json &req)
{
    /* ---------- request-specific state ---------- */
    const std::string body = req.dump(); // JSON payload (must stay alive)
    std::string resp_body;               // will collect response

    struct curl_slist *hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    /* ---------- PER-REQUEST options (must be set each call) ---------- */
    curl_easy_setopt(curl_.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl_.get(), CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl_.get(), CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl_.get(), CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl_.get(), CURLOPT_WRITEDATA, &resp_body);

    /* ---------- perform ---------- */
    CURLcode rc = curl_easy_perform(curl_.get());

    /* hdrs is no longer needed after the transfer */
    curl_slist_free_all(hdrs); // safe: libcurl does NOT free it

    /* ---------- status code BEFORE reset ---------- */
    long http_status = 0;
    if (rc == CURLE_OK)
        curl_easy_getinfo(curl_.get(), CURLINFO_RESPONSE_CODE, &http_status);

    /* ---------- reset handle to clear dangling pointers ---------- */
    curl_easy_reset(curl_.get());

    apply_invariants();

    /* ---------- error handling ---------- */
    if (rc != CURLE_OK)
        throw std::runtime_error("libcurl: " +
                                 std::string(curl_easy_strerror(rc)));

    if (http_status != 200)
        throw std::runtime_error("HTTP " + std::to_string(http_status) +
                                 " - body: " + resp_body);

    /* ---------- JSON parse ---------- */
    try
    {
        return nlohmann::json::parse(resp_body);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("JSON parse error: " +
                                 std::string(e.what()) +
                                 " - raw: " + resp_body);
    }
}
