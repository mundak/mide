#include "document_snapshot.h"

#include "compiler/syntax/lexer.h"
#include "compiler/syntax/syntax_tree.h"

#include <stdexcept>
#include <utility>

namespace
{
  std::shared_ptr<const compiler::syntax::syntax_tree> ensure_tree(
    uint64_t generation, const std::shared_ptr<const compiler::syntax::syntax_tree>& syntax_tree)
  {
    if (syntax_tree != nullptr)
    {
      if (syntax_tree->get_source_generation() != generation)
      {
        throw std::invalid_argument("syntax_tree generation does not match snapshot generation");
      }

      return syntax_tree;
    }

    return compiler::syntax::syntax_tree::create_empty(generation);
  }
}

compiler::document::document_snapshot::document_snapshot(uint64_t generation, std::string text)
  : document_snapshot(generation, std::move(text), compiler::syntax::syntax_tree::create_empty(generation))
{
}

compiler::document::document_snapshot::document_snapshot(
  uint64_t generation, std::string text, const std::shared_ptr<const compiler::syntax::syntax_tree>& syntax_tree)
  : m_generation(generation)
  , m_text(std::move(text))
  , m_line_index(m_text)
  , m_syntax_tree(ensure_tree(generation, syntax_tree))
{
}

compiler::document::document_snapshot compiler::document::document_snapshot::create_initial(std::string text)
{
  std::shared_ptr<const compiler::syntax::syntax_tree> syntax_tree = compiler::syntax::lexer::lex(1, text);
  return document_snapshot(1, std::move(text), syntax_tree);
}

compiler::document::document_snapshot compiler::document::document_snapshot::apply_change(
  const text_change& change) const
{
  const uint64_t next_generation = m_generation + 1;
  std::string updated_text = apply_change_to_text(m_text, change);
  std::shared_ptr<const compiler::syntax::syntax_tree> syntax_tree
    = compiler::syntax::lexer::relex(next_generation, m_text, *m_syntax_tree, change, updated_text);
  return document_snapshot(next_generation, std::move(updated_text), syntax_tree);
}

compiler::document::document_snapshot compiler::document::document_snapshot::with_syntax_tree(
  const std::shared_ptr<const syntax::syntax_tree>& syntax_tree) const
{
  return document_snapshot(m_generation, m_text, ensure_tree(m_generation, syntax_tree));
}

uint64_t compiler::document::document_snapshot::get_generation() const
{
  return m_generation;
}

const compiler::document::line_index& compiler::document::document_snapshot::get_line_index() const
{
  return m_line_index;
}

std::shared_ptr<const compiler::syntax::syntax_tree> compiler::document::document_snapshot::get_syntax_tree() const
{
  return m_syntax_tree;
}

const std::string& compiler::document::document_snapshot::get_text() const
{
  return m_text;
}

size_t compiler::document::document_snapshot::get_text_size() const
{
  return m_text.size();
}

std::string compiler::document::document_snapshot::apply_change_to_text(
  const std::string& text, const text_change& change)
{
  if ((change.start_offset > change.end_offset) || (change.end_offset > text.size()))
  {
    throw std::out_of_range("text_change");
  }

  std::string updated_text;
  updated_text.reserve(text.size() + change.replacement_text.size() - change.get_removed_length());
  updated_text.append(text, 0, change.start_offset);
  updated_text.append(change.replacement_text);
  updated_text.append(text, change.end_offset, std::string::npos);
  return updated_text;
}
