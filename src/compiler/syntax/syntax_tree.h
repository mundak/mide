#pragma once

#include "syntax_node.h"
#include "syntax_token.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace compiler
{
  namespace syntax
  {
    class syntax_tree
    {
    public:
      syntax_tree(
        uint64_t source_generation,
        std::vector<syntax_node> nodes,
        std::vector<syntax_token> tokens,
        std::optional<size_t> root_node_index);

      [[nodiscard]] static std::shared_ptr<const syntax_tree> create_empty(uint64_t source_generation);

      [[nodiscard]] const syntax_node& get_node(size_t index) const;
      [[nodiscard]] size_t get_node_count() const;
      [[nodiscard]] const std::vector<syntax_node>& get_nodes() const;
      [[nodiscard]] const syntax_node& get_root() const;
      [[nodiscard]] std::optional<size_t> get_root_index() const;
      [[nodiscard]] uint64_t get_source_generation() const;
      [[nodiscard]] const syntax_token& get_token(size_t index) const;
      [[nodiscard]] size_t get_token_count() const;
      [[nodiscard]] const std::vector<syntax_token>& get_tokens() const;
      [[nodiscard]] bool has_root() const;

    private:
      uint64_t m_source_generation;
      std::vector<syntax_node> m_nodes;
      std::vector<syntax_token> m_tokens;
      std::optional<size_t> m_root_node_index;
    };
  }
}
