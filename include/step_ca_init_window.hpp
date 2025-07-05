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
#include "util_find_executable.hpp"
#include "string_utils.hpp"

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

        // --------

        ImGui::InputText("CA password", &caPass);

        ImGui::SameLine();
        if (ImGui::Button("Generate##ca"))
            caPass = pg.generate(20);

        ImGui::SameLine();
        if (ImGui::Button("Copy##ca"))
            ImGui::SetClipboardText(caPass.c_str());

        // --------

        ImGui::InputText("Provisioner password", &provPass);

        ImGui::SameLine();
        if (ImGui::Button("Generate##prov"))
            provPass = pg.generate(20);

        ImGui::SameLine();
        if (ImGui::Button("Copy##prov"))
            ImGui::SetClipboardText(provPass.c_str());

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
            if (!lastStdout.empty())
            {
                ImGui::Separator();
                ImGui::TextUnformatted(lastStdout.c_str());
            }

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
        lastStdout.clear();
        lastStderr.clear();

        auto pathToStepCli = find_executable("step");

        if (!pathToStepCli.has_value())
        {
            LOG_WARN("step executable not found");
            lastStderr = "step executable not found";
            return false;
        }

        std::vector<std::string> stepPathArgs{
            pathToStepCli.value().string(),
            "path"};

        auto stepPathRet = CmdRunner::run(stepPathArgs);

        if (stepPathRet.ec)
        {
            LOG_WARN("Failed to get step path: {}", stepPathRet.ec.message());
            lastStderr = "Failed to get step path: " + stepPathRet.ec.message();
            return false;
        }

        auto stepPath = fs::path(std::string(util::trim(stepPathRet.out)));

        LOG_DEBUG("Step path: {}", stepPath.string());
        LOG_DEBUG("Path to ca.json: {}", std::string(stepPath / "config" / "ca.json"));

        if (std::filesystem::exists(stepPath / "config" / "ca.json"))
        {
            LOG_WARN("CA already initialized at {}", stepPath.string());
            lastStderr = "CA already initialized at " + stepPath.string();
            return false;
        }

        std::vector<std::string> stepCaInitArgs{
            pathToStepCli.value().string(),
            "ca", "init",
            "--name", caName,
            "--dns", dnsName,
            "--address", addr,
            "--provisioner", provName,
            "--deployment-type", "standalone",
            "--remote-management"};

        std::optional<SecureTempFile> caPwFile;
        std::optional<SecureTempFile> provPwFile;

        if (addAcme)
            stepCaInitArgs.emplace_back("--acme");
        if (enableSsh)
            stepCaInitArgs.emplace_back("--ssh");
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
            stepCaInitArgs.emplace_back("--password-file");
            stepCaInitArgs.emplace_back(path);
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
            stepCaInitArgs.emplace_back("--provisioner-password-file");
            stepCaInitArgs.emplace_back(path);
        }
        if (noDb)
            stepCaInitArgs.emplace_back("--no-db");

        std::string fullCommand = util::join(stepCaInitArgs);

        LOG_DEBUG("Running step ca init: {}", fullCommand);

        auto ret = CmdRunner::run(stepCaInitArgs);

        lastStdout = ret.out;
        lastStderr = ret.err;

        if (ret.ec)
        {
            LOG_WARN("spawn failed: {}", ret.ec.message());
            lastStderr = "spawn failed: " + ret.ec.message();
            return false;
        }

        if (ret.exit_code != 0)
        {
            LOG_WARN("step exited {}", ret.exit_code);
            lastStderr = "step exited with code " + std::to_string(ret.exit_code);
            return false;
        }

        LOG_DEBUG("step ca init output:\n{}", ret.out);
        LOG_DEBUG("step ca init error:\n{}", ret.err);

        return true;
    }
};

#endif
