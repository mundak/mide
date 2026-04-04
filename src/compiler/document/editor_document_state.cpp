#include "editor_document_state.h"

#include "compiler/document/text_change.h"
#include "compiler/semantic/semantic_model.h"
#include "compiler/syntax/syntax_kind.h"
#include "compiler/syntax/syntax_node.h"
#include "compiler/syntax/syntax_token.h"
#include "compiler/syntax/syntax_tree.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
  using compiler::document::editor_diagnostic_item;
  using compiler::document::editor_highlight_span;
  using compiler::document::editor_outline_item;
  using compiler::document::editor_token_classification;
  using compiler::document::text_change;
  using compiler::syntax::source_span;
  using compiler::syntax::syntax_child_reference;
  using compiler::syntax::syntax_kind;
  using compiler::syntax::syntax_node;
  using compiler::syntax::syntax_token;
  using compiler::syntax::syntax_tree;

  std::string create_default_source_text()
  {
    return "int main()\n{\n  return 1;\n}\n";
  }

  size_t clamp_offset(size_t offset, size_t text_size)
  {
    return (offset > text_size) ? text_size : offset;
  }

  std::optional<text_change> build_text_change(const std::string& old_text, const std::string& new_text)
  {
    if (old_text == new_text)
    {
      return std::nullopt;
    }

    size_t prefix_length = 0;
    while ((prefix_length < old_text.size()) && (prefix_length < new_text.size())
           && (old_text[prefix_length] == new_text[prefix_length]))
    {
      ++prefix_length;
    }

    size_t old_suffix = old_text.size();
    size_t new_suffix = new_text.size();
    while ((old_suffix > prefix_length) && (new_suffix > prefix_length)
           && (old_text[old_suffix - 1] == new_text[new_suffix - 1]))
    {
      --old_suffix;
      --new_suffix;
    }

    return text_change {
      prefix_length,
      old_suffix,
      new_text.substr(prefix_length, new_suffix - prefix_length),
    };
  }

  std::string get_text_for_span(const std::string& text, source_span span)
  {
    const size_t end_offset = span.get_end();
    if ((span.start > text.size()) || (end_offset > text.size()))
    {
      throw std::out_of_range("source span outside document text");
    }

    return text.substr(span.start, span.length);
  }

  bool span_contains_inclusive(const source_span& span, size_t offset)
  {
    if (span.length == 0)
    {
      return offset == span.start;
    }

    return (offset >= span.start) && (offset <= (span.get_end() - 1));
  }

  bool spans_overlap(const source_span& left, const source_span& right)
  {
    if (left.length == 0)
    {
      return span_contains_inclusive(right, left.start);
    }

    if (right.length == 0)
    {
      return span_contains_inclusive(left, right.start);
    }

    return (left.start < right.get_end()) && (right.start < left.get_end());
  }

  bool subtree_is_recovered(const syntax_tree& tree, size_t node_index)
  {
    const syntax_node& node = tree.get_node(node_index);
    if (
      (node.kind == compiler::syntax::SYNTAX_KIND_ERROR) || (node.kind == compiler::syntax::SYNTAX_KIND_MISSING)
      || compiler::syntax::has_flag(node.flags, compiler::syntax::SYNTAX_NODE_FLAG_MISSING)
      || compiler::syntax::has_flag(node.flags, compiler::syntax::SYNTAX_NODE_FLAG_RECOVERED))
    {
      return true;
    }

    for (const syntax_child_reference& child : node.children)
    {
      if (child.is_node())
      {
        if (subtree_is_recovered(tree, child.index))
        {
          return true;
        }

        continue;
      }

      if (tree.get_token(child.index).malformed)
      {
        return true;
      }
    }

    return false;
  }

  std::optional<size_t> find_first_token_index(const syntax_tree& tree, size_t node_index, syntax_kind token_kind)
  {
    const syntax_node& node = tree.get_node(node_index);
    for (const syntax_child_reference& child : node.children)
    {
      if (child.is_token())
      {
        if (tree.get_token(child.index).kind == token_kind)
        {
          return child.index;
        }

        continue;
      }

      const std::optional<size_t> token_index = find_first_token_index(tree, child.index, token_kind);
      if (token_index.has_value())
      {
        return token_index;
      }
    }

    return std::nullopt;
  }

  bool token_has_diagnostic(
    const syntax_token& token, const std::vector<compiler::syntax::syntax_diagnostic>& diagnostics)
  {
    for (const compiler::syntax::syntax_diagnostic& diagnostic : diagnostics)
    {
      if (spans_overlap(token.span, diagnostic.span))
      {
        return true;
      }
    }

    return false;
  }

  bool token_is_in_recovered_region(const syntax_tree& tree, const syntax_token& token)
  {
    std::optional<size_t> parent_index = token.parent_index;
    while (parent_index.has_value())
    {
      const syntax_node& parent = tree.get_node(*parent_index);
      if (
        (parent.kind == compiler::syntax::SYNTAX_KIND_ERROR) || (parent.kind == compiler::syntax::SYNTAX_KIND_MISSING)
        || compiler::syntax::has_flag(parent.flags, compiler::syntax::SYNTAX_NODE_FLAG_MISSING)
        || compiler::syntax::has_flag(parent.flags, compiler::syntax::SYNTAX_NODE_FLAG_RECOVERED))
      {
        return true;
      }

      parent_index = parent.parent_index;
    }

    return false;
  }

  editor_token_classification classify_semantic_token(
    const compiler::semantic::semantic_model& semantic_model, const syntax_token& token)
  {
    switch (semantic_model.get_token_classification(token))
    {
    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_NONE:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_TYPE:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_TYPE;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_FUNCTION:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_FUNCTION;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_PARAMETER:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_PARAMETER;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_LOCAL:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_LOCAL;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_FIELD:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_FIELD;

    case compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_UNRESOLVED:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_UNRESOLVED;
    }

    throw std::invalid_argument("semantic token classification");
  }

  editor_token_classification classify_token(
    const syntax_tree& tree,
    const syntax_token& token,
    const std::vector<compiler::syntax::syntax_diagnostic>& diagnostics,
    const std::shared_ptr<const compiler::semantic::semantic_model>& semantic_model)
  {
    if (token.malformed || token_has_diagnostic(token, diagnostics) || token_is_in_recovered_region(tree, token))
    {
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_ERROR;
    }

    switch (token.kind)
    {
    case compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN:
      if (semantic_model != nullptr)
      {
        const editor_token_classification classification = classify_semantic_token(*semantic_model, token);
        if (classification != compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT)
        {
          return classification;
        }
      }

      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT;

    case compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_LITERAL;

    case compiler::syntax::SYNTAX_KIND_INT_KEYWORD:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_TYPE;

    case compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_KEYWORD;

    case compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN:
    case compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN:
    case compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN:
    case compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN:
    case compiler::syntax::SYNTAX_KIND_COMMA_TOKEN:
    case compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_PUNCTUATION;

    case compiler::syntax::SYNTAX_KIND_BAD_TOKEN:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_ERROR;

    case compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN:
      return compiler::document::EDITOR_TOKEN_CLASSIFICATION_DEFAULT;

    case compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT:
    case compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION:
    case compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR:
    case compiler::syntax::SYNTAX_KIND_PARAMETER_LIST:
    case compiler::syntax::SYNTAX_KIND_PARAMETER_DECLARATION:
    case compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT:
    case compiler::syntax::SYNTAX_KIND_DECLARATION_STATEMENT:
    case compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT:
    case compiler::syntax::SYNTAX_KIND_IDENTIFIER:
    case compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER:
    case compiler::syntax::SYNTAX_KIND_IDENTIFIER_EXPRESSION:
    case compiler::syntax::SYNTAX_KIND_LITERAL_EXPRESSION:
    case compiler::syntax::SYNTAX_KIND_MISSING:
    case compiler::syntax::SYNTAX_KIND_ERROR:
      break;
    }

    throw std::invalid_argument("unexpected syntax_kind while classifying token");
  }
}

