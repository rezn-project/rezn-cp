#ifndef CP_TUI_BACKEND_CPP
#define CP_TUI_BACKEND_CPP

#include "imtui/imtui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

class TuiBackend
{
public:
    explicit TuiBackend(bool mouse = true)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        screen_ = ImTui_ImplNcurses_Init(mouse);
        ImTui_ImplText_Init();
    }
    ~TuiBackend()
    {
        ImTui_ImplText_Shutdown();
        ImTui_ImplNcurses_Shutdown();
        ImGui::DestroyContext();
    }

    void new_frame() const
    {
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();
        ImGui::NewFrame();
    }

    void present() const
    {
        ImGui::Render();
        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen_);
        ImTui_ImplNcurses_DrawScreen();
    }

private:
    ImTui::TScreen *screen_{};
};

#endif
