#include "watch_panel.h"

#include "imgui.h"
#include "panel_common.h"

void panels::draw_watch_panel(ImFont* mono_font)
{
  detail::push_mono_font(mono_font);
  if (ImGui::BeginTable("watch_table", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("name");
    ImGui::TableSetupColumn("value");
    ImGui::TableSetupColumn("type");
    ImGui::TableHeadersRow();

    detail::draw_stat_pair("frame_counter", "0x000000B7");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("uint32_t");

    detail::draw_stat_pair("unit->dirty_functions", "4");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("int32_t");

    detail::draw_stat_pair("unit->cache_state", "CACHE_STATE_WARM");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("enum");

    ImGui::EndTable();
  }
  detail::pop_mono_font(mono_font);
}