compiler::document::editor_document_state::editor_document_state()
  : editor_document_state(document_snapshot::create_initial(create_default_source_text()))
{
}

compiler::document::editor_document_state::editor_document_state(document_snapshot snapshot)
  : m_snapshot(std::move(snapshot))
  , m_edit_buffer(m_snapshot.get_text())
  , m_cursor_offset(0)
  , m_frontend_failure_message(std::nullopt)
  , m_frontend_failure_offset(0)
{
  rebuild_derived_state();
}

compiler::document::editor_document_state compiler::document::editor_document_state::create_initial(std::string text)
{
  return editor_document_state(document_snapshot::create_initial(std::move(text)));
}

size_t compiler::document::editor_document_state::get_cursor_column() const
{
  const size_t cursor_offset = clamp_offset(m_cursor_offset, m_snapshot.get_text_size());
  const line_index& index = m_snapshot.get_line_index();
  const size_t line_number = index.get_line_index(cursor_offset);
  return (cursor_offset - index.get_line_start(line_number)) + 1;
}

size_t compiler::document::editor_document_state::get_cursor_line() const
{
  const size_t cursor_offset = clamp_offset(m_cursor_offset, m_snapshot.get_text_size());
  return m_snapshot.get_line_index().get_line_index(cursor_offset) + 1;
}

