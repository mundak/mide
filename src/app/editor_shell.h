#pragma once

#include "imgui.h"

namespace app
{
  class editor_shell
  {
  public:
    editor_shell();

    void draw();

  private:
    void draw_dockspace();
    void draw_panel_windows();
    void draw_status_bar();
    void draw_top_bar();
    void ensure_layout(ImGuiID dockspace_id);

    bool m_layout_initialized;
  };
}
