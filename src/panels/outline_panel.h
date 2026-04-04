#pragma once

namespace compiler
{
  namespace document
  {
    class editor_document_state;
  }
}

namespace panels
{
  void draw_outline_panel(const compiler::document::editor_document_state& document_state);
}
