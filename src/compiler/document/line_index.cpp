#include "line_index.h"

#include <algorithm>
#include <stdexcept>

compiler::document::line_index::line_index()
  : m_text_size(0)
{
  m_line_ends.push_back(0);
  m_line_starts.push_back(0);
}

compiler::document::line_index::line_index(std::string_view text)
{
  rebuild(text);
}

size_t compiler::document::line_index::get_line_count() const
{
  return m_line_starts.size();
}

size_t compiler::document::line_index::get_line_end(size_t line_number) const
{
  if (line_number >= m_line_ends.size())
  {
    throw std::out_of_range("line_number");
  }

  return m_line_ends[line_number];
}

size_t compiler::document::line_index::get_line_index(size_t offset) const
{
  if (offset > m_text_size)
  {
    throw std::out_of_range("offset");
  }

  const std::vector<size_t>::const_iterator upper
    = std::upper_bound(m_line_starts.begin(), m_line_starts.end(), offset);
  if (upper == m_line_starts.begin())
  {
    return 0;
  }

  return static_cast<size_t>(std::distance(m_line_starts.begin(), upper) - 1);
}

size_t compiler::document::line_index::get_line_start(size_t line_number) const
{
  if (line_number >= m_line_starts.size())
  {
    throw std::out_of_range("line_number");
  }

  return m_line_starts[line_number];
}

void compiler::document::line_index::rebuild(std::string_view text)
{
  m_line_ends.clear();
  m_line_starts.clear();
  m_text_size = text.size();

  size_t line_start = 0;

  for (size_t index = 0; index < text.size(); ++index)
  {
    if (text[index] == '\n')
    {
      size_t line_end = index;
      if ((line_end > line_start) && (text[line_end - 1] == '\r'))
      {
        --line_end;
      }

      m_line_starts.push_back(line_start);
      m_line_ends.push_back(line_end);
      line_start = index + 1;
    }
  }

  m_line_starts.push_back(line_start);
  m_line_ends.push_back(text.size());
}
