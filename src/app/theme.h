#pragma once

struct ImFont;

namespace app::theme
{
  void apply();
  void configure();
  ImFont* get_mono_font();
  ImFont* get_ui_font();
}
