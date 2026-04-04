#include "lexer.h"

#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
  using compiler::document::text_change;
  using compiler::syntax::source_span;
  using compiler::syntax::syntax_child_kind;
  using compiler::syntax::syntax_child_reference;
  using compiler::syntax::syntax_diagnostic;
  using compiler::syntax::syntax_diagnostic_code;
  using compiler::syntax::syntax_kind;
  using compiler::syntax::syntax_node;
  using compiler::syntax::syntax_node_flags;
  using compiler::syntax::syntax_node_id;
  using compiler::syntax::syntax_token;
  using compiler::syntax::syntax_tree;

  constexpr size_t NPOS = std::numeric_limits<size_t>::max();

  struct scan_result
  {
    syntax_token token;
    std::vector<syntax_diagnostic> diagnostics;
    size_t next_offset;
  };

  bool is_ascii_digit(char character)
  {
    return (character >= '0') && (character <= '9');
  }

  bool is_ascii_letter(char character)
  {
    return ((character >= 'a') && (character <= 'z')) || ((character >= 'A') && (character <= 'Z'));
  }

  bool is_identifier_start(char character)
  {
    return is_ascii_letter(character) || (character == '_');
  }

  bool is_identifier_continue(char character)
  {
    return is_identifier_start(character) || is_ascii_digit(character);
  }

  bool is_whitespace(char character)
  {
    return (character == ' ') || (character == '\t') || (character == '\r') || (character == '\n')
      || (character == '\f') || (character == '\v');
  }

  syntax_kind classify_identifier(std::string_view text)
  {
    if (text == "int")
    {
      return compiler::syntax::SYNTAX_KIND_INT_KEYWORD;
    }

    if (text == "return")
    {
      return compiler::syntax::SYNTAX_KIND_RETURN_KEYWORD;
    }

    return compiler::syntax::SYNTAX_KIND_IDENTIFIER_TOKEN;
  }

  size_t consume_trivia(std::string_view text, size_t offset, std::vector<syntax_diagnostic>& diagnostics)
  {
    while (offset < text.size())
    {
      if (is_whitespace(text[offset]))
      {
        ++offset;
        continue;
      }

      if ((text[offset] == '/') && ((offset + 1) < text.size()))
      {
        const char next_character = text[offset + 1];
        if (next_character == '/')
        {
          offset += 2;
          while ((offset < text.size()) && (text[offset] != '\r') && (text[offset] != '\n'))
          {
            ++offset;
          }

          continue;
        }

        if (next_character == '*')
        {
          const size_t comment_start = offset;
          offset += 2;
          while ((offset + 1) < text.size())
          {
            if ((text[offset] == '*') && (text[offset + 1] == '/'))
            {
              offset += 2;
              break;
            }

            ++offset;
          }

          if (offset >= text.size())
          {
            diagnostics.push_back(syntax_diagnostic {
              compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK_COMMENT,
              source_span::from_bounds(comment_start, text.size()),
              "unterminated block comment",
            });
            return text.size();
          }

          continue;
        }
      }

      break;
    }

    return offset;
  }

  scan_result scan_next_token(std::string_view text, size_t offset, syntax_node_id token_id)
  {
    std::vector<syntax_diagnostic> diagnostics;
    const size_t leading_start = offset;
    offset = consume_trivia(text, offset, diagnostics);
    const source_span leading_trivia_span = source_span::from_bounds(leading_start, offset);

    if (offset >= text.size())
    {
      return scan_result {
        syntax_token {
          compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN,
          token_id,
          source_span { offset, 0 },
          leading_trivia_span,
          source_span { offset, 0 },
          std::nullopt,
          false,
        },
        std::move(diagnostics),
        text.size(),
      };
    }

    const size_t token_start = offset;
    syntax_kind token_kind = compiler::syntax::SYNTAX_KIND_BAD_TOKEN;
    bool malformed = false;

    if (is_identifier_start(text[offset]))
    {
      ++offset;
      while ((offset < text.size()) && is_identifier_continue(text[offset]))
      {
        ++offset;
      }

      token_kind = classify_identifier(text.substr(token_start, offset - token_start));
    }
    else if (is_ascii_digit(text[offset]))
    {
      ++offset;
      while ((offset < text.size()) && is_ascii_digit(text[offset]))
      {
        ++offset;
      }

      if ((offset < text.size()) && is_identifier_continue(text[offset]))
      {
        malformed = true;
        while ((offset < text.size()) && is_identifier_continue(text[offset]))
        {
          ++offset;
        }

        diagnostics.push_back(syntax_diagnostic {
          compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_INVALID_INTEGER_LITERAL,
          source_span::from_bounds(token_start, offset),
          "invalid integer literal",
        });
      }
      else
      {
        token_kind = compiler::syntax::SYNTAX_KIND_INTEGER_LITERAL_TOKEN;
      }
    }
    else
    {
      switch (text[offset])
      {
      case '(':
        token_kind = compiler::syntax::SYNTAX_KIND_OPEN_PAREN_TOKEN;
        ++offset;
        break;

      case ')':
        token_kind = compiler::syntax::SYNTAX_KIND_CLOSE_PAREN_TOKEN;
        ++offset;
        break;

      case '{':
        token_kind = compiler::syntax::SYNTAX_KIND_OPEN_BRACE_TOKEN;
        ++offset;
        break;

      case '}':
        token_kind = compiler::syntax::SYNTAX_KIND_CLOSE_BRACE_TOKEN;
        ++offset;
        break;

      case ';':
        token_kind = compiler::syntax::SYNTAX_KIND_SEMICOLON_TOKEN;
        ++offset;
        break;

      default:
        malformed = true;
        ++offset;
        diagnostics.push_back(syntax_diagnostic {
          compiler::syntax::SYNTAX_DIAGNOSTIC_CODE_INVALID_TOKEN,
          source_span::from_bounds(token_start, offset),
          "invalid token",
        });
        break;
      }
    }

    if (malformed)
    {
      token_kind = compiler::syntax::SYNTAX_KIND_BAD_TOKEN;
    }

    return scan_result {
      syntax_token {
        token_kind,
        token_id,
        source_span::from_bounds(token_start, offset),
        leading_trivia_span,
        source_span { offset, 0 },
        std::nullopt,
        malformed,
      },
      std::move(diagnostics),
      offset,
    };
  }

  uint64_t find_max_id(const syntax_tree& tree)
  {
    uint64_t max_id = 0;

    for (const syntax_node& node : tree.get_nodes())
    {
      if (node.id.value > max_id)
      {
        max_id = node.id.value;
      }
    }

    for (const syntax_token& token : tree.get_tokens())
    {
      if (token.id.value > max_id)
      {
        max_id = token.id.value;
      }
    }

    return max_id;
  }

  size_t get_owned_start(const syntax_token& token)
  {
    return token.leading_trivia_span.start;
  }

  size_t get_owned_end(const syntax_token& token)
  {
    return token.span.get_end();
  }

  size_t find_token_index_for_offset(
    const std::vector<syntax_token>& tokens, size_t offset, bool prefer_previous_at_boundary)
  {
    for (size_t token_index = 0; token_index < tokens.size(); ++token_index)
    {
      const size_t owned_start = get_owned_start(tokens[token_index]);
      const size_t owned_end = get_owned_end(tokens[token_index]);
      if (offset < owned_start)
      {
        return token_index;
      }

      if (offset < owned_end)
      {
        return token_index;
      }

      if (prefer_previous_at_boundary && (offset == owned_end))
      {
        return token_index;
      }
    }

    return tokens.empty() ? 0 : (tokens.size() - 1);
  }

  size_t find_token_index_by_owned_start(const std::vector<syntax_token>& tokens, size_t owned_start, size_t hint_index)
  {
    for (size_t token_index = hint_index; token_index < tokens.size(); ++token_index)
    {
      const size_t token_owned_start = get_owned_start(tokens[token_index]);
      if (token_owned_start == owned_start)
      {
        return token_index;
      }

      if (token_owned_start > owned_start)
      {
        break;
      }
    }

    return NPOS;
  }

  bool remainder_matches(std::string_view old_text, size_t old_offset, std::string_view new_text, size_t new_offset)
  {
    return old_text.substr(old_offset) == new_text.substr(new_offset);
  }

  size_t shift_offset(size_t offset, int64_t delta)
  {
    if (delta >= 0)
    {
      const uint64_t positive_delta = static_cast<uint64_t>(delta);
      if (positive_delta > (std::numeric_limits<size_t>::max() - offset))
      {
        throw std::overflow_error("shifted offset overflow");
      }

      return offset + static_cast<size_t>(positive_delta);
    }

    const uint64_t negative_delta = static_cast<uint64_t>(-delta);
    if (negative_delta > offset)
    {
      throw std::out_of_range("shifted offset underflow");
    }

    return offset - static_cast<size_t>(negative_delta);
  }

  source_span shift_span(const source_span& span, int64_t delta)
  {
    return source_span { shift_offset(span.start, delta), span.length };
  }

  syntax_token clone_token_for_flat_tree(const syntax_token& token)
  {
    syntax_token copy = token;
    copy.parent_index = 0;
    return copy;
  }

  syntax_token shift_token_for_flat_tree(const syntax_token& token, int64_t delta)
  {
    syntax_token copy = token;
    copy.span = shift_span(copy.span, delta);
    copy.leading_trivia_span = shift_span(copy.leading_trivia_span, delta);
    copy.trailing_trivia_span = shift_span(copy.trailing_trivia_span, delta);
    copy.parent_index = 0;
    return copy;
  }

  syntax_diagnostic shift_diagnostic(const syntax_diagnostic& diagnostic, int64_t delta)
  {
    syntax_diagnostic copy = diagnostic;
    copy.span = shift_span(copy.span, delta);
    return copy;
  }

  std::shared_ptr<const syntax_tree> build_flat_tree(
    uint64_t source_generation,
    size_t text_size,
    std::vector<syntax_token> tokens,
    std::vector<syntax_diagnostic> diagnostics,
    uint64_t root_id)
  {
    if (tokens.empty())
    {
      throw std::invalid_argument("lexer requires at least one token");
    }

    std::vector<syntax_child_reference> children;
    children.reserve(tokens.size());
    for (size_t token_index = 0; token_index < tokens.size(); ++token_index)
    {
      tokens[token_index].parent_index = 0;
      children.push_back(syntax_child_reference { syntax_child_kind::SYNTAX_CHILD_KIND_TOKEN, token_index });
    }

    std::vector<syntax_node> nodes;
    nodes.push_back(syntax_node {
      compiler::syntax::SYNTAX_KIND_TRANSLATION_UNIT,
      syntax_node_id { root_id },
      source_span { 0, text_size },
      std::nullopt,
      std::move(children),
      syntax_node_flags::SYNTAX_NODE_FLAG_NONE,
    });

    return std::make_shared<const syntax_tree>(
      source_generation, std::move(nodes), std::move(tokens), 0, std::move(diagnostics));
  }
}

