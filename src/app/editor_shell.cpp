#include "editor_shell.h"

// clang-format off
#include "panels/dummy_panels.h"
#include "theme.h"

#include "imgui_internal.h"
// clang-format on

#include <cmath>

namespace
{
  float get_accent_bar_height()
  {
    return std::ceil(ImGui::GetFontSize() * 0.18f);
  }

  ImVec2 get_chrome_padding()
  {
    const ImGuiStyle& style = ImGui::GetStyle();
    return ImVec2(style.WindowPadding.x, style.FramePadding.y);
  }

  float get_top_bar_height()
  {
    const ImGuiStyle& style = ImGui::GetStyle();
    return std::ceil(ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f + style.WindowPadding.y * 2.0f);
  }

  float get_status_bar_height()
  {
    const ImGuiStyle& style = ImGui::GetStyle();
    return std::ceil(ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0f + style.FramePadding.y);
  }

  float get_top_bar_trailing_width()
  {
    return std::ceil(ImGui::GetFontSize() * 21.0f);
  }

  float get_status_bar_trailing_width()
  {
    return std::ceil(ImGui::GetFontSize() * 22.0f);
  }

  void draw_chip(const char* label, const ImVec4& color)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::Button(label);
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(3);
  }
}

app::editor_shell::editor_shell()
  : m_layout_initialized(false)
  , m_profiler_window_initialized(false)
{
}

void app::editor_shell::draw()
{
  draw_top_bar();
  draw_status_bar();
  draw_dockspace();
  draw_panel_windows();
}

void app::editor_shell::draw_top_bar()
{
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float top_bar_height = get_top_bar_height();

  ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, top_bar_height), ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, get_chrome_padding());
  ImGui::Begin("TOP_BAR", nullptr, flags);
  ImGui::PopStyleVar(2);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 min = ImGui::GetWindowPos();
  ImVec2 max = ImVec2(min.x + ImGui::GetWindowWidth(), min.y + get_accent_bar_height());
  draw_list->AddRectFilled(min, max, IM_COL32(96, 150, 255, 255));

  if (ImGui::BeginMenuBar())
  {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.97f, 1.0f, 1.0f));
    ImGui::TextUnformatted("mIDE");
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::SeparatorText("prototype");

    if (ImGui::BeginMenu("File"))
    {
      ImGui::MenuItem("New Translation Unit", nullptr, false, false);
      ImGui::MenuItem("Open Workspace", nullptr, false, false);
      ImGui::MenuItem("Save Snapshot", nullptr, false, false);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Build"))
    {
      ImGui::MenuItem("Parse Continuously", nullptr, true, false);
      ImGui::MenuItem("Build", "F7", false, false);
      ImGui::MenuItem("Run", "Ctrl+F5", false, false);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
      ImGui::MenuItem("Reset Layout", nullptr, false, false);
      ImGui::MenuItem("Show Assembly", nullptr, true, false);
      ImGui::MenuItem("Show Registers", nullptr, true, false);
      ImGui::EndMenu();
    }

    float trailing_width = get_top_bar_trailing_width();
    float cursor_x = ImGui::GetWindowContentRegionMax().x - trailing_width;
    if (cursor_x > ImGui::GetCursorPosX())
    {
      ImGui::SetCursorPosX(cursor_x);
    }

    draw_chip("Debug", ImVec4(0.36f, 0.52f, 0.96f, 0.85f));
    ImGui::SameLine();
    draw_chip("x64", ImVec4(0.22f, 0.24f, 0.31f, 1.0f));
    ImGui::SameLine();
    draw_chip("Docking + Viewports", ImVec4(0.18f, 0.55f, 0.38f, 0.85f));

    ImGui::EndMenuBar();
  }

  ImGui::End();
}

