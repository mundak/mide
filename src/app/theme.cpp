#include "theme.h"

#include "imgui.h"

#include <array>
#include <filesystem>

namespace app::theme
{
  namespace
  {
    constexpr float BASE_UI_FONT_SIZE = 15.0f;
    constexpr float BASE_MONO_FONT_SIZE = 14.0f;

    float current_scale = 1.0f;
    ImFont* ui_font = nullptr;
    ImFont* mono_font = nullptr;

    ImFont* try_load_font(const std::array<const char*, 4>& candidates, float size_pixels)
    {
      ImGuiIO& io = ImGui::GetIO();

      for (const char* candidate : candidates)
      {
        if (!std::filesystem::exists(candidate))
        {
          continue;
        }

        ImFont* font = io.Fonts->AddFontFromFileTTF(candidate, size_pixels);
        if (font != nullptr)
        {
          return font;
        }
      }

      return nullptr;
    }

    float sanitize_scale(float scale)
    {
      if (scale <= 0.0f)
      {
        return 1.0f;
      }

      return (scale > 1.0f) ? scale : 1.0f;
    }
  }

  void configure()
  {
    ImGuiIO& io = ImGui::GetIO();

    ui_font = try_load_font(
      {
        "C:/Windows/Fonts/bahnschrift.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/trebuc.ttf",
        "C:/Windows/Fonts/arial.ttf",
      },
      BASE_UI_FONT_SIZE);

    mono_font = try_load_font(
      {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cascadiamono.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/lucon.ttf",
      },
      BASE_MONO_FONT_SIZE);

    if (ui_font == nullptr)
    {
      ui_font = io.Fonts->AddFontDefault();
    }

    if (mono_font == nullptr)
    {
      mono_font = ui_font;
    }

    io.FontDefault = ui_font;
  }

  void apply(float scale)
  {
    current_scale = sanitize_scale(scale);

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();

    ImVec4* colors = style.Colors;
    const ImVec4 highlight(0.50f, 0.70f, 1.00f, 1.00f);

    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.13f, 0.98f);

    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.28f, 0.38f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.27f, 0.34f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.31f, 0.40f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = highlight;

    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);

    colors[ImGuiCol_Border] = ImVec4(0.21f, 0.22f, 0.27f, 0.70f);
    colors[ImGuiCol_Separator] = ImVec4(0.21f, 0.22f, 0.27f, 0.90f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.45f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.58f, 0.78f, 1.00f, 0.70f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.66f, 0.86f, 1.00f, 0.92f);

    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.58f, 1.00f);
    colors[ImGuiCol_CheckMark] = highlight;
    colors[ImGuiCol_SliderGrab] = highlight;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.48f, 0.88f, 0.65f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = highlight;

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.26f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.33f, 0.35f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.43f, 0.51f, 1.00f);

    colors[ImGuiCol_TableBorderLight] = ImVec4(0.18f, 0.19f, 0.24f, 0.85f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.21f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09f, 0.10f, 0.13f, 0.45f);

    style.WindowRounding = 3.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = 0.0f;
    style.TabBarOverlineSize = 2.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 5.0f);
    style.PopupBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.SeparatorTextBorderSize = 1.0f;
    style.SeparatorTextAlign = ImVec2(0.0f, 0.5f);
    style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);

    style.ScaleAllSizes(current_scale);
    style.FontScaleMain = 1.0f;
    style.FontScaleDpi = current_scale;
  }

  float get_scale()
  {
    return current_scale;
  }

  ImFont* get_mono_font()
  {
    return mono_font;
  }

  ImFont* get_ui_font()
  {
    return ui_font;
  }
}
