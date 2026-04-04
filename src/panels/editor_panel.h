#pragma once

struct ImFont;

namespace compiler
{
  namespace document
  {
    class editor_document_state;
  }
}

namespace panels
{
  void draw_editor_panel(compiler::document::editor_document_state& document_state, ImFont* mono_font);
}
