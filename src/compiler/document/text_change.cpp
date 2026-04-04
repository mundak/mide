#include "text_change.h"

#include <stdexcept>

int64_t compiler::document::text_change::get_delta() const
{
  return static_cast<int64_t>(replacement_text.size()) - static_cast<int64_t>(get_removed_length());
}

size_t compiler::document::text_change::get_removed_length() const
{
  if (end_offset < start_offset)
  {
    throw std::invalid_argument("text_change range");
  }

  return end_offset - start_offset;
}
