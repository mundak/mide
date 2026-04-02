#include "memory_panel.h"

#include "imgui.h"

#include <array>
#include <cstdint>

namespace
{
  struct segment
  {
    const char* label;
    float value;
    uint32_t color;
  };

  constexpr std::array<segment, 4> MEMORY_SEGMENTS { {
    { "Code", 0.34f, IM_COL32(71, 125, 230, 255) },
    { "Symbols", 0.18f, IM_COL32(84, 196, 112, 255) },
    { "AST", 0.22f, IM_COL32(224, 167, 74, 255) },
    { "Scratch", 0.11f, IM_COL32(198, 108, 208, 255) },
  } };
}

void panels::draw_memory_panel()
{
  ImGui::SeparatorText("runtime footprint");

  ImVec2 start = ImGui::GetCursorScreenPos();
  float width = ImGui::GetContentRegionAvail().x;
  float height = 20.0f;
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(start, ImVec2(start.x + width, start.y + height), IM_COL32(40, 40, 45, 255), 4.0f);

  float offset = 0.0f;
  for (const segment& item : MEMORY_SEGMENTS)
  {
    float segment_width = width * item.value;
    draw_list->AddRectFilled(
      ImVec2(start.x + offset, start.y), ImVec2(start.x + offset + segment_width, start.y + height), item.color, 4.0f);
    offset += segment_width;
  }

  ImGui::Dummy(ImVec2(0.0f, height + 10.0f));

  for (const segment& item : MEMORY_SEGMENTS)
  {
    ImGui::ColorButton(item.label, ImColor(item.color).Value, ImGuiColorEditFlags_NoTooltip, ImVec2(10.0f, 10.0f));
    ImGui::SameLine();
    ImGui::Text("%s", item.label);
    ImGui::SameLine(180.0f);
    ImGui::TextDisabled("%.0f KB", 1024.0f * item.value);
  }

  ImGui::SeparatorText("allocators");
  ImGui::ProgressBar(0.72f, ImVec2(-1.0f, 0.0f), "arena 72%");
  ImGui::ProgressBar(0.39f, ImVec2(-1.0f, 0.0f), "symbol pool 39%");
}
