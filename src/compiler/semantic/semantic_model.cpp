#include "semantic_model.h"

#include "compiler/syntax/source_span.h"
#include "compiler/syntax/syntax_kind.h"
#include "compiler/syntax/syntax_node.h"
#include "compiler/syntax/syntax_tree.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
  using compiler::semantic::semantic_model;
  using compiler::semantic::semantic_scope;
  using compiler::semantic::semantic_symbol;
  using compiler::semantic::semantic_symbol_kind;
  using compiler::semantic::semantic_token_classification;
  using compiler::semantic::semantic_type_kind;
  using compiler::syntax::source_span;
  using compiler::syntax::syntax_child_reference;
  using compiler::syntax::syntax_kind;
  using compiler::syntax::syntax_node;
  using compiler::syntax::syntax_token;
  using compiler::syntax::syntax_tree;

  std::string get_text_for_span(std::string_view text, source_span span)
  {
    const size_t end_offset = span.get_end();
    if ((span.start > text.size()) || (end_offset > text.size()))
    {
      throw std::out_of_range("source span outside document text");
    }

    return std::string(text.substr(span.start, span.length));
  }

  class semantic_analyzer
  {
  public:
    semantic_analyzer(const syntax_tree& tree, std::string_view text)
      : m_tree(tree)
      , m_text(text)
    {
    }

    std::shared_ptr<const semantic_model> analyze()
    {
      if (!m_tree.has_root())
      {
        return std::make_shared<const semantic_model>(
          std::vector<semantic_scope> {},
          std::vector<semantic_symbol> {},
          std::unordered_map<uint64_t, semantic_token_classification> {},
          std::unordered_map<uint64_t, semantic_type_kind> {});
      }

      const size_t root_scope_index = create_scope(std::nullopt, m_tree.get_root().id);
      analyze_translation_unit(root_scope_index);

      return std::make_shared<const semantic_model>(
        std::move(m_scopes), std::move(m_symbols), std::move(m_token_classifications), std::move(m_node_types));
    }

  private:
    size_t create_scope(const std::optional<size_t>& parent_index, const compiler::syntax::syntax_node_id& owner_id)
    {
      const size_t scope_index = m_scopes.size();
      m_scopes.push_back(semantic_scope {
        owner_id,
        parent_index,
        std::vector<size_t> {},
      });
      m_scope_bindings.push_back(std::unordered_map<std::string, size_t> {});
      return scope_index;
    }

    size_t define_symbol(
      size_t scope_index,
      const compiler::syntax::syntax_node_id& declaration_id,
      std::string name,
      semantic_symbol_kind kind,
      semantic_type_kind type_kind)
    {
      const size_t symbol_index = m_symbols.size();
      m_symbols.push_back(semantic_symbol {
        declaration_id,
        std::move(name),
        kind,
        type_kind,
        scope_index,
      });
      m_scopes[scope_index].symbol_indices.push_back(symbol_index);

      std::unordered_map<std::string, size_t>& bindings = m_scope_bindings[scope_index];
      if (bindings.find(m_symbols[symbol_index].name) == bindings.end())
      {
        bindings.insert(std::make_pair(m_symbols[symbol_index].name, symbol_index));
      }

      return symbol_index;
    }

    std::optional<size_t> resolve_symbol(size_t scope_index, const std::string& name) const
    {
      std::optional<size_t> current_scope_index = scope_index;
      while (current_scope_index.has_value())
      {
        const std::unordered_map<std::string, size_t>& bindings = m_scope_bindings[*current_scope_index];
        const std::unordered_map<std::string, size_t>::const_iterator iterator = bindings.find(name);
        if (iterator != bindings.end())
        {
          return iterator->second;
        }

        current_scope_index = m_scopes[*current_scope_index].parent_index;
      }

      return std::nullopt;
    }

    void record_token_classification(const syntax_token& token, semantic_token_classification classification)
    {
      m_token_classifications[token.id.value] = classification;
    }

    void record_node_type(const syntax_node& node, semantic_type_kind type_kind)
    {
      m_node_types[node.id.value] = type_kind;
    }

    std::optional<size_t> find_identifier_token_index(size_t node_index) const
    {
      const syntax_node& node = m_tree.get_node(node_index);
      for (const syntax_child_reference& child : node.children)
      {
        if (child.is_token())
        {
          if (m_tree.get_token(child.index).kind == compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN)
          {
            return child.index;
          }

          continue;
        }

        const std::optional<size_t> token_index = find_identifier_token_index(child.index);
        if (token_index.has_value())
        {
          return token_index;
        }
      }

      return std::nullopt;
    }

    std::optional<size_t> find_child_node(size_t parent_index, syntax_kind kind) const
    {
      const syntax_node& parent = m_tree.get_node(parent_index);
      for (const syntax_child_reference& child : parent.children)
      {
        if (child.is_node() && (m_tree.get_node(child.index).kind == kind))
        {
          return child.index;
        }
      }

      return std::nullopt;
    }

    semantic_type_kind resolve_type_specifier(size_t node_index)
    {
      const syntax_node& node = m_tree.get_node(node_index);
      record_node_type(node, compiler::semantic::SEMANTIC_TYPE_KIND_INT);
      return compiler::semantic::SEMANTIC_TYPE_KIND_INT;
    }

    void analyze_translation_unit(size_t root_scope_index)
    {
      const syntax_node& root = m_tree.get_root();
      for (const syntax_child_reference& child : root.children)
      {
        if (!child.is_node())
        {
          continue;
        }

        if (m_tree.get_node(child.index).kind == compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION)
        {
          analyze_function_definition(child.index, root_scope_index);
        }
      }
    }

    void analyze_function_definition(size_t function_index, size_t parent_scope_index)
    {
      const std::optional<size_t> type_specifier_index
        = find_child_node(function_index, compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER);
      const std::optional<size_t> declarator_index
        = find_child_node(function_index, compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR);
      const std::optional<size_t> body_index
        = find_child_node(function_index, compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT);

      if (!type_specifier_index.has_value() || !declarator_index.has_value() || !body_index.has_value())
      {
        return;
      }

      const semantic_type_kind return_type = resolve_type_specifier(*type_specifier_index);
      const syntax_node& function_node = m_tree.get_node(function_index);
      record_node_type(function_node, return_type);

      const std::optional<size_t> function_name_node_index
        = find_child_node(*declarator_index, compiler::syntax::SYNTAX_KIND_IDENTIFIER);
      if (!function_name_node_index.has_value())
      {
        return;
      }

      const std::optional<size_t> function_name_token_index = find_identifier_token_index(*function_name_node_index);
      if (!function_name_token_index.has_value())
      {
        return;
      }

      const syntax_node& function_name_node = m_tree.get_node(*function_name_node_index);
      const syntax_token& function_name_token = m_tree.get_token(*function_name_token_index);
      const std::string function_name = get_text_for_span(m_text, function_name_token.span);
      define_symbol(
        parent_scope_index,
        function_name_node.id,
        function_name,
        compiler::semantic::SEMANTIC_SYMBOL_KIND_FUNCTION,
        return_type);
      record_token_classification(function_name_token, compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_FUNCTION);

      const size_t function_scope_index = create_scope(std::optional<size_t>(parent_scope_index), function_node.id);
      analyze_parameter_list(*declarator_index, function_scope_index);

      const size_t body_scope_index = create_scope(std::optional<size_t>(function_scope_index), function_node.id);
      analyze_compound_statement(*body_index, body_scope_index);
    }

    void analyze_parameter_list(size_t declarator_index, size_t function_scope_index)
    {
      const std::optional<size_t> parameter_list_index
        = find_child_node(declarator_index, compiler::syntax::SYNTAX_KIND_PARAMETER_LIST);
      if (!parameter_list_index.has_value())
      {
        return;
      }

      const syntax_node& parameter_list = m_tree.get_node(*parameter_list_index);
      for (const syntax_child_reference& child : parameter_list.children)
      {
        if (!child.is_node())
        {
          continue;
        }

        if (m_tree.get_node(child.index).kind == compiler::syntax::SYNTAX_KIND_PARAMETER_DECLARATION)
        {
          analyze_parameter_declaration(child.index, function_scope_index);
        }
      }
    }

    void analyze_parameter_declaration(size_t declaration_index, size_t function_scope_index)
    {
      const std::optional<size_t> type_specifier_index
        = find_child_node(declaration_index, compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER);
      const std::optional<size_t> identifier_index
        = find_child_node(declaration_index, compiler::syntax::SYNTAX_KIND_IDENTIFIER);

      if (!type_specifier_index.has_value() || !identifier_index.has_value())
      {
        return;
      }

      const semantic_type_kind parameter_type = resolve_type_specifier(*type_specifier_index);
      const syntax_node& declaration_node = m_tree.get_node(declaration_index);
      record_node_type(declaration_node, parameter_type);

      const std::optional<size_t> token_index = find_identifier_token_index(*identifier_index);
      if (!token_index.has_value())
      {
        return;
      }

      const syntax_node& identifier_node = m_tree.get_node(*identifier_index);
      const syntax_token& identifier_token = m_tree.get_token(*token_index);
      define_symbol(
        function_scope_index,
        identifier_node.id,
        get_text_for_span(m_text, identifier_token.span),
        compiler::semantic::SEMANTIC_SYMBOL_KIND_PARAMETER,
        parameter_type);
      record_token_classification(identifier_token, compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_PARAMETER);
    }

    void analyze_compound_statement(size_t compound_index, size_t scope_index)
    {
      const syntax_node& compound_node = m_tree.get_node(compound_index);
      for (const syntax_child_reference& child : compound_node.children)
      {
        if (!child.is_node())
        {
          continue;
        }

        const syntax_node& child_node = m_tree.get_node(child.index);
        switch (child_node.kind)
        {
        case compiler::syntax::SYNTAX_KIND_DECLARATION_STATEMENT:
          analyze_declaration_statement(child.index, scope_index);
          break;

        case compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT:
          analyze_return_statement(child.index, scope_index);
          break;

        case compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT:
        {
          const size_t nested_scope_index = create_scope(std::optional<size_t>(scope_index), child_node.id);
          analyze_compound_statement(child.index, nested_scope_index);
          break;
        }

        case compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT:
        case compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION:
        case compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR:
        case compiler::syntax::SYNTAX_KIND_PARAMETER_LIST:
        case compiler::syntax::SYNTAX_KIND_PARAMETER_DECLARATION:
        case compiler::syntax::SYNTAX_KIND_IDENTIFIER:
        case compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER:
        case compiler::syntax::SYNTAX_KIND_IDENTIFIER_EXPRESSION:
        case compiler::syntax::SYNTAX_KIND_LITERAL_EXPRESSION:
        case compiler::syntax::SYNTAX_KIND_MISSING:
        case compiler::syntax::SYNTAX_KIND_ERROR:
          break;

        case compiler::syntax::SYNTAX_KIND_BAD_TOKEN:
        case compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN:
        case compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN:
        case compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN:
        case compiler::syntax::SYNTAX_KIND_INT_KEYWORD:
        case compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD:
        case compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN:
        case compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN:
        case compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN:
        case compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN:
        case compiler::syntax::SYNTAX_KIND_COMMA_TOKEN:
        case compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN:
          break;
        }
      }
    }

    void analyze_declaration_statement(size_t declaration_index, size_t scope_index)
    {
      const std::optional<size_t> type_specifier_index
        = find_child_node(declaration_index, compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER);
      const std::optional<size_t> identifier_index
        = find_child_node(declaration_index, compiler::syntax::SYNTAX_KIND_IDENTIFIER);

      if (!type_specifier_index.has_value() || !identifier_index.has_value())
      {
        return;
      }

      const semantic_type_kind local_type = resolve_type_specifier(*type_specifier_index);
      const syntax_node& declaration_node = m_tree.get_node(declaration_index);
      record_node_type(declaration_node, local_type);

      const std::optional<size_t> token_index = find_identifier_token_index(*identifier_index);
      if (!token_index.has_value())
      {
        return;
      }

      const syntax_node& identifier_node = m_tree.get_node(*identifier_index);
      const syntax_token& identifier_token = m_tree.get_token(*token_index);
      define_symbol(
        scope_index,
        identifier_node.id,
        get_text_for_span(m_text, identifier_token.span),
        compiler::semantic::SEMANTIC_SYMBOL_KIND_LOCAL,
        local_type);
      record_token_classification(identifier_token, compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_LOCAL);
    }

    void analyze_return_statement(size_t return_index, size_t scope_index)
    {
      const syntax_node& return_node = m_tree.get_node(return_index);
      for (const syntax_child_reference& child : return_node.children)
      {
        if (child.is_node())
        {
          analyze_expression(child.index, scope_index);
        }
      }
    }

    void analyze_expression(size_t expression_index, size_t scope_index)
    {
      const syntax_node& expression_node = m_tree.get_node(expression_index);
      switch (expression_node.kind)
      {
      case compiler::syntax::SYNTAX_KIND_LITERAL_EXPRESSION:
        record_node_type(expression_node, compiler::semantic::SEMANTIC_TYPE_KIND_INT);
        return;

      case compiler::syntax::SYNTAX_KIND_IDENTIFIER_EXPRESSION:
        analyze_identifier_expression(expression_index, scope_index);
        return;

      case compiler::syntax::SYNTAX_KIND_IDENTIFIER:
      case compiler::syntax::SYNTAX_KIND_MISSING:
      case compiler::syntax::SYNTAX_KIND_ERROR:
        return;

      case compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT:
      case compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION:
      case compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR:
      case compiler::syntax::SYNTAX_KIND_PARAMETER_LIST:
      case compiler::syntax::SYNTAX_KIND_PARAMETER_DECLARATION:
      case compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT:
      case compiler::syntax::SYNTAX_KIND_DECLARATION_STATEMENT:
      case compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT:
      case compiler::syntax::SYNTAX_KIND_TYPE_SPECIFIER:
        return;

      case compiler::syntax::SYNTAX_KIND_BAD_TOKEN:
      case compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN:
      case compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN:
      case compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN:
      case compiler::syntax::SYNTAX_KIND_INT_KEYWORD:
      case compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD:
      case compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN:
      case compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN:
      case compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN:
      case compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN:
      case compiler::syntax::SYNTAX_KIND_COMMA_TOKEN:
      case compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN:
        return;
      }
    }

    semantic_token_classification classify_symbol_kind(semantic_symbol_kind kind) const
    {
      switch (kind)
      {
      case compiler::semantic::SEMANTIC_SYMBOL_KIND_TYPE:
        return compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_TYPE;

      case compiler::semantic::SEMANTIC_SYMBOL_KIND_FUNCTION:
        return compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_FUNCTION;

      case compiler::semantic::SEMANTIC_SYMBOL_KIND_PARAMETER:
        return compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_PARAMETER;

      case compiler::semantic::SEMANTIC_SYMBOL_KIND_LOCAL:
        return compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_LOCAL;

      case compiler::semantic::SEMANTIC_SYMBOL_KIND_FIELD:
        return compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_FIELD;
      }

      throw std::invalid_argument("semantic_symbol_kind");
    }

    void analyze_identifier_expression(size_t expression_index, size_t scope_index)
    {
      const std::optional<size_t> identifier_index
        = find_child_node(expression_index, compiler::syntax::SYNTAX_KIND_IDENTIFIER);
      if (!identifier_index.has_value())
      {
        return;
      }

      const std::optional<size_t> token_index = find_identifier_token_index(*identifier_index);
      if (!token_index.has_value())
      {
        return;
      }

      const syntax_token& identifier_token = m_tree.get_token(*token_index);
      const std::string symbol_name = get_text_for_span(m_text, identifier_token.span);
      const std::optional<size_t> symbol_index = resolve_symbol(scope_index, symbol_name);
      if (!symbol_index.has_value())
      {
        record_token_classification(identifier_token, compiler::semantic::SEMANTIC_TOKEN_CLASSIFICATION_UNRESOLVED);
        return;
      }

      const semantic_symbol& symbol = m_symbols[*symbol_index];
      record_token_classification(identifier_token, classify_symbol_kind(symbol.kind));
      record_node_type(m_tree.get_node(expression_index), symbol.type_kind);
    }

    const syntax_tree& m_tree;
    std::string_view m_text;
    std::vector<semantic_scope> m_scopes;
    std::vector<semantic_symbol> m_symbols;
    std::vector<std::unordered_map<std::string, size_t>> m_scope_bindings;
    std::unordered_map<uint64_t, semantic_token_classification> m_token_classifications;
    std::unordered_map<uint64_t, semantic_type_kind> m_node_types;
  };
}

