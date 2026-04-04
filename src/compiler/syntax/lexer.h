#pragma once

#include "compiler/document/text_change.h"
#include "syntax_tree.h"

#include <memory>
#include <string_view>

namespace compiler
{
  namespace syntax
  {
    class lexer
    {
    public:
      [[nodiscard]] static std::shared_ptr<const syntax_tree> lex(uint64_t source_generation, std::string_view text);

      [[nodiscard]] static std::shared_ptr<const syntax_tree> relex(
        uint64_t source_generation,
        std::string_view old_text,
        const syntax_tree& old_tree,
        const document::text_change& change,
        std::string_view new_text);
    };
  }
}
