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
      SYNTAX_DIAGNOSTIC_INVALID_TOKEN,
      SYNTAX_DIAGNOSTIC_INVALID_INTEGER_LITERAL,
      SYNTAX_DIAGNOSTIC_UNTERMINATED_BLOCK_COMMENT,
    };

    struct syntax_diagnostic
    {
      syntax_diagnostic_code code;
      source_span span;
      std::string message;
    };
  }
}
