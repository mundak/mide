#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace compiler
{
  namespace document
  {
    struct text_change
    {
      size_t start_offset;
      size_t end_offset;
      std::string replacement_text;

      [[nodiscard]] int64_t get_delta() const;
      [[nodiscard]] size_t get_removed_length() const;
    };
  }
}