size_t compiler::document::editor_document_state::get_diagnostic_count() const
{
  return m_diagnostics.size();
}

const std::vector<editor_diagnostic_item>& compiler::document::editor_document_state::get_diagnostics() const
{
  return m_diagnostics;
}

std::string& compiler::document::editor_document_state::get_edit_buffer()
{
  return m_edit_buffer;
}

const std::string& compiler::document::editor_document_state::get_edit_buffer() const
{
  return m_edit_buffer;
}

uint64_t compiler::document::editor_document_state::get_generation() const
{
  return m_snapshot.get_generation();
}

const std::vector<editor_highlight_span>& compiler::document::editor_document_state::get_highlight_spans() const
{
  return m_highlight_spans;
}

const std::vector<editor_outline_item>& compiler::document::editor_document_state::get_outline_items() const
{
  return m_outline_items;
}

const compiler::document::document_snapshot& compiler::document::editor_document_state::get_snapshot() const
{
  return m_snapshot;
}

bool compiler::document::editor_document_state::has_diagnostic_at_line(size_t line_number) const
{
  if ((line_number == 0) || (line_number > m_lines_with_diagnostics.size()))
  {
    return false;
  }

  return m_lines_with_diagnostics[line_number - 1];
}

bool compiler::document::editor_document_state::has_diagnostics() const
{
  return !m_diagnostics.empty();
}

void compiler::document::editor_document_state::replace_text(std::string text)
{
  m_edit_buffer = std::move(text);
  sync_from_edit_buffer();
}

void compiler::document::editor_document_state::set_cursor_offset(size_t offset)
{
  m_cursor_offset = clamp_offset(offset, m_edit_buffer.size());
}

void compiler::document::editor_document_state::sync_from_edit_buffer()
{
  const std::optional<text_change> change = build_text_change(m_snapshot.get_text(), m_edit_buffer);
  if (!change.has_value())
  {
    m_cursor_offset = clamp_offset(m_cursor_offset, m_edit_buffer.size());
    return;
  }

  try
  {
    m_snapshot = m_snapshot.apply_change(*change);
    m_frontend_failure_message.reset();
  }
  catch (const std::exception& exception)
  {
    const uint64_t next_generation = m_snapshot.get_generation() + 1;
    m_snapshot
      = document_snapshot(next_generation, m_edit_buffer, compiler::syntax::syntax_tree::create_empty(next_generation));
    m_frontend_failure_message = std::string("frontend update failed: ") + exception.what();
    m_frontend_failure_offset = clamp_offset(change->start_offset, m_edit_buffer.size());
  }

  m_cursor_offset = clamp_offset(m_cursor_offset, m_snapshot.get_text_size());
  rebuild_derived_state();
}

