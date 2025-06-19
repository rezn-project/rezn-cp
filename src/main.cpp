#include <vector>
#include <string>
#include <iostream>

#include "imtui/imtui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

#include <nlohmann/json.hpp>

#include "tui_backend.hpp"
#include "host_descriptor.hpp"
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

    auto tuiBackend = std::make_unique<TuiBackend>(true);

    bool demo = true;
    int nframes = 0;
    float fval = 1.23f;
    bool showHostsNodesWindow = true;

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
            ImGui::SetNextWindowPos(ImVec2(0, 1), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(150.0, 20.0), ImGuiCond_Once);

            if (ImGui::Begin("Hosts / Nodes", &showHostsNodesWindow, ImGuiWindowFlags_NoCollapse))
            {

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
                    ImGui::InputText("ID", &newHost.id);
                    ImGui::InputText("Name", &newHost.name);
                    ImGui::InputText("Host", &newHost.host);

                    if (ImGui::Button("Add"))
                    {
                        std::string error;

                        if (api->add_host(newHost, &error))
                        {
                            hosts.push_back(newHost);
                            newHost = {};
                            ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            // Consider showing error to user
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        newHost = {};
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }

            ImGui::End();
        }

        tuiBackend->present();
    }

    return 0;
}