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

    std::cout << "Connected to daemon at " << sock_path << std::endl;

    auto hostService = std::make_unique<HostService>(*api);
    auto hostsWindow = std::make_unique<HostsWindow>(*hostService);

    auto tuiBackend = std::make_unique<TuiBackend>(true);

    bool demo = true;
    int nframes = 0;
    float fval = 1.23f;
    bool showHostsNodesWindow = false;

    while (true)
    {
        tuiBackend->new_frame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open...", "CTRL+O"))
                {
                }
                if (ImGui::MenuItem("Open Hosts / Nodes manager", "CTRL+O"))
                {
                    showHostsNodesWindow = true;
                }
                if (ImGui::MenuItem("Quit", "CTRL+Q"))
                {
                    break;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z"))
                {
                }
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
                {
                } // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X"))
                {
                }
                if (ImGui::MenuItem("Copy", "CTRL+C"))
                {
                }
                if (ImGui::MenuItem("Paste", "CTRL+V"))
                {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (showHostsNodesWindow)
        {
            hostsWindow->draw(&showHostsNodesWindow);
        }

        tuiBackend->present();
    }

    return 0;
}