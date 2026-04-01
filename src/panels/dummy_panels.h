#pragma once

struct ImFont;

namespace panels
{
  void draw_assembly_panel(ImFont* mono_font);
  void draw_console_panel(ImFont* mono_font);
  void draw_editor_panel(ImFont* mono_font);
  void draw_explorer_panel();
  void draw_inspector_panel();
  void draw_memory_panel();
  void draw_outline_panel();
  void draw_preview_panel(ImFont* mono_font);
  void draw_profiler_panel();
  void draw_watch_panel(ImFont* mono_font);
}