void app::editor_shell::draw_status_bar()
{
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float status_bar_height = get_status_bar_height();

  ImGui::SetNextWindowPos(
    ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - status_bar_height), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, status_bar_height), ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoTitleBar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, get_chrome_padding());
  ImGui::Begin("STATUS_BAR", nullptr, flags);
  ImGui::PopStyleVar(2);

  float frame_time = 1000.0f / ImGui::GetIO().Framerate;
  if (!std::isfinite(frame_time))
  {
    frame_time = 0.0f;
  }

  ImGui::TextColored(ImVec4(0.51f, 0.76f, 1.0f, 1.0f), "READY");
  ImGui::SameLine();
  ImGui::TextDisabled("main.c");
  ImGui::SameLine();
  ImGui::TextDisabled("Ln 42, Col 18");
  ImGui::SameLine();
  ImGui::TextDisabled("clang frontend: warm");

  float right_edge = ImGui::GetWindowContentRegionMax().x - get_status_bar_trailing_width();
  if (right_edge > ImGui::GetCursorPosX())
  {
    ImGui::SetCursorPosX(right_edge);
  }

  ImGui::TextDisabled("Tabs can be dragged into native windows");
  ImGui::SameLine();
  ImGui::Text("%.1f FPS | %.2f ms", ImGui::GetIO().Framerate, frame_time);

  ImGui::End();
}

void app::editor_shell::draw_dockspace()
{
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float top_bar_height = get_top_bar_height();
  const float status_bar_height = get_status_bar_height();

  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + top_bar_height), ImGuiCond_Always);
  ImGui::SetNextWindowSize(
    ImVec2(viewport->Size.x, viewport->Size.y - top_bar_height - status_bar_height), ImGuiCond_Always);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::Begin("MAIN_DOCK", nullptr, flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("MAIN_DOCKSPACE");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
  ensure_layout(dockspace_id);

  ImGui::End();
}

void app::editor_shell::ensure_layout(ImGuiID dockspace_id)
{
  if (m_layout_initialized)
  {
    return;
  }

  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

  ImGuiID dock_left_id = 0;
  ImGuiID dock_right_id = 0;
  ImGuiID dock_bottom_id = 0;
  ImGuiID dock_center_id = dockspace_id;

  dock_left_id = ImGui::DockBuilderSplitNode(dock_center_id, ImGuiDir_Left, 0.19f, nullptr, &dock_center_id);
  dock_right_id = ImGui::DockBuilderSplitNode(dock_center_id, ImGuiDir_Right, 0.25f, nullptr, &dock_center_id);
  dock_bottom_id = ImGui::DockBuilderSplitNode(dock_center_id, ImGuiDir_Down, 0.30f, nullptr, &dock_center_id);

  ImGui::DockBuilderDockWindow("Explorer", dock_left_id);
  ImGui::DockBuilderDockWindow("Outline", dock_left_id);

  ImGui::DockBuilderDockWindow("Editor", dock_center_id);

  ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
  ImGui::DockBuilderDockWindow("Watch", dock_right_id);

  ImGui::DockBuilderDockWindow("Assembly", dock_bottom_id);
  ImGui::DockBuilderDockWindow("Console", dock_bottom_id);
  ImGui::DockBuilderDockWindow("Memory", dock_bottom_id);

  ImGui::DockBuilderFinish(dockspace_id);
  m_layout_initialized = true;
}

void app::editor_shell::draw_panel_windows()
{
  ImFont* mono_font = app::theme::get_mono_font();

  ImGui::Begin("Explorer");
  panels::draw_explorer_panel();
  ImGui::End();

  ImGui::Begin("Outline");
  panels::draw_outline_panel();
  ImGui::End();

  ImGui::Begin("Editor");
  panels::draw_editor_panel(mono_font);
  ImGui::End();

  ImGui::Begin("Inspector");
  panels::draw_inspector_panel();
  ImGui::End();

  ImGui::Begin("Watch");
  panels::draw_watch_panel(mono_font);
  ImGui::End();

  ImGui::Begin("Assembly");
  panels::draw_assembly_panel(mono_font);
  ImGui::End();

  ImGui::Begin("Console");
  panels::draw_console_panel(mono_font);
  ImGui::End();

  ImGui::Begin("Memory");
  panels::draw_memory_panel();
  ImGui::End();

  if (!m_profiler_window_initialized)
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float font_size = ImGui::GetFontSize();
    const float frame_height = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x + viewport->Size.x - font_size * 22.5f, viewport->Pos.y + frame_height * 3.0f),
      ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(font_size * 20.0f, frame_height * 8.0f), ImGuiCond_Once);
    m_profiler_window_initialized = true;
  }

  ImGui::Begin("Profiler");
  panels::draw_profiler_panel();
  ImGui::End();
}
