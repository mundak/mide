#include "parser.h"

#include "source_span.h"
#include "syntax_diagnostic.h"
#include "syntax_identity.h"
#include "syntax_kind.h"
#include "syntax_node.h"
#include "syntax_token.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
  using compiler::syntax::source_span;
  using compiler::syntax::syntax_child_reference;
  using compiler::syntax::syntax_diagnostic;
  using compiler::syntax::syntax_kind;
  using compiler::syntax::syntax_node;
  using compiler::syntax::syntax_node_flags;
  using compiler::syntax::syntax_node_id;
  using compiler::syntax::syntax_token;
  using compiler::syntax::syntax_tree;

  uint64_t find_next_id(const syntax_tree& tree)
  {
    uint64_t max_id = 0;

    for (const syntax_node& node : tree.get_nodes())
    {
      if (node.id.value > max_id)
      {
        max_id = node.id.value;
      }
    }

    for (const syntax_token& token : tree.get_tokens())
    {
      if (token.id.value > max_id)
      {
        max_id = token.id.value;
      }
    }

    return max_id + 1;
  }

  const char* describe_token(syntax_kind kind)
  {
    switch (kind)
    {
    case compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN:
      return "end of file";

    case compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN:
      return "identifier";

    case compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN:
      return "integer literal";

    case compiler::syntax::SYNTAX_KIND_INT_KEYWORD:
      return "'int'";

    case compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD:
      return "'return'";

    case compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN:
      return "'('";

    case compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN:
      return "')'";

    case compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN:
      return "'{'";

    case compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN:
      return "'}'";

    case compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN:
      return "';'";

    case compiler::syntax::SYNTAX_KIND_BAD_TOKEN:
      return "invalid token";

    case compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT:
    case compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION:
    case compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR:
    case compiler::syntax::SYNTAX_KIND_PARAMETER_LIST:
    case compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT:
    case compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT:
    case compiler::syntax::SYNTAX_KIND_IDENTIFIER:
    case compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER:
    case compiler::syntax::SYNTAX_KIND_LITERAL_EXPRESSION:
    case compiler::syntax::SYNTAX_KIND_MISSING:
    case compiler::syntax::SYNTAX_KIND_ERROR:
      break;
    }

    throw std::invalid_argument("syntax_kind");
  }

  class parser_state
  {
  public:
    parser_state(const syntax_tree& token_tree, size_t text_size)
      : m_source_generation(token_tree.get_source_generation())
      , m_text_size(text_size)
      , m_tokens(token_tree.get_tokens())
      , m_next_id(find_next_id(token_tree))
      , m_position(0)
    {
      for (const syntax_diagnostic& diagnostic : token_tree.get_diagnostics())
      {
        if (compiler::syntax::is_lexical_diagnostic_code(diagnostic.code))
        {
          m_diagnostics.push_back(diagnostic);
        }
      }
    }

    std::shared_ptr<const syntax_tree> parse()
    {
      const size_t root_index = create_node(
        compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT, std::nullopt, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      while (get_current_kind() != compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN)
      {
        if (get_current_kind() == compiler::syntax::SYNTAX_KIND_INT_KEYWORD)
        {
          children.push_back(node_child(parse_function_definition(root_index)));
          continue;
        }

        children.push_back(node_child(parse_unexpected_token(root_index, "expected declaration")));
      }

      children.push_back(attach_current_token(root_index));
      finalize_node(root_index, source_span { 0, m_text_size }, std::move(children));

      return std::make_shared<const syntax_tree>(
        m_source_generation, std::move(m_nodes), std::move(m_tokens), root_index, std::move(m_diagnostics));
    }

  private:
    syntax_kind get_current_kind() const { return get_current_token().kind; }

    const syntax_token& get_current_token() const
    {
      if (m_position >= m_tokens.size())
      {
        throw std::out_of_range("parser token position");
      }

      return m_tokens[m_position];
    }

    size_t create_node(syntax_kind kind, const std::optional<size_t>& parent_index, syntax_node_flags flags)
    {
      const size_t node_index = m_nodes.size();
      m_nodes.push_back(syntax_node {
        kind,
        syntax_node_id { m_next_id++ },
        source_span { 0, 0 },
        parent_index,
        std::vector<syntax_child_reference> {},
        flags,
      });
      return node_index;
    }

    void finalize_node(size_t node_index, const source_span& span, std::vector<syntax_child_reference> children)
    {
      m_nodes[node_index].span = span;
      m_nodes[node_index].children = std::move(children);
    }

    syntax_child_reference node_child(size_t node_index) const
    {
      return syntax_child_reference { compiler::syntax::SYNTAX_CHILD_KIND_NODE, node_index };
    }

    syntax_child_reference attach_current_token(size_t parent_index)
    {
      if (m_position >= m_tokens.size())
      {
        throw std::out_of_range("parser token position");
      }

      m_tokens[m_position].parent_index = parent_index;
      const syntax_child_reference child {
        compiler::syntax::SYNTAX_CHILD_KIND_TOKEN,
        m_position,
      };
      ++m_position;
      return child;
    }

    source_span get_fallback_span() const
    {
      if (m_tokens.empty())
      {
        return source_span { 0, 0 };
      }

      return source_span { get_current_token().span.start, 0 };
    }

    source_span span_from_bounds(size_t start, size_t end) const { return source_span::from_bounds(start, end); }

    void add_diagnostic(compiler::syntax::syntax_diagnostic_code code, const source_span& span, std::string message)
    {
      m_diagnostics.push_back(syntax_diagnostic { code, span, std::move(message) });
    }

    size_t create_missing_node(size_t parent_index, std::string expected_description)
    {
      add_diagnostic(
        compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_EXPECTED_TOKEN,
        get_fallback_span(),
        "expected " + std::move(expected_description));

      const size_t node_index
        = create_node(compiler::syntax::SYNTAX_KIND_MISSING, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_MISSING);
      finalize_node(node_index, get_fallback_span(), std::vector<syntax_child_reference> {});
      return node_index;
    }

    size_t parse_unexpected_token(size_t parent_index, std::string context_message)
    {
      const size_t node_index
        = create_node(compiler::syntax::SYNTAX_KIND_ERROR, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_RECOVERED);

      const source_span token_span = get_current_token().span;
      add_diagnostic(
        compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN,
        token_span,
        std::move(context_message) + ": unexpected " + describe_token(get_current_kind()));

      std::vector<syntax_child_reference> children;
      children.push_back(attach_current_token(node_index));
      finalize_node(node_index, token_span, std::move(children));
      return node_index;
    }

    size_t parse_identifier(size_t parent_index)
    {
      const size_t node_index
        = create_node(compiler::syntax::SYNTAX_KIND_IDENTIFIER, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;

      if (get_current_kind() != compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN)
      {
        children.push_back(node_child(create_missing_node(node_index, "identifier")));
        finalize_node(node_index, get_fallback_span(), std::move(children));
        return node_index;
      }

      const source_span span = get_current_token().span;
      children.push_back(attach_current_token(node_index));
      finalize_node(node_index, span, std::move(children));
      return node_index;
    }

    size_t parse_type_specifier(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      if (get_current_kind() != compiler::syntax::SYNTAX_KIND_INT_KEYWORD)
      {
        add_diagnostic(compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_EXPECTED_TOKEN, get_fallback_span(), "expected 'int'");
        const size_t missing_index
          = create_node(compiler::syntax::SYNTAX_KIND_MISSING, node_index, compiler::syntax::SYNTAX_NODE_FLAG_MISSING);
        finalize_node(missing_index, get_fallback_span(), std::vector<syntax_child_reference> {});
        std::vector<syntax_child_reference> children;
        children.push_back(node_child(missing_index));
        finalize_node(node_index, get_fallback_span(), std::move(children));
        return node_index;
      }

      const source_span span = get_current_token().span;
      std::vector<syntax_child_reference> children;
      children.push_back(attach_current_token(node_index));
      finalize_node(node_index, span, std::move(children));
      return node_index;
    }

    size_t parse_literal_expression(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_LITERAL_EXPRESSION, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      if (get_current_kind() != compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN)
      {
        add_diagnostic(
          compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION,
          get_fallback_span(),
          "expected integer literal");
        const size_t missing_index
          = create_node(compiler::syntax::SYNTAX_KIND_MISSING, node_index, compiler::syntax::SYNTAX_NODE_FLAG_MISSING);
        finalize_node(missing_index, get_fallback_span(), std::vector<syntax_child_reference> {});
        std::vector<syntax_child_reference> children;
        children.push_back(node_child(missing_index));
        finalize_node(node_index, get_fallback_span(), std::move(children));
        return node_index;
      }

      const source_span span = get_current_token().span;
      std::vector<syntax_child_reference> children;
      children.push_back(attach_current_token(node_index));
      finalize_node(node_index, span, std::move(children));
      return node_index;
    }

    size_t parse_parameter_list(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_PARAMETER_LIST, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      size_t span_start = get_fallback_span().start;
      size_t span_end = span_start;

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN)
      {
        span_start = get_current_token().span.start;
        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "'('")));
      }

      while ((get_current_kind() != compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN)
             && (get_current_kind() != compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN))
      {
        children.push_back(node_child(parse_unexpected_token(node_index, "unexpected token in parameter list")));
        span_end = m_nodes[children.back().index].span.get_end();
      }

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN)
      {
        if (children.empty())
        {
          span_start = get_current_token().span.start;
        }

        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "')'")));
      }

      finalize_node(node_index, span_from_bounds(span_start, span_end), std::move(children));
      return node_index;
    }

    size_t parse_function_declarator(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      children.push_back(node_child(parse_identifier(node_index)));
      children.push_back(node_child(parse_parameter_list(node_index)));

      const size_t start = m_nodes[children.front().index].span.start;
      const size_t end = m_nodes[children.back().index].span.get_end();
      finalize_node(node_index, span_from_bounds(start, end), std::move(children));
      return node_index;
    }

    size_t parse_return_statement(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      size_t span_start = get_fallback_span().start;
      size_t span_end = span_start;

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD)
      {
        span_start = get_current_token().span.start;
        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "'return'")));
      }

      children.push_back(node_child(parse_literal_expression(node_index)));
      if (m_nodes[children.back().index].span.get_end() > span_end)
      {
        span_end = m_nodes[children.back().index].span.get_end();
      }

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN)
      {
        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "';'")));
      }

      finalize_node(node_index, span_from_bounds(span_start, span_end), std::move(children));
      return node_index;
    }

    size_t parse_compound_statement(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      size_t span_start = get_fallback_span().start;
      size_t span_end = span_start;

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN)
      {
        span_start = get_current_token().span.start;
        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "'{'")));
      }

      while ((get_current_kind() != compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN)
             && (get_current_kind() != compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN))
      {
        if (get_current_kind() == compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD)
        {
          children.push_back(node_child(parse_return_statement(node_index)));
        }
        else
        {
          children.push_back(node_child(parse_unexpected_token(node_index, "unexpected token in compound statement")));
        }

        if (m_nodes[children.back().index].span.get_end() > span_end)
        {
          span_end = m_nodes[children.back().index].span.get_end();
        }
      }

      if (get_current_kind() == compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN)
      {
        span_end = get_current_token().span.get_end();
        children.push_back(attach_current_token(node_index));
      }
      else
      {
        children.push_back(node_child(create_missing_node(node_index, "'}'")));
      }

      finalize_node(node_index, span_from_bounds(span_start, span_end), std::move(children));
      return node_index;
    }

    size_t parse_function_definition(size_t parent_index)
    {
      const size_t node_index = create_node(
        compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION, parent_index, compiler::syntax::SYNTAX_NODE_FLAG_NONE);

      std::vector<syntax_child_reference> children;
      children.push_back(node_child(parse_type_specifier(node_index)));
      children.push_back(node_child(parse_function_declarator(node_index)));
      children.push_back(node_child(parse_compound_statement(node_index)));

      const size_t start = m_nodes[children.front().index].span.start;
      const size_t end = m_nodes[children.back().index].span.get_end();
      finalize_node(node_index, span_from_bounds(start, end), std::move(children));
      return node_index;
    }

    uint64_t m_source_generation;
    size_t m_text_size;
    std::vector<syntax_token> m_tokens;
    std::vector<syntax_node> m_nodes;
    std::vector<syntax_diagnostic> m_diagnostics;
    uint64_t m_next_id;
    size_t m_position;
  };
}

std::shared_ptr<const syntax_tree> compiler::syntax::parser::parse(const syntax_tree& token_tree, size_t text_size)
{
  parser_state state(token_tree, text_size);
  return state.parse();
}
