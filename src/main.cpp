#include <vector>
#include <string>
#include <iostream>

#include "imtui/imtui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

#include <nlohmann/json.hpp>

#include "tui_backend.hpp"
#include "host_descriptor.hpp"
#include "host_service.hpp"
#include "hosts_window.hpp"
#include "api_client.hpp"
#include "log_window.hpp"
#include "log.hpp"
#include "step_ca_init_window.hpp"

using json = nlohmann::json;

static ledgr::HostDescriptor newHost;

int main()
{
    const char *sock_env = std::getenv("LEDGR_SOCKET_PATH");
    std::string sock_path = sock_env ? sock_env : "/tmp/reznledgr.sock";

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

    auto hostService = std::make_unique<HostService>(*api);
    auto hostsWindow = std::make_unique<HostsWindow>(*hostService);

    auto stepCaInitWindow = std::make_unique<StepCaInitWindow>();

    auto logWindow = std::make_unique<LogWindow>();

    auto tuiBackend = std::make_unique<TuiBackend>(true);

    bool showHostsNodesWindow = false;
    bool showStepCaInitWindow = false;
    bool showLogWindow = false;

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

        tuiBackend->present();
    }

    return 0;
}