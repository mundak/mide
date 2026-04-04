#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace compiler
{
  namespace document
  {
    class line_index
    {
    public:
      line_index();
      explicit line_index(std::string_view text);

      [[nodiscard]] size_t get_line_count() const;
      [[nodiscard]] size_t get_line_end(size_t line_number) const;
      [[nodiscard]] size_t get_line_index(size_t offset) const;
      [[nodiscard]] size_t get_line_start(size_t line_number) const;

    private:
      void rebuild(std::string_view text);

      std::vector<size_t> m_line_ends;
      std::vector<size_t> m_line_starts;
      size_t m_text_size;
    };
  }
}
