#pragma once

#include "syntax_tree.h"

#include <memory>
#include <string_view>

namespace compiler
{
  namespace syntax
  {
    class parser
    {
    public:
      [[nodiscard]] static std::shared_ptr<const syntax_tree> parse(const syntax_tree& token_tree, size_t text_size);
    };
  }
}
