#ifndef CP_REZN_WS_CLIENT_HPP
#define CP_REZN_WS_CLIENT_HPP

#include <string>
#include <expected>
#include <chrono>
#include <format>
#include <source_location>

#include <nlohmann/json.hpp>
#include <readerwriterqueue.h>

#include "ws_client/ws_client.hpp"
#include "ws_client/transport/builtin/TcpSocket.hpp"
#include "ws_client/transport/builtin/OpenSslSocket.hpp"
#include "ws_client/transport/builtin/DnsResolver.hpp"

#include "stats_model.hpp"
#include "stats_json.hpp"

#include "log.hpp"

using namespace std::chrono_literals;
namespace wsc = ws_client;

// ──────────────────────────────────────────────────────────────
// Console logger (drop-in for websocketclient Logger concept)
// ──────────────────────────────────────────────────────────────
struct WsLogger
{
    template <wsc::LogLevel L, wsc::LogTopic T>
    bool is_enabled() const noexcept { return true; }

    template <wsc::LogLevel L, wsc::LogTopic T>
    void log(std::string_view msg,
             std::source_location loc = std::source_location::current()) noexcept
    {
        // std::cout << std::format("{} {} {}:{} | {}\n",
        //                          wsc::to_string(T),
        //                          wsc::to_string(L),
        //                          wsc::extract_log_file_name(loc.file_name()),
        //                          loc.line(),
        //                          msg);

        LOG_DEBUG("{}", msg);
    }
};

// ──────────────────────────────────────────────────────────────
// StatsWsClient – one-shot connect/consume; caller decides
// whether to reconnect by calling run_once() again.
// ──────────────────────────────────────────────────────────────
class StatsWsClient
{
public:
    StatsWsClient(std::string uri,
                  moodycamel::ReaderWriterQueue<StatsMap> &q)
        : uri_(std::move(uri)), queue_(q) {}

    std::expected<void, wsc::WSError> run_once()
    {
        WS_TRY(url, wsc::URL::parse(uri_));

        // DNS
        wsc::DnsResolver dns(&log_);
        WS_TRY(r, dns.resolve(url->host(), url->port_str(), wsc::AddrType::ipv4));
        wsc::AddressInfo &addr = (*r)[0];

        // one shared read buffer
        WS_TRY(buf, wsc::Buffer::create(4 * 1024, 1 * 1024 * 1024));

        // decide transport by scheme prefix (cheap & cheerful)
        const bool secure = uri_.rfind("wss://", 0) == 0;

        if (secure)
        {
            // TCP
            wsc::TcpSocket<WsLogger> tcp(&log_, std::move(addr));
            WS_TRYV(tcp.init());
            WS_TRYV(tcp.connect(2s));

            // TLS wrap
            wsc::OpenSslContext ctx(&log_);
            WS_TRYV(ctx.init());
            WS_TRYV(ctx.set_default_verify_paths());

            wsc::OpenSslSocket<WsLogger> ssl(&log_, std::move(tcp), &ctx, url->host(), true);
            WS_TRYV(ssl.init());
            WS_TRYV(ssl.connect(2s));

            // WebSocket client (three template params!)
            wsc::WebSocketClient<WsLogger,
                                 wsc::OpenSslSocket<WsLogger>,
                                 wsc::DefaultMaskKeyGen>
                client(&log_, std::move(ssl));

            return pump_messages(*url, client, *buf);
        }
        else
        {
            wsc::TcpSocket<WsLogger> tcp(&log_, std::move(addr));
            WS_TRYV(tcp.init());
            WS_TRYV(tcp.connect(2s));

            wsc::WebSocketClient<WsLogger,
                                 wsc::TcpSocket<WsLogger>,
                                 wsc::DefaultMaskKeyGen>
                client(&log_, std::move(tcp));

            return pump_messages(*url, client, *buf);
        }
    }

private:
    // ------------ message loop (templated only on socket type) -------------
    template <typename TSocket>
    std::expected<void, wsc::WSError> pump_messages(
        const wsc::URL &url,
        wsc::WebSocketClient<WsLogger, TSocket, wsc::DefaultMaskKeyGen> &client,
        wsc::Buffer &buf)
    {
        wsc::Handshake hs(&log_, url);
        WS_TRYV(client.handshake(hs, 5s));

        while (true)
        {
            auto evt = client.read_message(buf, 65s);

            // TEXT ----------------------------------------------------------
            if (auto msg = std::get_if<wsc::Message>(&evt);
                msg && msg->type == wsc::MessageType::text)
            {
                StatsMap sm;

                std::string_view json_sv = msg->to_string_view();
                auto j = nlohmann::json::parse(json_sv, nullptr, false);

                if (!j.is_discarded())
                {
                    for (auto &[id, val] : j.items())
                    {
                        try
                        {
                            sm.emplace(id, val.get<TimestampedStats>());
                        }
                        catch (...)
                        { /* skip malformed */
                        }
                    }
                    queue_.enqueue(std::move(sm));
                }
            }
            // PING ----------------------------------------------------------
            else if (auto ping = std::get_if<wsc::PingFrame>(&evt))
            {
                client.send_pong_frame(ping->payload_bytes());
            }
            // CLOSE ---------------------------------------------------------
            else if (auto close = std::get_if<wsc::CloseFrame>(&evt))
            {
                client.close(wsc::close_code::normal_closure);
                return {};
            }
            // ERROR ---------------------------------------------------------
            else if (auto err = std::get_if<wsc::WSError>(&evt))
            {
                client.close(err->close_with_code);
                return std::unexpected(*err);
            }
        }
    }

    // members
    std::string uri_;
    moodycamel::ReaderWriterQueue<StatsMap> &queue_;
    WsLogger log_;
};

#endif // CP_REZN_WS_CLIENT_HPP