std::shared_ptr<const syntax_tree> compiler::syntax::lexer::lex(uint64_t source_generation, std::string_view text)
{
  uint64_t next_id = 1;
  std::vector<syntax_token> tokens;
  std::vector<syntax_diagnostic> diagnostics;
  size_t offset = 0;

  while (true)
  {
    scan_result result = scan_next_token(text, offset, syntax_node_id { next_id });
    ++next_id;
    result.token.parent_index = 0;
    offset = result.next_offset;

    for (const syntax_diagnostic& diagnostic : result.diagnostics)
    {
      diagnostics.push_back(diagnostic);
    }

    const bool is_eof = result.token.kind == SYNTAX_KIND_END_OF_FILE_TOKEN;
    tokens.push_back(std::move(result.token));
    if (is_eof)
    {
      break;
    }
  }

  return build_flat_tree(source_generation, text.size(), std::move(tokens), std::move(diagnostics), next_id);
}

std::shared_ptr<const syntax_tree> compiler::syntax::lexer::relex(
  uint64_t source_generation,
  std::string_view old_text,
  const syntax_tree& old_tree,
  const text_change& change,
  std::string_view new_text)
{
  if (old_tree.get_token_count() == 0)
  {
    return lex(source_generation, new_text);
  }

  const int64_t delta = change.get_delta();
  uint64_t next_id = find_max_id(old_tree) + 1;
  const std::vector<syntax_token>& old_tokens = old_tree.get_tokens();
  const std::vector<syntax_diagnostic>& old_diagnostics = old_tree.get_diagnostics();

  if (old_text == new_text)
  {
    std::vector<syntax_token> tokens;
    tokens.reserve(old_tokens.size());
    for (const syntax_token& token : old_tokens)
    {
      tokens.push_back(clone_token_for_flat_tree(token));
    }

    std::vector<syntax_diagnostic> diagnostics;
    diagnostics.reserve(old_diagnostics.size());
    for (const syntax_diagnostic& diagnostic : old_diagnostics)
    {
      if (compiler::syntax::is_lexical_diagnostic_code(diagnostic.code))
      {
        diagnostics.push_back(diagnostic);
      }
    }

    return build_flat_tree(source_generation, new_text.size(), std::move(tokens), std::move(diagnostics), next_id);
  }

  const size_t probe_offset = (change.start_offset == 0) ? 0 : (change.start_offset - static_cast<size_t>(1));
  const size_t start_index = find_token_index_for_offset(old_tokens, probe_offset, true);
  const size_t old_relex_start = get_owned_start(old_tokens[start_index]);

  std::vector<syntax_token> tokens;
  tokens.reserve(old_tokens.size() + 8);
  for (size_t token_index = 0; token_index < start_index; ++token_index)
  {
    tokens.push_back(clone_token_for_flat_tree(old_tokens[token_index]));
  }

  std::vector<syntax_diagnostic> diagnostics;
  diagnostics.reserve(old_diagnostics.size() + 4);
  for (const syntax_diagnostic& diagnostic : old_diagnostics)
  {
    if (compiler::syntax::is_lexical_diagnostic_code(diagnostic.code) && (diagnostic.span.get_end() <= old_relex_start))
    {
      diagnostics.push_back(diagnostic);
    }
  }

  size_t new_offset = old_relex_start;
  size_t old_hint_index = start_index;
  size_t sync_index = NPOS;

  while (true)
  {
    scan_result result = scan_next_token(new_text, new_offset, syntax_node_id { next_id });
    ++next_id;
    result.token.parent_index = 0;
    new_offset = result.next_offset;

    for (const syntax_diagnostic& diagnostic : result.diagnostics)
    {
      diagnostics.push_back(diagnostic);
    }

    const bool is_eof = result.token.kind == SYNTAX_KIND_END_OF_FILE_TOKEN;
    tokens.push_back(std::move(result.token));

    if ((delta <= static_cast<int64_t>(new_offset)) || (delta < 0))
    {
      const int64_t old_offset_candidate = static_cast<int64_t>(new_offset) - delta;
      if (
        (old_offset_candidate >= 0)
        && (static_cast<uint64_t>(old_offset_candidate) <= static_cast<uint64_t>(old_text.size())))
      {
        const size_t old_offset = static_cast<size_t>(old_offset_candidate);
        const size_t candidate_index = find_token_index_by_owned_start(old_tokens, old_offset, old_hint_index);
        if ((candidate_index != NPOS) && remainder_matches(old_text, old_offset, new_text, new_offset))
        {
          sync_index = candidate_index;
          break;
        }
      }
    }

    if (is_eof)
    {
      break;
    }
  }

  if (sync_index != NPOS)
  {
    const size_t old_resume_offset = get_owned_start(old_tokens[sync_index]);
    const bool skip_synced_eof = !tokens.empty()
      && (tokens.back().kind == compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN)
      && (old_tokens[sync_index].kind == compiler::syntax::SYNTAX_KIND_END_OF_FILE_TOKEN);
    const size_t resume_index = skip_synced_eof ? (sync_index + static_cast<size_t>(1)) : sync_index;

    for (size_t token_index = resume_index; token_index < old_tokens.size(); ++token_index)
    {
      tokens.push_back(shift_token_for_flat_tree(old_tokens[token_index], delta));
    }

    for (const syntax_diagnostic& diagnostic : old_diagnostics)
    {
      if (compiler::syntax::is_lexical_diagnostic_code(diagnostic.code) && (diagnostic.span.start >= old_resume_offset))
      {
        diagnostics.push_back(shift_diagnostic(diagnostic, delta));
      }
    }
  }

  return build_flat_tree(source_generation, new_text.size(), std::move(tokens), std::move(diagnostics), next_id);
}
