#pragma once

#include "compiler/document/document_snapshot.h"
#include "compiler/syntax/source_span.h"
#include "compiler/syntax/syntax_diagnostic.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace compiler
{
  namespace document
  {
    enum editor_token_classification : uint32_t
    {
      EDITOR_TOKEN_CLASSIFICATION_DEFAULT,
      EDITOR_TOKEN_CLASSIFICATION_KEYWORD,
      EDITOR_TOKEN_CLASSIFICATION_TYPE,
      EDITOR_TOKEN_CLASSIFICATION_FUNCTION,
      EDITOR_TOKEN_CLASSIFICATION_LITERAL,
      EDITOR_TOKEN_CLASSIFICATION_PUNCTUATION,
      EDITOR_TOKEN_CLASSIFICATION_ERROR,
    };

    struct editor_highlight_span
    {
      syntax::source_span span;
      editor_token_classification classification;
    };

    struct editor_outline_item
    {
      std::string label;
      size_t line_number;
      syntax::source_span span;
      bool recovered;
    };

    struct editor_diagnostic_item
    {
      syntax::syntax_diagnostic_code code;
      syntax::source_span span;
      std::string message;
      size_t line_number;
      bool lexical;
    };

    class editor_document_state
    {
    public:
      editor_document_state();
      explicit editor_document_state(document_snapshot snapshot);

      [[nodiscard]] static editor_document_state create_initial(std::string text);

      [[nodiscard]] size_t get_cursor_column() const;
      [[nodiscard]] size_t get_cursor_line() const;
      [[nodiscard]] size_t get_diagnostic_count() const;
      [[nodiscard]] const std::vector<editor_diagnostic_item>& get_diagnostics() const;
      [[nodiscard]] std::string& get_edit_buffer();
      [[nodiscard]] const std::string& get_edit_buffer() const;
      [[nodiscard]] uint64_t get_generation() const;
      [[nodiscard]] const std::vector<editor_highlight_span>& get_highlight_spans() const;
      [[nodiscard]] const std::vector<editor_outline_item>& get_outline_items() const;
      [[nodiscard]] const document_snapshot& get_snapshot() const;
      [[nodiscard]] bool has_diagnostic_at_line(size_t line_number) const;
      [[nodiscard]] bool has_diagnostics() const;

      void replace_text(std::string text);
      void set_cursor_offset(size_t offset);
      void sync_from_edit_buffer();

    private:
      void rebuild_derived_state();

      document_snapshot m_snapshot;
      std::string m_edit_buffer;
      size_t m_cursor_offset;
      std::optional<std::string> m_frontend_failure_message;
      size_t m_frontend_failure_offset;
      std::vector<editor_highlight_span> m_highlight_spans;
      std::vector<editor_outline_item> m_outline_items;
      std::vector<editor_diagnostic_item> m_diagnostics;
      std::vector<bool> m_lines_with_diagnostics;
    };
  }
}
