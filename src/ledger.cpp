#include <vector>
#include <string>
#include <iostream>

#include "imtui/imtui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

#include <nlohmann/json.hpp>

#include "host_descriptor.hpp"
#include <api_client.hpp>

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

    std::vector<ledgr::HostDescriptor> hosts;
    try
    {
        hosts = api->list_hosts();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Failed to fetch hosts: " << ex.what() << std::endl;
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

    bool demo = true;
    int nframes = 0;
    float fval = 1.23f;

    while (true)
    {
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();

        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(4, 27), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(50.0, 10.0), ImGuiCond_Once);
        ImGui::Begin("Hello, world!");

        if (ImGui::BeginTable("HostLedger", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Host");
            ImGui::TableHeadersRow();

            for (const auto &host : hosts)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(host.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(host.host.c_str());
            }

            ImGui::EndTable();
        }

        if (ImGui::Button("Add Host"))
        {
            ImGui::OpenPopup("AddHostPopup");
        }

        if (ImGui::BeginPopupModal("AddHostPopup"))
        {
            ImGui::InputText("Name", &newHost.name);
            ImGui::InputText("Host", &newHost.host);

            if (ImGui::Button("Add"))
            {
                hosts.push_back(newHost);
                newHost = {};
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                newHost = {};
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

#ifndef __EMSCRIPTEN__
        ImGui::Text("%s", "");
        if (ImGui::Button("Exit program", {ImGui::GetContentRegionAvail().x, 2}))
        {
            break;
        }
#endif

        ImGui::End();

        ImGui::Render();

        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
        ImTui_ImplNcurses_DrawScreen();
    }

    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();

    ImGui::DestroyContext();

    return 0;
}