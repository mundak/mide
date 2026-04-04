#pragma once

#include "line_index.h"
#include "text_change.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace compiler
{
  namespace semantic
  {
    class semantic_model;
  }

  namespace syntax
  {
    class syntax_tree;
  }

  namespace document
  {
    class document_snapshot
    {
    public:
      document_snapshot(uint64_t generation, std::string text);
      document_snapshot(
        uint64_t generation, std::string text, const std::shared_ptr<const syntax::syntax_tree>& syntax_tree);

      [[nodiscard]] static document_snapshot create_initial(std::string text);

      [[nodiscard]] document_snapshot apply_change(const text_change& change) const;
      [[nodiscard]] document_snapshot with_syntax_tree(
        const std::shared_ptr<const syntax::syntax_tree>& syntax_tree) const;

      [[nodiscard]] uint64_t get_generation() const;
      [[nodiscard]] const line_index& get_line_index() const;
      [[nodiscard]] std::shared_ptr<const semantic::semantic_model> get_semantic_model() const;
      [[nodiscard]] std::shared_ptr<const syntax::syntax_tree> get_syntax_tree() const;
      [[nodiscard]] const std::string& get_text() const;
      [[nodiscard]] size_t get_text_size() const;

    private:
      static std::string apply_change_to_text(const std::string& text, const text_change& change);

      uint64_t m_generation;
      std::string m_text;
      class line_index m_line_index;
      std::shared_ptr<const syntax::syntax_tree> m_syntax_tree;
      std::shared_ptr<const semantic::semantic_model> m_semantic_model;
    };
  }
}
