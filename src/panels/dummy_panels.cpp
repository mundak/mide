#include "dummy_panels.h"

#include "imgui.h"

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

  struct assembly_row
  {
    const char* address;
    const char* bytes;
    const char* instruction;
    const char* notes;
    bool selected;
  };

  struct segment
  {
    const char* label;
    float value;
    ImU32 color;
  };

  constexpr std::array<source_line, 15> SOURCE_LINES
  {{
    {1, "#include <stdint.h>", ImVec4(0.63f, 0.77f, 1.00f, 1.00f), false},
    {2, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {3, "static uint32_t frame_counter = 0;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {4, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {5, "int32_t compile_step(struct unit* unit)", ImVec4(0.51f, 0.76f, 1.00f, 1.00f), false},
    {6, "{", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {7, "  if (unit == nullptr)", ImVec4(0.51f, 0.76f, 1.00f, 1.00f), false},
    {8, "  {", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {9, "    return -1;", ImVec4(0.93f, 0.72f, 0.45f, 1.00f), false},
    {10, "  }", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {11, "", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {12, "  unit->cache_state = CACHE_STATE_WARM;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), true},
    {13, "  frame_counter += unit->dirty_functions;", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
    {14, "  return emit_live_assembly(unit);", ImVec4(0.93f, 0.72f, 0.45f, 1.00f), false},
    {15, "}", ImVec4(0.90f, 0.90f, 0.95f, 1.00f), false},
  }};

  constexpr std::array<assembly_row, 7> ASSEMBLY_ROWS
  {{
    {"00007FF6`18001040", "48 89 5C 24 08", "mov [rsp+08h], rbx", "prologue", false},
    {"00007FF6`18001045", "48 83 EC 20", "sub rsp, 20h", "stack frame", false},
    {"00007FF6`18001049", "48 85 C9", "test rcx, rcx", "unit == nullptr", false},
    {"00007FF6`1800104C", "74 19", "je 00007FF6`18001067", "early exit", false},
    {"00007FF6`1800104E", "83 41 14 01", "add dword ptr [rcx+14h], 1", "cache_state", true},
    {"00007FF6`18001052", "8B 41 18", "mov eax, dword ptr [rcx+18h]", "dirty_functions", false},
    {"00007FF6`18001055", "E8 96 02 00 00", "call emit_live_assembly", "live sync", false},
  }};

  constexpr std::array<segment, 4> MEMORY_SEGMENTS
  {{
    {"Code", 0.34f, IM_COL32(71, 125, 230, 255)},
    {"Symbols", 0.18f, IM_COL32(84, 196, 112, 255)},
    {"AST", 0.22f, IM_COL32(224, 167, 74, 255)},
    {"Scratch", 0.11f, IM_COL32(198, 108, 208, 255)},
  }};

  void push_mono_font(ImFont* mono_font)
  {
    if (mono_font != nullptr)
    {
      ImGui::PushFont(mono_font);
    }
  }

  void pop_mono_font(ImFont* mono_font)
  {
    if (mono_font != nullptr)
    {
      ImGui::PopFont();
    }
  }

  void draw_stat_pair(
    const char* label,
    const char* value)
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", value);
  }

  void draw_grid_canvas()
  {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 64.0f || canvas_size.y < 64.0f)
    {
      return;
    }

    ImVec2 canvas_max(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos, canvas_max, IM_COL32(18, 21, 27, 255), 4.0f);
    draw_list->AddRectFilledMultiColor(
      canvas_pos,
      canvas_max,
      IM_COL32(22, 27, 35, 180),
      IM_COL32(22, 27, 35, 32),
      IM_COL32(12, 16, 22, 180),
      IM_COL32(12, 16, 22, 140));

    for (float x = canvas_pos.x; x < canvas_max.x; x += 16.0f)
    {
      ImU32 color = (static_cast<int32_t>(x - canvas_pos.x) % 64 == 0)
        ? IM_COL32(78, 88, 102, 96)
        : IM_COL32(53, 60, 70, 54);
      draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_max.y), color, 1.0f);
    }

    for (float y = canvas_pos.y; y < canvas_max.y; y += 16.0f)
    {
      ImU32 color = (static_cast<int32_t>(y - canvas_pos.y) % 64 == 0)
        ? IM_COL32(78, 88, 102, 96)
        : IM_COL32(53, 60, 70, 54);
      draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_max.x, y), color, 1.0f);
    }

    ImVec2 chip_min(canvas_pos.x + 24.0f, canvas_pos.y + 28.0f);
    ImVec2 chip_max(chip_min.x + 184.0f, chip_min.y + 72.0f);
    draw_list->AddRectFilled(chip_min, chip_max, IM_COL32(34, 39, 49, 240), 6.0f);
    draw_list->AddRect(chip_min, chip_max, IM_COL32(114, 146, 255, 180), 6.0f, 0, 1.0f);
    draw_list->AddText(ImVec2(chip_min.x + 14.0f, chip_min.y + 12.0f), IM_COL32(230, 235, 244, 255), "live parse graph");
    draw_list->AddText(ImVec2(chip_min.x + 14.0f, chip_min.y + 36.0f), IM_COL32(131, 212, 145, 255), "entry -> fold constants -> lower x64");

    ImVec2 chip_two_min(canvas_pos.x + canvas_size.x * 0.56f, canvas_pos.y + canvas_size.y * 0.24f);
    ImVec2 chip_two_max(chip_two_min.x + 176.0f, chip_two_min.y + 92.0f);
    draw_list->AddRectFilled(chip_two_min, chip_two_max, IM_COL32(30, 35, 44, 240), 6.0f);
    draw_list->AddRect(chip_two_min, chip_two_max, IM_COL32(104, 192, 122, 180), 6.0f, 0, 1.0f);
    draw_list->AddText(ImVec2(chip_two_min.x + 14.0f, chip_two_min.y + 12.0f), IM_COL32(233, 236, 242, 255), "watchpoint");
    draw_list->AddText(ImVec2(chip_two_min.x + 14.0f, chip_two_min.y + 38.0f), IM_COL32(255, 197, 107, 255), "frame_counter");
    draw_list->AddText(ImVec2(chip_two_min.x + 14.0f, chip_two_min.y + 60.0f), IM_COL32(164, 173, 188, 255), "write observed at compile_step:12");

    draw_list->AddBezierCubic(
      ImVec2(chip_min.x + 184.0f, chip_min.y + 48.0f),
      ImVec2(chip_min.x + 280.0f, chip_min.y + 20.0f),
      ImVec2(chip_two_min.x - 80.0f, chip_two_min.y + 20.0f),
      ImVec2(chip_two_min.x, chip_two_min.y + 38.0f),
      IM_COL32(154, 166, 185, 180),
      2.0f);

    ImGui::InvisibleButton("preview_canvas", canvas_size);
  }
}

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

void panels::draw_editor_panel(ImFont* mono_font)
{
  if (ImGui::BeginTabBar("editor_tabs"))
  {
    if (ImGui::BeginTabItem("main.c"))
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.38f, 0.90f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.21f, 0.60f, 0.42f, 0.90f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.24f, 0.66f, 0.46f, 0.90f));
      ImGui::Button("LIVE PARSE");
      ImGui::PopStyleColor(3);
      ImGui::SameLine();
      ImGui::TextDisabled("cache warm | AST + x64 in sync");
      ImGui::Separator();

      push_mono_font(mono_font);
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
      pop_mono_font(mono_font);
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

void panels::draw_preview_panel(ImFont* mono_font)
{
  if (ImGui::BeginTabBar("preview_tabs"))
  {
    if (ImGui::BeginTabItem("Live Graph"))
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.31f, 0.40f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.38f, 0.50f, 1.0f));
      ImGui::Button("AST");
      ImGui::SameLine();
      ImGui::Button("CFG");
      ImGui::SameLine();
      ImGui::Button("IR");
      ImGui::SameLine();
      ImGui::Button("REGISTERS");
      ImGui::PopStyleColor(3);
      ImGui::Separator();
      draw_grid_canvas();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Registers"))
    {
      push_mono_font(mono_font);
      ImGui::Columns(2, nullptr, false);
      ImGui::Text("rax  00000000000000B7");
      ImGui::Text("rbx  00007FF618002000");
      ImGui::Text("rcx  000000C4F84FF960");
      ImGui::Text("rdx  0000000000000012");
      ImGui::NextColumn();
      ImGui::Text("r8   0000000000000001");
      ImGui::Text("r9   00007FF618004A10");
      ImGui::Text("rip  00007FF618001052");
      ImGui::Text("rsp  000000C4F84FF8D0");
      ImGui::Columns(1);
      pop_mono_font(mono_font);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}

void panels::draw_inspector_panel()
{
  static int32_t optimization_level = 2;
  static bool emit_debug_symbols = true;
  static bool live_parse_enabled = true;

  if (ImGui::CollapsingHeader("Document", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (ImGui::BeginTable("document_table", 2, ImGuiTableFlags_SizingFixedFit))
    {
      draw_stat_pair("Path", "src/main.c");
      draw_stat_pair("Target", "x64-windows-msvc");
      draw_stat_pair("Dirty Funcs", "4");
      draw_stat_pair("Last Build", "00:00:18 ago");
      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Build", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("Live Parse", &live_parse_enabled);
    ImGui::Checkbox("Emit Debug Symbols", &emit_debug_symbols);
    ImGui::SliderInt("Optimization", &optimization_level, 0, 3, "O%d");
    ImGui::SeparatorText("pipeline");
    ImGui::TextDisabled("lexer");
    ImGui::ProgressBar(1.0f, ImVec2(-1.0f, 0.0f), "complete");
    ImGui::TextDisabled("parser");
    ImGui::ProgressBar(0.82f, ImVec2(-1.0f, 0.0f), "rebuilding");
    ImGui::TextDisabled("x64 emitter");
    ImGui::ProgressBar(0.63f, ImVec2(-1.0f, 0.0f), "queued");
  }

  if (ImGui::CollapsingHeader("Cursor", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (ImGui::BeginTable("cursor_table", 2, ImGuiTableFlags_SizingFixedFit))
    {
      draw_stat_pair("Function", "compile_step");
      draw_stat_pair("Source Line", "12");
      draw_stat_pair("Assembly", "00007FF6`1800104E");
      draw_stat_pair("Registers", "rax, rcx");
      ImGui::EndTable();
    }
  }
}

void panels::draw_watch_panel(ImFont* mono_font)
{
  push_mono_font(mono_font);
  if (ImGui::BeginTable("watch_table", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("name");
    ImGui::TableSetupColumn("value");
    ImGui::TableSetupColumn("type");
    ImGui::TableHeadersRow();

    draw_stat_pair("frame_counter", "0x000000B7");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("uint32_t");

    draw_stat_pair("unit->dirty_functions", "4");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("int32_t");

    draw_stat_pair("unit->cache_state", "CACHE_STATE_WARM");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("enum");

    ImGui::EndTable();
  }
  pop_mono_font(mono_font);
}

void panels::draw_assembly_panel(ImFont* mono_font)
{
  push_mono_font(mono_font);
  if (ImGui::BeginTable(
    "assembly_table",
    4,
    ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
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
  pop_mono_font(mono_font);
}

void panels::draw_console_panel(ImFont* mono_font)
{
  ImGui::Button("Clear");
  ImGui::SameLine();
  ImGui::Button("Copy");
  ImGui::SameLine();
  ImGui::TextDisabled("auto-scroll on");
  ImGui::Separator();

  push_mono_font(mono_font);
  ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.53f, 1.00f), "[parser]    reduced translation unit in 2.8 ms");
  ImGui::TextColored(ImVec4(0.51f, 0.76f, 1.00f, 1.00f), "[watch]     frame_counter write observed at line 12");
  ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.45f, 1.00f), "[x64]       re-emitted 7 instructions for compile_step");
  ImGui::TextColored(ImVec4(0.75f, 0.79f, 0.88f, 1.00f), "[debugger]  paused before emit_live_assembly()");
  pop_mono_font(mono_font);
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
      ImVec2(start.x + offset, start.y),
      ImVec2(start.x + offset + segment_width, start.y + height),
      item.color,
      4.0f);
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

void panels::draw_profiler_panel()
{
  ImGui::TextDisabled("compile timeline");
  ImGui::Separator();
  ImGui::Text("front-end");
  ImGui::ProgressBar(0.84f, ImVec2(-1.0f, 0.0f), "6.4 ms");
  ImGui::Text("assembly mapping");
  ImGui::ProgressBar(0.58f, ImVec2(-1.0f, 0.0f), "4.1 ms");
  ImGui::Text("debug metadata");
  ImGui::ProgressBar(0.31f, ImVec2(-1.0f, 0.0f), "1.7 ms");
  ImGui::SeparatorText("hot path");
  ImGui::BulletText("emit_live_assembly()");
  ImGui::BulletText("symbol patching");
  ImGui::BulletText("watchpoint reconciliation");
}
