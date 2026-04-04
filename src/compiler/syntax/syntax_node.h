#pragma once

#include "source_span.h"
#include "syntax_identity.h"
#include "syntax_kind.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace compiler
{
  namespace syntax
  {
    enum syntax_node_flags : uint32_t
    {
      SYNTAX_NODE_FLAG_NONE = 0,
      SYNTAX_NODE_FLAG_MISSING = 1u << 0,
      SYNTAX_NODE_FLAG_RECOVERED = 1u << 1,
    };

    [[nodiscard]] syntax_node_flags operator|(syntax_node_flags left, syntax_node_flags right);

    [[nodiscard]] bool has_flag(syntax_node_flags flags, syntax_node_flags flag);

    struct syntax_node
    {
      syntax_kind kind;
      syntax_node_id id;
      source_span span;
      std::optional<size_t> parent_index;
      std::vector<syntax_child_reference> children;
      syntax_node_flags flags;
    };
  }
}
