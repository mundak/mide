#pragma once

#include <cstddef>
#include <cstdint>

namespace compiler
{
  namespace syntax
  {
    struct syntax_node_id
    {
      uint64_t value;

      [[nodiscard]] bool is_valid() const;
    };

    enum syntax_child_kind : uint8_t
    {
      SYNTAX_CHILD_KIND_NODE,
      SYNTAX_CHILD_KIND_TOKEN,
    };

    struct syntax_child_reference
    {
      syntax_child_kind kind;
      size_t index;

      [[nodiscard]] bool is_node() const;
      [[nodiscard]] bool is_token() const;
    };
  }
}
