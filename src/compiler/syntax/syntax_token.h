#pragma once

#include "source_span.h"
#include "syntax_identity.h"
#include "syntax_kind.h"

#include <cstddef>
#include <optional>

namespace compiler
{
  namespace syntax
  {
    struct syntax_token
    {
      syntax_kind kind;
      syntax_node_id id;
      source_span span;
      source_span leading_trivia_span;
      source_span trailing_trivia_span;
      std::optional<size_t> parent_index;
      bool malformed;
    };
  }
}
