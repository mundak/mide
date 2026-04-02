#include "console_panel.h"

#include "imgui.h"
#include "panel_common.h"

void panels::draw_console_panel(ImFont* mono_font)
{
  ImGui::Button("Clear");
  ImGui::SameLine();
  ImGui::Button("Copy");
  ImGui::SameLine();
  ImGui::TextDisabled("auto-scroll on");
  ImGui::Separator();

  detail::push_mono_font(mono_font);
  ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.53f, 1.00f), "[parser]    reduced translation unit in 2.8 ms");
  ImGui::TextColored(ImVec4(0.51f, 0.76f, 1.00f, 1.00f), "[watch]     frame_counter write observed at line 12");
  ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.45f, 1.00f), "[x64]       re-emitted 7 instructions for compile_step");
  ImGui::TextColored(ImVec4(0.75f, 0.79f, 0.88f, 1.00f), "[debugger]  paused before emit_live_assembly()");
  detail::pop_mono_font(mono_font);
}
