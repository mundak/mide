#include "editor_panel.h"

// clang-format off
#include "compiler/document/editor_document_state.h"
#include "panel_common.h"

#include "imgui.h"
#include "imgui_internal.h"
// clang-format on

#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
  struct input_text_user_data
  {
    std::string* buffer;
    compiler::document::editor_document_state* document_state;
  };

  ImVec2 editor_overlay_scroll(0.0f, 0.0f);

  size_t count_digits(size_t value)
  {
    size_t digit_count = 1;
    while (value >= 10)
    {
      value /= 10;
      ++digit_count;
    }

    return digit_count;
  }

  size_t get_line_number_digits(size_t line_count)
  {
    const size_t minimum_digits = 4;
    const size_t digit_count = count_digits((line_count == 0) ? 1 : line_count);
    return (digit_count > minimum_digits) ? digit_count : minimum_digits;
  }

  ImVec4 get_token_color(compiler::document::editor_token_classification classification)
  {
    switch (classification)
    {
    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT:
      return ImVec4(0.90f, 0.90f, 0.95f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_KEYWORD:
      return ImVec4(0.51f, 0.76f, 1.00f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_TYPE:
      return ImVec4(0.63f, 0.77f, 1.00f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_FUNCTION:
      return ImVec4(0.98f, 0.84f, 0.56f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_PARAMETER:
      return ImVec4(0.57f, 0.88f, 0.70f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_LOCAL:
      return ImVec4(0.83f, 0.90f, 0.68f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_FIELD:
      return ImVec4(0.69f, 0.82f, 0.91f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_UNRESOLVED:
      return ImVec4(0.95f, 0.60f, 0.37f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_LITERAL:
      return ImVec4(0.93f, 0.72f, 0.45f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_PUNCTUATION:
      return ImVec4(0.74f, 0.78f, 0.86f, 1.00f);

    case compiler::document::EDITOR_TOKEN_CLASSIFICATION_ERROR:
      return ImVec4(0.96f, 0.43f, 0.43f, 1.00f);
    }

    return ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
  }

  int input_text_callback(ImGuiInputTextCallbackData* data)
  {
    input_text_user_data* user_data = static_cast<input_text_user_data*>(data->UserData);
    if (user_data == nullptr)
    {
      return 0;
    }

    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
      std::string* buffer = user_data->buffer;
      buffer->resize(static_cast<size_t>(data->BufTextLen));
      data->Buf = buffer->data();
      return 0;
    }

    user_data->document_state->set_cursor_offset(static_cast<size_t>(data->CursorPos));
    return 0;
  }

  float measure_text_width(const char* text_begin, const char* text_end)
  {
    if (text_end <= text_begin)
    {
      return 0.0f;
    }

    return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, text_begin, text_end, nullptr).x;
  }

  float get_gutter_width(size_t line_count, float text_padding_x)
  {
    const size_t line_number_digits = get_line_number_digits(line_count);
    char line_number_buffer[32];
    std::snprintf(
      line_number_buffer, sizeof(line_number_buffer), "%0*zu", static_cast<int>(line_number_digits), line_count);
    return measure_text_width(line_number_buffer, line_number_buffer + std::strlen(line_number_buffer))
      + (text_padding_x * 2.0f);
  }

  void draw_text_segment(
    ImDrawList* draw_list,
    const ImVec2& position,
    const ImVec4& color,
    const char* text_begin,
    const char* text_end,
    const ImVec4& clip_rect)
  {
    if (text_end <= text_begin)
    {
      return;
    }

    draw_list->AddText(
      ImGui::GetFont(),
      ImGui::GetFontSize(),
      position,
      ImGui::GetColorU32(color),
      text_begin,
      text_end,
      0.0f,
      &clip_rect);
  }

  void advance_highlight_index(
    const std::vector<compiler::document::editor_highlight_span>& highlight_spans,
    size_t line_start,
    size_t line_end,
    size_t& highlight_index)
  {
    while ((highlight_index < highlight_spans.size())
           && (highlight_spans[highlight_index].span.get_end() <= line_start))
    {
      ++highlight_index;
    }

    while ((highlight_index < highlight_spans.size()) && (highlight_spans[highlight_index].span.start < line_end))
    {
      ++highlight_index;
    }
  }

  void draw_highlighted_line(
    ImDrawList* draw_list,
    const std::string& text,
    size_t line_start,
    size_t line_end,
    const std::vector<compiler::document::editor_highlight_span>& highlight_spans,
    size_t& highlight_index,
    const ImVec2& line_position,
    const ImVec4& clip_rect)
  {
    const char* text_base = text.data();
    float cursor_x = line_position.x;
    size_t cursor_offset = line_start;

    while ((highlight_index < highlight_spans.size())
           && (highlight_spans[highlight_index].span.get_end() <= line_start))
    {
      ++highlight_index;
    }

    size_t line_highlight_index = highlight_index;
    while ((line_highlight_index < highlight_spans.size())
           && (highlight_spans[line_highlight_index].span.start < line_end))
    {
      const compiler::document::editor_highlight_span& highlight = highlight_spans[line_highlight_index];
      const size_t span_start = (highlight.span.start > line_start) ? highlight.span.start : line_start;
      const size_t span_end = (highlight.span.get_end() < line_end) ? highlight.span.get_end() : line_end;

      if (cursor_offset < span_start)
      {
        draw_text_segment(
          draw_list,
          ImVec2(cursor_x, line_position.y),
          get_token_color(compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT),
          text_base + cursor_offset,
          text_base + span_start,
          clip_rect);
        cursor_x += measure_text_width(text_base + cursor_offset, text_base + span_start);
        cursor_offset = span_start;
      }

      if (span_end > span_start)
      {
        draw_text_segment(
          draw_list,
          ImVec2(cursor_x, line_position.y),
          get_token_color(highlight.classification),
          text_base + span_start,
          text_base + span_end,
          clip_rect);
        cursor_x += measure_text_width(text_base + span_start, text_base + span_end);
        cursor_offset = span_end;
      }

      ++line_highlight_index;
    }

    if (cursor_offset < line_end)
    {
      draw_text_segment(
        draw_list,
        ImVec2(cursor_x, line_position.y),
        get_token_color(compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT),
        text_base + cursor_offset,
        text_base + line_end,
        clip_rect);
    }

    highlight_index = line_highlight_index;
  }

  void draw_editor_overlay(
    const compiler::document::editor_document_state& document_state,
    const ImRect& frame_rect,
    const ImVec2& scroll,
    float gutter_width,
    float text_padding_x,
    float frame_padding_y)
  {
    const compiler::document::document_snapshot& snapshot = document_state.get_snapshot();
    const compiler::document::line_index& index = snapshot.get_line_index();
    const std::string& text = snapshot.get_text();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float line_height = ImGui::GetTextLineHeight();
    const float content_height = index.get_line_count() * line_height + frame_padding_y * 2.0f;
    const bool has_vertical_scrollbar = content_height > frame_rect.GetHeight();
    const float clip_max_x = has_vertical_scrollbar ? (frame_rect.Max.x - style.ScrollbarSize) : frame_rect.Max.x;
    const float gutter_min_x = frame_rect.Min.x;
    const float gutter_max_x = frame_rect.Min.x + gutter_width;
    const ImVec4 clip_rect(
      gutter_max_x + text_padding_x,
      frame_rect.Min.y + frame_padding_y,
      clip_max_x - text_padding_x,
      frame_rect.Max.y - frame_padding_y);
    const ImVec2 text_origin(gutter_max_x + text_padding_x - scroll.x, frame_rect.Min.y + frame_padding_y - scroll.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    size_t highlight_index = 0;
    const size_t line_number_digits = get_line_number_digits(index.get_line_count());

    draw_list->AddRectFilled(
      ImVec2(gutter_min_x, frame_rect.Min.y), ImVec2(gutter_max_x, frame_rect.Max.y), IM_COL32(22, 26, 34, 230));
    draw_list->AddLine(
      ImVec2(gutter_max_x, frame_rect.Min.y), ImVec2(gutter_max_x, frame_rect.Max.y), IM_COL32(52, 60, 74, 255));

    draw_list->PushClipRect(frame_rect.Min, frame_rect.Max, true);

    for (size_t line_number = 1; line_number <= index.get_line_count(); ++line_number)
    {
      const size_t line_start = index.get_line_start(line_number - 1);
      const size_t line_end = index.get_line_end(line_number - 1);
      const float line_y = text_origin.y + (line_height * static_cast<float>(line_number - 1));
      const bool has_diagnostic = document_state.has_diagnostic_at_line(line_number);

      if (has_diagnostic)
      {
        const ImVec2 rect_min(gutter_min_x, line_y);
        const ImVec2 rect_max(clip_rect.z, line_y + line_height);
        draw_list->AddRectFilled(rect_min, rect_max, IM_COL32(57, 25, 25, 180));
      }

      if ((line_y + line_height) < clip_rect.y)
      {
        advance_highlight_index(document_state.get_highlight_spans(), line_start, line_end, highlight_index);
        continue;
      }

      if (line_y > clip_rect.w)
      {
        break;
      }

      char line_number_buffer[32];
      std::snprintf(
        line_number_buffer, sizeof(line_number_buffer), "%0*zu", static_cast<int>(line_number_digits), line_number);
      const ImVec4 line_number_color
        = has_diagnostic ? ImVec4(0.94f, 0.62f, 0.52f, 1.00f) : ImVec4(0.46f, 0.50f, 0.60f, 1.00f);
      const float line_number_width
        = measure_text_width(line_number_buffer, line_number_buffer + std::strlen(line_number_buffer));
      draw_text_segment(
        draw_list,
        ImVec2(gutter_max_x - text_padding_x - line_number_width, line_y),
        line_number_color,
        line_number_buffer,
        line_number_buffer + std::strlen(line_number_buffer),
        ImVec4(gutter_min_x, frame_rect.Min.y + frame_padding_y, gutter_max_x, frame_rect.Max.y - frame_padding_y));

      draw_highlighted_line(
        draw_list,
        text,
        line_start,
        line_end,
        document_state.get_highlight_spans(),
        highlight_index,
        ImVec2(text_origin.x, line_y),
        clip_rect);
    }

    draw_list->PopClipRect();
  }
}

void panels::draw_editor_panel(compiler::document::editor_document_state& document_state, ImFont* mono_font)
{
  if (ImGui::BeginTabBar("editor_tabs"))
  {
    if (ImGui::BeginTabItem("main.c"))
    {
      detail::push_mono_font(mono_font);

      input_text_user_data user_data { &document_state.get_edit_buffer(), &document_state };
      const ImVec2 editor_size = ImGui::GetContentRegionAvail();
      const ImVec2 base_frame_padding = ImGui::GetStyle().FramePadding;
      const float text_padding_x = 10.0f;
      const float gutter_width
        = get_gutter_width(document_state.get_snapshot().get_line_index().get_line_count(), text_padding_x);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(gutter_width + text_padding_x, base_frame_padding.y));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      const bool changed = ImGui::InputTextMultiline(
        "##live_source_input",
        document_state.get_edit_buffer().data(),
        document_state.get_edit_buffer().capacity() + 1,
        editor_size,
        ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackResize,
        input_text_callback,
        &user_data);
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      if (changed)
      {
        document_state.sync_from_edit_buffer();
      }

      const ImGuiID input_id = ImGui::GetItemID();
      ImGuiInputTextState* input_state = ImGui::GetInputTextState(input_id);
      if (input_state != nullptr)
      {
        editor_overlay_scroll = input_state->Scroll;
        document_state.set_cursor_offset(static_cast<size_t>(input_state->GetCursorPos()));
      }

      draw_editor_overlay(
        document_state,
        ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()),
        (input_state != nullptr) ? input_state->Scroll : editor_overlay_scroll,
        gutter_width,
        text_padding_x,
        base_frame_padding.y);

      detail::pop_mono_font(mono_font);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}
