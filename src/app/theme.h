#pragma once

struct ImFont;

namespace app::theme
{
  void apply(float scale = 1.0f);
  void configure();
  ImFont* get_mono_font();
  ImFont* get_ui_font();
  float get_scale();
}
