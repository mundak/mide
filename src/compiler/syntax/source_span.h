#pragma once

#include <cstddef>

namespace compiler
{
  namespace syntax
  {
    struct source_span
    {
      size_t start;
      size_t length;

      [[nodiscard]] bool contains(size_t offset) const;
      [[nodiscard]] size_t get_end() const;
      [[nodiscard]] static source_span from_bounds(size_t start_offset, size_t end_offset);
    };
  }
}