compiler::semantic::semantic_model::semantic_model(
  std::vector<semantic_scope> scopes,
  std::vector<semantic_symbol> symbols,
  std::unordered_map<uint64_t, semantic_token_classification> token_classifications,
  std::unordered_map<uint64_t, semantic_type_kind> node_types)
  : m_scopes(std::move(scopes))
  , m_symbols(std::move(symbols))
  , m_token_classifications(std::move(token_classifications))
  , m_node_types(std::move(node_types))
{
}

std::shared_ptr<const compiler::semantic::semantic_model> compiler::semantic::semantic_model::analyze(
  const syntax::syntax_tree& tree, std::string_view text)
{
  semantic_analyzer analyzer(tree, text);
  return analyzer.analyze();
}

compiler::semantic::semantic_token_classification compiler::semantic::semantic_model::get_token_classification(
  const syntax::syntax_token& token) const
{
  const std::unordered_map<uint64_t, semantic_token_classification>::const_iterator iterator
    = m_token_classifications.find(token.id.value);
  if (iterator == m_token_classifications.end())
  {
    return SEMANTIC_TOKEN_CLASSIFICATION_NONE;
  }

  return iterator->second;
}

compiler::semantic::semantic_type_kind compiler::semantic::semantic_model::get_node_type(
  const syntax::syntax_node_id& node_id) const
{
  const std::unordered_map<uint64_t, semantic_type_kind>::const_iterator iterator = m_node_types.find(node_id.value);
  if (iterator == m_node_types.end())
  {
    return SEMANTIC_TYPE_KIND_UNKNOWN;
  }

  return iterator->second;
}

const std::vector<semantic_scope>& compiler::semantic::semantic_model::get_scopes() const
{
  return m_scopes;
}

const std::vector<semantic_symbol>& compiler::semantic::semantic_model::get_symbols() const
{
  return m_symbols;
}
