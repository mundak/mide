#include "source_span.h"
#include "syntax_diagnostic.h"
#include "syntax_identity.h"
#include "syntax_kind.h"
#include "syntax_node.h"

#include <limits>
#include <stdexcept>

bool compiler::syntax::source_span::contains(size_t offset) const
{
  return (offset >= start) && (offset < get_end());
}

size_t compiler::syntax::source_span::get_end() const
{
  if (length > (std::numeric_limits<size_t>::max() - start))
  {
    throw std::overflow_error("source_span end overflow");
  }

  return start + length;
}

compiler::syntax::source_span compiler::syntax::source_span::from_bounds(size_t start_offset, size_t end_offset)
{
  if (end_offset < start_offset)
  {
    throw std::invalid_argument("source_span bounds");
  }

  return source_span { start_offset, end_offset - start_offset };
}

bool compiler::syntax::syntax_node_id::is_valid() const
{
  return value != 0;
}

bool compiler::syntax::syntax_child_reference::is_node() const
{
  return kind == SYNTAX_CHILD_KIND_NODE;
}

bool compiler::syntax::syntax_child_reference::is_token() const
{
  return kind == SYNTAX_CHILD_KIND_TOKEN;
}

bool compiler::syntax::is_token_kind(syntax_kind kind)
{
  switch (kind)
  {
  case SYNTAX_KIND_BAD_TOKEN:
  case SYNTAX_KIND_END_OF_FILE_TOKEN:
  case SYNTAX_KIND_IDENTIFIER_TOKEN:
  case SYNTAX_KIND_INTEGER_LITERAL_TOKEN:
  case SYNTAX_KIND_INT_KEYWORD:
  case SYNTAX_KIND_RETURN_KEYWORD:
  case SYNTAX_KIND_OPEN_PAREN_TOKEN:
  case SYNTAX_KIND_CLOSE_PAREN_TOKEN:
  case SYNTAX_KIND_OPEN_BRACE_TOKEN:
  case SYNTAX_KIND_CLOSE_BRACE_TOKEN:
  case SYNTAX_KIND_COMMA_TOKEN:
  case SYNTAX_KIND_SEMICOLON_TOKEN:
    return true;

  case SYNTAX_KIND_TRANSLATION_UNIT:
  case SYNTAX_KIND_FUNCTION_DEFINITION:
  case SYNTAX_KIND_FUNCTION_DECLARATOR:
  case SYNTAX_KIND_PARAMETER_LIST:
  case SYNTAX_KIND_PARAMETER_DECLARATION:
  case SYNTAX_KIND_COMPOUND_STATEMENT:
  case SYNTAX_KIND_DECLARATION_STATEMENT:
  case SYNTAX_KIND_RETURN_STATEMENT:
  case SYNTAX_KIND_IDENTIFIER:
  case SYNTAX_KIND_TYPE_SPECIFIER:
  case SYNTAX_KIND_IDENTIFIER_EXPRESSION:
  case SYNTAX_KIND_LITERAL_EXPRESSION:
  case SYNTAX_KIND_MISSING:
  case SYNTAX_KIND_ERROR:
    return false;
  }

  throw std::invalid_argument("syntax_kind");
}

compiler::syntax::syntax_node_flags compiler::syntax::operator|(syntax_node_flags left, syntax_node_flags right)
{
  return static_cast<syntax_node_flags>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
}

bool compiler::syntax::has_flag(syntax_node_flags flags, syntax_node_flags flag)
{
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

bool compiler::syntax::is_lexical_diagnostic_code(syntax_diagnostic_code code)
{
  switch (code)
  {
  case SYNTAX_DIAGNOSTIC_CODE_INVALID_TOKEN:
  case SYNTAX_DIAGNOSTIC_CODE_INVALID_INTEGER_LITERAL:
  case SYNTAX_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK_COMMENT:
    return true;

  case SYNTAX_DIAGNOSTIC_CODE_INTERNAL_ERROR:
  case SYNTAX_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN:
  case SYNTAX_DIAGNOSTIC_CODE_EXPECTED_TOKEN:
  case SYNTAX_DIAGNOSTIC_CODE_EXPECTED_DECLARATION:
  case SYNTAX_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION:
    return false;
  }

  throw std::invalid_argument("syntax_diagnostic_code");
}
