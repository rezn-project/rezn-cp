#ifndef CP_LEDGR_TUI_BACKEND_HPP
#define CP_LEDGR_TUI_BACKEND_HPP

#include "imtui/imtui.h"

class TuiBackend
{
public:
    explicit TuiBackend(bool mouse = true);
    ~TuiBackend();

    void new_frame() const;
    void present() const;

private:
    // regular pointer to aviod double free()'ing
    ImTui::TScreen *screen_{};
};

#endif