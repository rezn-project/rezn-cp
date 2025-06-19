#ifndef CP_LOG_WINDOW_HPP
#define CP_LOG_WINDOW_HPP

#include <vector>
#include <imgui.h>

#include "log_service.hpp"

class LogWindow
{
public:
    void draw(bool *open)
    {
        if (!open || !*open)
            return;

        ImGui::SetNextWindowPos({0, 1}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({120, 30}, ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Logs", open))
        {
            ImGui::End();
            return;
        }

        // freeze snapshot so scrolling doesn’t fight producer
        if (ImGui::Button("Clear"))
            lines_.clear();
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
            lines_ = gLog.snapshot();

        ImGui::Separator();

        ImGuiListClipper clip;
        clip.Begin(static_cast<int>(lines_.size()));
        while (clip.Step())
        {
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i)
            {
                const auto &e = lines_[i];
                ImGui::TextUnformatted(e.msg.c_str());
            }
        }

        ImGui::End();
    }

private:
    std::vector<LogEntry> lines_;
};

#endif
