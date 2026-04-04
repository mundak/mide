#pragma once

#include "compiler/syntax/syntax_identity.h"
#include "compiler/syntax/syntax_token.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace compiler
{
  namespace syntax
  {
    class syntax_tree;
  }

  namespace semantic
  {
    enum semantic_symbol_kind : uint32_t
    {
      SEMANTIC_SYMBOL_KIND_TYPE,
      SEMANTIC_SYMBOL_KIND_FUNCTION,
      SEMANTIC_SYMBOL_KIND_PARAMETER,
      SEMANTIC_SYMBOL_KIND_LOCAL,
      SEMANTIC_SYMBOL_KIND_FIELD,
    };

    enum semantic_type_kind : uint32_t
    {
      SEMANTIC_TYPE_KIND_UNKNOWN,
      SEMANTIC_TYPE_KIND_INT,
    };

    enum semantic_token_classification : uint32_t
    {
      SEMANTIC_TOKEN_CLASSIFICATION_NONE,
      SEMANTIC_TOKEN_CLASSIFICATION_TYPE,
      SEMANTIC_TOKEN_CLASSIFICATION_FUNCTION,
      SEMANTIC_TOKEN_CLASSIFICATION_PARAMETER,
      SEMANTIC_TOKEN_CLASSIFICATION_LOCAL,
      SEMANTIC_TOKEN_CLASSIFICATION_FIELD,
      SEMANTIC_TOKEN_CLASSIFICATION_UNRESOLVED,
    };

    struct semantic_scope
    {
      syntax::syntax_node_id owner_id;
      std::optional<size_t> parent_index;
      std::vector<size_t> symbol_indices;
    };

    struct semantic_symbol
    {
      syntax::syntax_node_id declaration_id;
      std::string name;
      semantic_symbol_kind kind;
      semantic_type_kind type_kind;
      size_t scope_index;
    };

    class semantic_model
    {
    public:
      semantic_model(
        std::vector<semantic_scope> scopes,
        std::vector<semantic_symbol> symbols,
        std::unordered_map<uint64_t, semantic_token_classification> token_classifications,
        std::unordered_map<uint64_t, semantic_type_kind> node_types);

      [[nodiscard]] static std::shared_ptr<const semantic_model> analyze(
        const syntax::syntax_tree& tree, std::string_view text);

      [[nodiscard]] semantic_token_classification get_token_classification(const syntax::syntax_token& token) const;
      [[nodiscard]] semantic_type_kind get_node_type(const syntax::syntax_node_id& node_id) const;
      [[nodiscard]] const std::vector<semantic_scope>& get_scopes() const;
      [[nodiscard]] const std::vector<semantic_symbol>& get_symbols() const;

    private:
      std::vector<semantic_scope> m_scopes;
      std::vector<semantic_symbol> m_symbols;
      std::unordered_map<uint64_t, semantic_token_classification> m_token_classifications;
      std::unordered_map<uint64_t, semantic_type_kind> m_node_types;
    };
  }
}
