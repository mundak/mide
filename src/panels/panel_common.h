#pragma once

struct ImFont;

namespace panels::detail
{
  void push_mono_font(ImFont* mono_font);
  void pop_mono_font(ImFont* mono_font);
  void draw_stat_pair(const char* label, const char* value);
}
