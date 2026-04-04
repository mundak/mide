#pragma once

// clang-format off
#include "compiler/document/editor_document_state.h"

#include "imgui.h"
// clang-format on

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

    compiler::document::editor_document_state m_document_state;
    bool m_layout_initialized;
  };
}
