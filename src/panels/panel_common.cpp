#include "panel_common.h"

#include "imgui.h"

void panels::detail::push_mono_font(ImFont* mono_font)
{
  if (mono_font != nullptr)
  {
    ImGui::PushFont(mono_font);
  }
}

void panels::detail::pop_mono_font(ImFont* mono_font)
{
  if (mono_font != nullptr)
  {
    ImGui::PopFont();
  }
}

void panels::detail::draw_stat_pair(const char* label, const char* value)
{
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::TextDisabled("%s", label);
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%s", value);
}
