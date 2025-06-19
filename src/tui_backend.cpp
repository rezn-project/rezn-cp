#include "tui_backend.hpp"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imtui/imtui-impl-ncurses.h"

TuiBackend::TuiBackend(bool mouse)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    screen_ = ImTui_ImplNcurses_Init(mouse);
    ImTui_ImplText_Init();
}

TuiBackend::~TuiBackend()
{
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
    ImGui::DestroyContext();
}

void TuiBackend::new_frame() const
{
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();
}

void TuiBackend::present() const
{
    ImGui::Render();
    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen_);
    ImTui_ImplNcurses_DrawScreen();
}