void compiler::document::editor_document_state::rebuild_derived_state()
{
  m_highlight_spans.clear();
  m_outline_items.clear();
  m_diagnostics.clear();
  m_lines_with_diagnostics.assign(m_snapshot.get_line_index().get_line_count(), false);

  const std::shared_ptr<const syntax_tree> tree = m_snapshot.get_syntax_tree();
  if (tree == nullptr)
  {
    return;
  }

  const std::vector<compiler::syntax::syntax_diagnostic>& tree_diagnostics = tree->get_diagnostics();
  const std::shared_ptr<const compiler::semantic::semantic_model> semantic_model = m_snapshot.get_semantic_model();
  for (const compiler::syntax::syntax_diagnostic& diagnostic : tree_diagnostics)
  {
    const size_t start_offset = clamp_offset(diagnostic.span.start, m_snapshot.get_text_size());
    const size_t end_offset = (diagnostic.span.length == 0)
      ? start_offset
      : clamp_offset(diagnostic.span.get_end() - 1, m_snapshot.get_text_size());
    const size_t start_line = m_snapshot.get_line_index().get_line_index(start_offset);
    const size_t end_line = m_snapshot.get_line_index().get_line_index(end_offset);

    for (size_t line_index = start_line; line_index <= end_line; ++line_index)
    {
      m_lines_with_diagnostics[line_index] = true;
    }

    m_diagnostics.push_back(editor_diagnostic_item {
      diagnostic.code,
      diagnostic.span,
      diagnostic.message,
      start_line + 1,
      compiler::syntax::is_lexical_diagnostic_code(diagnostic.code),
    });
  }

  if (m_frontend_failure_message.has_value())
  {
    const size_t failure_offset = clamp_offset(m_frontend_failure_offset, m_snapshot.get_text_size());
    const size_t failure_line = m_snapshot.get_line_index().get_line_index(failure_offset);
    m_lines_with_diagnostics[failure_line] = true;
    m_diagnostics.push_back(editor_diagnostic_item {
      compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_INTERNAL_ERROR,
      source_span { failure_offset, 0 },
      *m_frontend_failure_message,
      failure_line + 1,
      false,
    });
  }

  for (const syntax_token& token : tree->get_tokens())
  {
    if (token.kind == compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN)
    {
      continue;
    }

    m_highlight_spans.push_back(editor_highlight_span {
      token.span,
      classify_token(*tree, token, tree_diagnostics, semantic_model),
    });
  }

  if (!tree->has_root())
  {
    return;
  }

  const syntax_node& root = tree->get_root();
  for (const syntax_child_reference& child : root.children)
  {
    if (!child.is_node())
    {
      continue;
    }

    const syntax_node& node = tree->get_node(child.index);
    if (node.kind != compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION)
    {
      continue;
    }

    const std::optional<size_t> identifier_token_index
      = find_first_token_index(*tree, child.index, compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN);
    const std::string function_name = identifier_token_index.has_value()
      ? get_text_for_span(m_snapshot.get_text(), tree->get_token(*identifier_token_index).span)
      : std::string("<missing>");

    m_outline_items.push_back(editor_outline_item {
      "function " + function_name,
      m_snapshot.get_line_index().get_line_index(clamp_offset(node.span.start, m_snapshot.get_text_size())) + 1,
      node.span,
      subtree_is_recovered(*tree, child.index),
    });
  }
}
