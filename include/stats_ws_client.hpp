#ifndef CP_REZN_WS_CLIENT_HPP
#define CP_REZN_WS_CLIENT_HPP

#include <string>
#include <expected>
#include <chrono>
#include <format>
#include <source_location>
#include <unordered_map>

#include "ws_client/ws_client.hpp"
#include "ws_client/transport/builtin/TcpSocket.hpp"
#include "ws_client/transport/builtin/OpenSslSocket.hpp"
#include "ws_client/transport/builtin/DnsResolver.hpp"

#include <nlohmann/json.hpp>
#include "readerwriterqueue.h"

using StatsMap = std::unordered_map<std::string, double>; // container-id → cpu_avg

// -----------------------------------------------------------------------------
// Tiny logger that prints everything to stdout
// -----------------------------------------------------------------------------
struct WsLogger
{
    template <ws_client::LogLevel L, ws_client::LogTopic T>
    bool is_enabled() const noexcept { return true; }

    template <ws_client::LogLevel L, ws_client::LogTopic T>
    void log(std::string_view msg,
             std::source_location loc = std::source_location::current()) noexcept
    {
        std::cout << std::format("{} {} {}:{} | {}\n",
                                 ws_client::to_string(T),
                                 ws_client::to_string(L),
                                 ws_client::extract_log_file_name(loc.file_name()),
                                 loc.line(),
                                 msg);
    }
};

// -----------------------------------------------------------------------------
// StatsWsClient  — one public method: run_once()
// Reconnect logic lives in main(); run_once() exits on error/close.
// -----------------------------------------------------------------------------
class StatsWsClient
{
public:
    StatsWsClient(const std::string &uri,
                  moodycamel::ReaderWriterQueue<StatsMap> &q)
        : uri_(uri), queue_(q) {}

    /** Connect, pump until error/close, then return. */
    std::expected<void, ws_client::WSError> run_once()
    {
        using namespace ws_client;
        using namespace std::chrono_literals;

        WsLogger log;

        // 1. Parse URL -------------------------------------------------------
        WS_TRY(url, URL::parse(uri_));

        // 2. Resolve DNS -----------------------------------------------------
        DnsResolver dns(&log);
        WS_TRY(res, dns.resolve(url->host(), url->port_str(), AddrType::ipv4));
        AddressInfo &addr = (*res)[0];

        // 3. TCP socket ------------------------------------------------------
        auto tcp = TcpSocket(&log, std::move(addr));
        WS_TRYV(tcp.init());
        WS_TRYV(tcp.connect(2s));

        // 4. (Optional) TLS --------------------------------------------------
        std::unique_ptr<AbstractSocket> sock;
        if (url->scheme() == "wss")
        {
            OpenSslContext ctx(&log);
            WS_TRYV(ctx.init());
            WS_TRYV(ctx.set_default_verify_paths());

            auto ssl = std::make_unique<OpenSslSocket>(&log, std::move(tcp),
                                                       &ctx, url->host(), true);
            WS_TRYV(ssl->init());
            WS_TRYV(ssl->connect(2s));
            sock = std::move(ssl);
        }
        else
        {
            sock = std::make_unique<TcpSocket>(std::move(tcp)); // already connected
        }

        // 5. Handshake -------------------------------------------------------
        WebSocketClient ws(&log, std::move(sock));
        Handshake hs(&log, *url);
        WS_TRYV(ws.handshake(hs, 5s));

        // 6. Buffer ----------------------------------------------------------
        WS_TRY(buf, Buffer::create(4096, 1 * 1024 * 1024));

        for (;;)
        {
            auto evt = ws.read_message(*buf, 65s);

            // ---- TEXT MESSAGE ---------------------------------------------
            if (auto msg = std::get_if<Message>(&evt);
                msg && msg->type() == MessageType::text)
            {
                StatsMap sm;

                // parse with nlohmann::json
                auto j = nlohmann::json::parse(msg->text(), nullptr, false);
                if (!j.is_discarded())
                {
                    for (auto &[id, payload] : j.items())
                    {
                        try
                        {
                            double cpu = payload["stats"]["cpu_avg"].get<double>();
                            sm.emplace(id, cpu);
                        }
                        catch (...)
                        {
                            log.log<LogLevel::W, LogTopic::User>(
                                std::format("Malformed entry for id {}", id));
                        }
                    }
                    queue_.enqueue(std::move(sm));
                }
            }
            // ---- PING/PONG -------------------------------------------------
            else if (auto ping = std::get_if<PingFrame>(&evt))
            {
                ws.send_pong_frame(ping->payload_bytes());
            }
            // ---- CLOSE FRAME ----------------------------------------------
            else if (auto close = std::get_if<CloseFrame>(&evt))
            {
                ws.close(close_code::normal_closure);
                return {};
            }
            // ---- ERROR -----------------------------------------------------
            else if (auto err = std::get_if<WSError>(&evt))
            {
                ws.close(err->close_with_code);
                return std::unexpected(*err);
            }
        }
    }

private:
    std::string uri_;
    moodycamel::ReaderWriterQueue<StatsMap> &queue_;
};

#endif