#include "outline_panel.h"

// clang-format off
#include "compiler/document/editor_document_state.h"

#include "imgui.h"
// clang-format on

void panels::draw_outline_panel(const compiler::document::editor_document_state& document_state)
{
  ImGui::SeparatorText("declarations");
  if (document_state.get_outline_items().empty())
  {
    ImGui::TextDisabled("No parsed declarations in the current snapshot.");
  }
  else
  {
    for (const compiler::document::editor_outline_item& item : document_state.get_outline_items())
    {
      const ImVec4 color = item.recovered ? ImVec4(0.90f, 0.66f, 0.35f, 1.00f) : ImVec4(0.82f, 0.88f, 0.96f, 1.00f);
      ImGui::TextDisabled("%04zu", item.line_number);
      ImGui::SameLine();
      ImGui::TextColored(color, "%s", item.label.c_str());
    }
  }

  ImGui::SeparatorText("diagnostics");
  if (document_state.get_diagnostics().empty())
  {
    ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.53f, 1.00f), "Snapshot parses cleanly.");
    return;
  }

  for (const compiler::document::editor_diagnostic_item& diagnostic : document_state.get_diagnostics())
  {
    const ImVec4 badge_color
      = diagnostic.lexical ? ImVec4(0.93f, 0.46f, 0.36f, 1.00f) : ImVec4(0.90f, 0.66f, 0.35f, 1.00f);
    ImGui::TextColored(badge_color, diagnostic.lexical ? "lex" : "parse");
    ImGui::SameLine();
    ImGui::TextWrapped("L%zu: %s", diagnostic.line_number, diagnostic.message.c_str());
  }
}
