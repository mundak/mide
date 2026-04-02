#include "assembly_panel.h"

#include "imgui.h"
#include "panel_common.h"

#include <array>

namespace
{
  struct assembly_row
  {
    const char* address;
    const char* bytes;
    const char* instruction;
    const char* notes;
    bool selected;
  };

  constexpr std::array<assembly_row, 7> ASSEMBLY_ROWS { {
    { "00007FF6`18001040", "48 89 5C 24 08", "mov [rsp+08h], rbx", "prologue", false },
    { "00007FF6`18001045", "48 83 EC 20", "sub rsp, 20h", "stack frame", false },
    { "00007FF6`18001049", "48 85 C9", "test rcx, rcx", "unit == nullptr", false },
    { "00007FF6`1800104C", "74 19", "je 00007FF6`18001067", "early exit", false },
    { "00007FF6`1800104E", "83 41 14 01", "add dword ptr [rcx+14h], 1", "cache_state", true },
    { "00007FF6`18001052", "8B 41 18", "mov eax, dword ptr [rcx+18h]", "dirty_functions", false },
    { "00007FF6`18001055", "E8 96 02 00 00", "call emit_live_assembly", "live sync", false },
  } };
}

void panels::draw_assembly_panel(ImFont* mono_font)
{
  detail::push_mono_font(mono_font);
  if (ImGui::BeginTable(
        "assembly_table",
        4,
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY
          | ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableSetupColumn("address", ImGuiTableColumnFlags_WidthStretch, 1.8f);
    ImGui::TableSetupColumn("bytes", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("instruction", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("notes", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableHeadersRow();

    for (const assembly_row& row : ASSEMBLY_ROWS)
    {
      ImGui::TableNextRow();
      if (row.selected)
      {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(38, 45, 59, 255));
      }
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", row.address);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextColored(ImVec4(0.51f, 0.76f, 1.00f, 1.00f), "%s", row.bytes);
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s", row.instruction);
      ImGui::TableSetColumnIndex(3);
      ImGui::TextDisabled("%s", row.notes);
    }

    ImGui::EndTable();
  }
  detail::pop_mono_font(mono_font);
}
