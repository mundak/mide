#include "editor_panel.h"

#include "imgui.h"
#include "panel_common.h"

#include <array>
#include <cstdint>

namespace
{
  struct source_line
  {
    int32_t number;
    const char* text;
    ImVec4 color;
    bool selected;
  };

  constexpr std::array<source_line, 15> SOURCE_LINES { {
    { 1, "#include <stdint.h>", ImVec4(0.63f, 0.77f, 1.00f, 1.00f), false },
    { 2, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 3, "static uint32_t frame_counter = 0;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 4, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 5, "int32_t compile_step(struct unit* unit)", ImVec4(0.51f, 0.76f, 1.00f, 1.00f), false },
    { 6, "{", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 7, "  if (unit == nullptr)", ImVec4(0.51f, 0.76f, 1.00f, 1.00f), false },
    { 8, "  {", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 9, "    return -1;", ImVec4(0.93f, 0.72f, 0.45f, 1.00f), false },
    { 10, "  }", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 11, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 12, "  unit->cache_state = CACHE_STATE_WARM;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), true },
    { 13, "  frame_counter += unit->dirty_functions;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
    { 14, "  return emit_live_assembly(unit);", ImVec4(0.93f, 0.72f, 0.45f, 1.00f), false },
    { 15, "}", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false },
  } };
}

void panels::draw_editor_panel(ImFont* mono_font)
{
  if (ImGui::BeginTabBar("editor_tabs"))
  {
    if (ImGui::BeginTabItem("main.c"))
    {
      ImGui::Separator();

      detail::push_mono_font(mono_font);
      if (ImGui::BeginTable("source_table", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
      {
        ImGui::TableSetupColumn("line", ImGuiTableColumnFlags_WidthFixed, 56.0f);
        ImGui::TableSetupColumn("code", ImGuiTableColumnFlags_WidthStretch);

        for (const source_line& line : SOURCE_LINES)
        {
          ImGui::TableNextRow();
          if (line.selected)
          {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(33, 41, 55, 255));
          }
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("%04d", line.number);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextColored(line.color, "%s", line.text);
        }

        ImGui::EndTable();
      }
      detail::pop_mono_font(mono_font);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("compiler_state.h"))
    {
      ImGui::TextDisabled("This tab exists only to sell the prototype layout.");
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}
