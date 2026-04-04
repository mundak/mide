#pragma once

#include "source_span.h"

#include <cstdint>
#include <string>

namespace compiler
{
  namespace syntax
  {
    enum syntax_diagnostic_code : uint32_t
    {
      SYNTAX_DIAGNOSTIC_CODE_INVALID_TOKEN,
      SYNTAX_DIAGNOSTIC_CODE_INVALID_INTEGER_LITERAL,
      SYNTAX_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK_COMMENT,
      SYNTAX_DIAGNOSTIC_CODE_INTERNAL_ERROR,
      SYNTAX_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN,
      SYNTAX_DIAGNOSTIC_CODE_EXPECTED_TOKEN,
      SYNTAX_DIAGNOSTIC_CODE_EXPECTED_DECLARATION,
      SYNTAX_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION,
    };

    struct syntax_diagnostic
    {
      syntax_diagnostic_code code;
      source_span span;
      std::string message;
    };

    [[nodiscard]] bool is_lexical_diagnostic_code(syntax_diagnostic_code code);
  }
}
