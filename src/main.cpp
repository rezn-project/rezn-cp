#include <vector>
#include <string>
#include <iostream>
#include <thread>

#include "imtui/imtui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

#include <nlohmann/json.hpp>
#include "readerwriterqueue.h"

#include "tui_backend.hpp"
#include "host_descriptor.hpp"
#include "host_service.hpp"
#include "hosts_window.hpp"
#include "api_client.hpp"
#include "log_window.hpp"
#include "log.hpp"
#include "step_ca_init_window.hpp"
#include "stats_ws_client.hpp"
#include <stats_window.hpp>

using json = nlohmann::json;

static ledgr::HostDescriptor newHost;

int main()
{
    const char *sock_env = std::getenv("LEDGR_SOCKET_PATH");
    std::string sock_path = sock_env ? sock_env : "/tmp/reznledgr.sock";

    const char *stats_ws_uri_env = std::getenv("REZN_STATS_WS_URI");
    std::string stats_ws_uri = stats_ws_uri_env ? stats_ws_uri_env : "ws://localhost:4000/stats/ws";

    std::unique_ptr<LedgerApiClient> api;
    try
    {
        api = std::make_unique<LedgerApiClient>(sock_path);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Failed to connect to daemon at " << sock_path << ": " << ex.what() << std::endl;
        return 1;
    }

    LOG_INFO("Connected to daemon at {}", sock_path);

    moodycamel::ReaderWriterQueue<StatsMap> q(1024);
    auto statsWsClient = std::make_unique<StatsWsClient>(stats_ws_uri, q);

    // Start the WebSocket client in a separate thread
    std::thread statsThread([&statsWsClient]()
                            {
    while (true) {
        auto result = statsWsClient->run_once();
        if (!result.has_value()) {
           LOG_ERROR("WebSocket connection failed: {}", result.error().message);
            // Add reconnection delay
           std::this_thread::sleep_for(std::chrono::seconds(5));
        }
   } });
    statsThread.detach();

    auto hostService = std::make_unique<HostService>(*api);
    auto hostsWindow = std::make_unique<HostsWindow>(*hostService);

    auto stepCaInitWindow = std::make_unique<StepCaInitWindow>();

    auto logWindow = std::make_unique<LogWindow>();

    auto statsWindow = std::make_unique<StatsWindow>(&q);

    auto tuiBackend = std::make_unique<TuiBackend>(true);

    bool showHostsNodesWindow = false;
    bool showStepCaInitWindow = false;
    bool showLogWindow = false;
    bool showStatsWindow = false;

    while (true)
    {
        tuiBackend->new_frame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Quit", "CTRL+Q"))
                {
                    break;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows"))
            {
                if (ImGui::MenuItem("Hosts / Nodes"))
                {
                    showHostsNodesWindow = true;
                }
                if (ImGui::MenuItem("Step CA Init"))
                {
                    showStepCaInitWindow = true;
                }
                if (ImGui::MenuItem("Logs"))
                {
                    showLogWindow = true;
                }
                if (ImGui::MenuItem("Stats"))
                {
                    showStatsWindow = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (showHostsNodesWindow)
        {
            hostsWindow->draw(&showHostsNodesWindow);
        }

        if (showLogWindow)
        {
            logWindow->draw(&showLogWindow);
        }

        if (showStepCaInitWindow)
        {
            stepCaInitWindow->draw(&showStepCaInitWindow);
        }

        if (showStatsWindow)
        {
            statsWindow->draw(&showStatsWindow);
        }

        tuiBackend->present();
    }

    return 0;
}