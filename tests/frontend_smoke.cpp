#include "compiler/document/document_snapshot.h"
#include "compiler/document/text_change.h"
#include "compiler/syntax/syntax_kind.h"
#include "compiler/syntax/syntax_tree.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
  void require(bool condition, const std::string& message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  const compiler::syntax::syntax_node& get_child_node(
    const compiler::syntax::syntax_tree& tree, const compiler::syntax::syntax_node& parent, size_t child_offset)
  {
    require(child_offset < parent.children.size(), "missing child node");
    const compiler::syntax::syntax_child_reference& child = parent.children[child_offset];
    require(child.is_node(), "expected node child");
    return tree.get_node(child.index);
  }

  bool tree_contains_kind(const compiler::syntax::syntax_tree& tree, compiler::syntax::syntax_kind kind)
  {
    for (const compiler::syntax::syntax_node& node : tree.get_nodes())
    {
      if (node.kind == kind)
      {
        return true;
      }
    }

    return false;
  }

  size_t find_first_token_index(const compiler::syntax::syntax_tree& tree, compiler::syntax::syntax_kind kind)
  {
    for (size_t token_index = 0; token_index < tree.get_token_count(); ++token_index)
    {
      if (tree.get_token(token_index).kind == kind)
      {
        return token_index;
      }
    }

    throw std::runtime_error("token kind not found");
  }

  void verify_bootstrap_parse()
  {
    const std::string source_text = "int main() { return 1; }";
    const compiler::document::document_snapshot snapshot
      = compiler::document::document_snapshot::create_initial(source_text);
    const std::shared_ptr<const compiler::syntax::syntax_tree> tree = snapshot.get_syntax_tree();

    require(tree != nullptr, "syntax tree missing");
    require(tree->get_diagnostics().empty(), "unexpected diagnostics for bootstrap input");
    require(tree->get_root().kind == compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT, "wrong root kind");
    require(tree->get_root().children.size() == 2, "expected function definition and eof at root");

    const compiler::syntax::syntax_node& function_definition = get_child_node(*tree, tree->get_root(), 0);
    require(
      function_definition.kind == compiler::syntax::SYNTAX_KIND_FUNCTION_DEFINITION,
      "expected function definition node");

    const compiler::syntax::syntax_node& function_declarator = get_child_node(*tree, function_definition, 1);
    require(
      function_declarator.kind == compiler::syntax::SYNTAX_KIND_FUNCTION_DECLARATOR,
      "expected function declarator node");

    const compiler::syntax::syntax_node& parameter_list = get_child_node(*tree, function_declarator, 1);
    require(parameter_list.kind == compiler::syntax::SYNTAX_KIND_PARAMETER_LIST, "expected parameter list node");

    const compiler::syntax::syntax_node& compound_statement = get_child_node(*tree, function_definition, 2);
    require(
      compound_statement.kind == compiler::syntax::SYNTAX_KIND_COMPOUND_STATEMENT, "expected compound statement node");

    const compiler::syntax::syntax_node& return_statement = get_child_node(*tree, compound_statement, 1);
    require(return_statement.kind == compiler::syntax::SYNTAX_KIND_RETURN_STATEMENT, "expected return statement");
  }

  void verify_incremental_relex_preserves_unaffected_tokens()
  {
    const std::string source_text = "int main() { return 1; }";
    const compiler::document::document_snapshot initial_snapshot
      = compiler::document::document_snapshot::create_initial(source_text);
    const std::shared_ptr<const compiler::syntax::syntax_tree> initial_tree = initial_snapshot.get_syntax_tree();
    const size_t literal_offset = source_text.find('1');
    require(literal_offset != std::string::npos, "literal not found");

    const compiler::document::text_change change {
      literal_offset + 1,
      literal_offset + 1,
      "2",
    };

    const compiler::document::document_snapshot updated_snapshot = initial_snapshot.apply_change(change);
    const std::shared_ptr<const compiler::syntax::syntax_tree> updated_tree = updated_snapshot.get_syntax_tree();
    require(updated_tree->get_diagnostics().empty(), "unexpected diagnostics after incremental relex");
    require(initial_tree->get_token_count() == updated_tree->get_token_count(), "token count changed unexpectedly");

    require(initial_tree->get_token(0).id.value == updated_tree->get_token(0).id.value, "first token id changed");
    require(initial_tree->get_token(1).id.value == updated_tree->get_token(1).id.value, "identifier token id changed");
    require(initial_tree->get_token(2).id.value == updated_tree->get_token(2).id.value, "open paren id changed");
    require(
      initial_tree->get_token(initial_tree->get_token_count() - 1).id.value
        == updated_tree->get_token(updated_tree->get_token_count() - 1).id.value,
      "eof token id changed");

    const size_t initial_literal_index
      = find_first_token_index(*initial_tree, compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN);
    const size_t updated_literal_index
      = find_first_token_index(*updated_tree, compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN);
    require(
      initial_tree->get_token(initial_literal_index).id.value
        != updated_tree->get_token(updated_literal_index).id.value,
      "literal token id did not change");
  }

  void verify_error_recovery()
  {
    const compiler::document::document_snapshot snapshot
      = compiler::document::document_snapshot::create_initial("int main( { return ;");
    const std::shared_ptr<const compiler::syntax::syntax_tree> tree = snapshot.get_syntax_tree();

    require(tree != nullptr, "syntax tree missing for broken input");
    require(!tree->get_diagnostics().empty(), "expected diagnostics for broken input");
    require(tree_contains_kind(*tree, compiler::syntax::SYNTAX_KIND_MISSING), "expected missing syntax node");
  }
}

int main()
{
  try
  {
    verify_bootstrap_parse();
    verify_incremental_relex_preserves_unaffected_tokens();
    verify_error_recovery();
    return 0;
  }
  catch (const std::exception& exception)
  {
    std::cerr << exception.what() << '\n';
    return 1;
  }
}
