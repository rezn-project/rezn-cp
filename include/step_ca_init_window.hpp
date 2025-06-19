#ifndef CP_STEP_CA_INIT_WINDOW_HPP
#define CP_STEP_CA_INIT_WINDOW_HPP

#include <vector>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <string>

#include "passgen.hpp"

class StepCaInitWindow
{
public:
    void draw(bool *open)
    {
        if (!open || !*open)
            return;

        ImGui::SetNextWindowPos({0, 1}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({120, 30}, ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Step CA Init", open))
        {
            ImGui::End();
            return;
        }

        ImGui::InputText("CA name", &caName);
        ImGui::InputText("DNS name", &dnsName);
        ImGui::InputText("Listen address", &addr);
        ImGui::InputText("Provisioner name", &provName);

        ImGui::Checkbox("Add ACME provisioner", &addAcme);
        ImGui::Checkbox("Enable SSH CA", &enableSsh);
        ImGui::Checkbox("Disable DB", &noDb);

        // Password fields with inline “Generate” buttons
        ImGui::InputText("CA password", &caPass,
                         ImGuiInputTextFlags_Password);
        ImGui::SameLine();
        if (ImGui::Button("Generate##ca"))
            caPass = make_password();

        ImGui::InputText("Provisioner password", &provPass,
                         ImGuiInputTextFlags_Password);
        ImGui::SameLine();
        if (ImGui::Button("Generate##prov"))
            provPass = make_password();

        ImGui::End();
    }

private:
    std::string caName = "rezn-seedr";
    std::string dnsName = "localhost";
    std::string addr = "127.0.0.1:9000";
    std::string provName = "admin";
    std::string caPass = "";
    std::string provPass = "";
    bool addAcme = false;
    bool enableSsh = true;
    bool noDb = false;
};

#endif
