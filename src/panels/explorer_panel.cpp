#include "explorer_panel.h"

#include "imgui.h"

void panels::draw_explorer_panel()
{
  static char filter[32] = "";

  ImGui::InputTextWithHint("##filter", "filter workspace", filter, IM_ARRAYSIZE(filter));
  ImGui::SeparatorText("workspace");

  if (ImGui::TreeNodeEx("src", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (ImGui::TreeNodeEx("compiler", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Selectable("lexer.c");
      ImGui::Selectable("parser.c");
      ImGui::Selectable("ir_lowering.c");
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("runtime", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Selectable("thread.c");
      ImGui::Selectable("arena.c");
      ImGui::TreePop();
    }

    ImGui::Selectable("main.c", true);
    ImGui::Selectable("debug_symbols.c");
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("tests", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Selectable("parser_smoke.c");
    ImGui::Selectable("x64_emit_regressions.c");
    ImGui::TreePop();
  }

  ImGui::SeparatorText("recent sessions");
  ImGui::BulletText("boot sequence / x64 startup");
  ImGui::BulletText("expression folding benchmark");
  ImGui::BulletText("watchpoint: frame_counter");
}
