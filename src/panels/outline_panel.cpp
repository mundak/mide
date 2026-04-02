#include "outline_panel.h"

#include "imgui.h"

void panels::draw_outline_panel()
{
  ImGui::SeparatorText("symbols");
  ImGui::BulletText("compile_step");
  ImGui::BulletText("emit_live_assembly");
  ImGui::BulletText("CACHE_STATE_WARM");
  ImGui::BulletText("frame_counter");

  ImGui::SeparatorText("diagnostics");
  ImGui::TextColored(ImVec4(0.90f, 0.66f, 0.35f, 1.00f), "warning");
  ImGui::SameLine();
  ImGui::TextUnformatted("possible narrowing conversion in ir_lowering.c:118");
  ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.53f, 1.00f), "note");
  ImGui::SameLine();
  ImGui::TextUnformatted("live assembly synchronized 18 ms ago");
}
