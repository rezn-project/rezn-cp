#ifndef CP_STEP_CA_INIT_WINDOW_HPP
#define CP_STEP_CA_INIT_WINDOW_HPP

#include <vector>
#include <string>
#include <ranges>
#include <string_view>
#include <filesystem>
#include <optional>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <reproc++/run.hpp>

#include "passgen_ctx.hpp"
#include "secure_tmp_files.hpp"
#include "log.hpp"
#include "cmd_runner.hpp"

using namespace std::string_view_literals;

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
            caPass = pg.generate(20);

        ImGui::InputText("Provisioner password", &provPass,
                         ImGuiInputTextFlags_Password);
        ImGui::SameLine();
        if (ImGui::Button("Generate##prov"))
            provPass = pg.generate(20);

        if (ImGui::Button("Run command"))
        {
            if (runCommand())
            {
                ImGui::OpenPopup("Success");
            }
            else
            {
                ImGui::OpenPopup("Error");
            }
        }
        if (ImGui::BeginPopupModal("Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("CA initialized successfully!");
            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Failed to initialize CA. Check logs for details.");
            if (!lastStderr.empty())
            {
                ImGui::Separator();
                ImGui::TextUnformatted(lastStderr.c_str());
            }

            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
    }

private:
    PassGenCtx pg;
    std::string caName = "rezn-seedr";
    std::string dnsName = "localhost";
    std::string addr = "127.0.0.1:9000";
    std::string provName = "admin";
    std::string caPass;
    std::string provPass;
    bool addAcme = false;
    bool enableSsh = true;
    bool noDb = false;

    std::string lastStdout;
    std::string lastStderr;

    fs::path caPwFilePath;
    fs::path provPwFilePath;

    bool runCommand()
    {
        std::vector<std::string> args{
            "ca", "init",
            "--name", caName,
            "--dns", dnsName,
            "--address", addr,
            "--provisioner", provName,
            "--deployment-type", "standalone"};

        std::optional<SecureTempFile> caPwFile;
        std::optional<SecureTempFile> provPwFile;

        if (addAcme)
            args.emplace_back("--acme");
        if (enableSsh)
            args.emplace_back("--ssh");
        if (!caPass.empty())
        {
            auto [caPwFileD, path] = secure_temp_file("step");

            if (::write(caPwFileD, caPass.data(), caPass.size()) != static_cast<ssize_t>(caPass.size()))
            {
                ::close(caPwFileD);
                LOG_WARN("Failed to write CA password to temporary file");
                return false;
            }
            if (::close(caPwFileD) != 0)
            {
                LOG_WARN("Failed to close CA password file descriptor");
                return false;
            }

            caPwFile = SecureTempFile(path);
            args.emplace_back("--password-file");
            args.emplace_back(path);
        }
        if (!provPass.empty())
        {
            auto [provPwFileD, path] = secure_temp_file("step");

            if (::write(provPwFileD, provPass.data(), provPass.size()) != static_cast<ssize_t>(provPass.size()))
            {
                ::close(provPwFileD);
                LOG_WARN("Failed to write provisioning password to temporary file");
                return false;
            }
            if (::close(provPwFileD) != 0)
            {
                LOG_WARN("Failed to close provisioning password file descriptor");
                return false;
            }

            provPwFile = SecureTempFile(path);
            args.emplace_back("--provisioner-password-file");
            args.emplace_back(path);
        }
        if (noDb)
            args.emplace_back("--no-db");

        std::vector<std::string> args2{
            "ls", "-al"};

        // auto joined = args | std::views::join_with(" "sv);
        // std::string cmd{std::ranges::begin(joined), std::ranges::end(joined)};

        auto ret = CmdRunner::run(args2);

        lastStdout = ret.out;
        lastStderr = ret.err;

        if (ret.ec)
        {
            LOG_WARN("spawn failed: {}", ret.ec.message());
            return false;
        }

        if (ret.exit_code != 0)
        {
            LOG_WARN("step exited {}", ret.exit_code);

            return false;
        }

        LOG_DEBUG("step ca init output:\n{}", ret.out);
        LOG_DEBUG("step ca init error:\n{}", ret.err);

        return true;
    }

    void runCliCommand()
    {
    }
};

#endif
