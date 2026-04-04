# Live Frontend Architecture Plan

## Objective

Build a live, editor-driven frontend for mIDE that parses code while the user is typing and keeps one tree as the source of truth for:

- editor structure,
- syntax highlighting,
- outline and diagnostics,
- semantic analysis, and
- compilation.

The initial implementation may target a subset of C, but the architecture must be able to grow into a full frontend without being replaced.

## Decisions Already Made

- The frontend will parse continuously while typing.
- The first implementation may support only a subset of C.
- Syntax highlighting must be derived from parsed structure, not from standalone regex or text-only rules.
- The parser should be incremental from the start.
- The same tree must remain the source of truth for both the editor and compilation.

## Bootstrap Parser Target

The first accepted program should be the smallest useful end-to-end case:

```c
int main() { return 1; }
```

This bootstrap target is intentionally narrow. The first parser iteration only needs to recognize:

- the `int` type specifier,
- a top-level function definition,
- the identifier `main`,
- an empty parameter list,
- a compound statement,
- a `return` statement, and
- an integer literal.

The point of this milestone is not language coverage. The point is to prove that one live tree can be created while typing, reused incrementally across edits, highlighted from structure, and lowered by the compiler pipeline.

## Core Architectural Rule

There is exactly one tree per document snapshot.

That tree must contain enough information to support both editor features and compilation. The compiler must not create a separate compiler-only AST with different node shapes. Instead, later passes may attach auxiliary data to nodes in the same tree snapshot, such as symbol resolution, type information, constant values, and diagnostics.

This means the tree should be treated as a full-fidelity AST for mIDE, with token leaves included so highlighting and precise source mapping do not depend on a second representation.

## Frontend Snapshot Model

Each edit produces a new immutable document snapshot.

Each snapshot owns:

- the document text,
- a line index,
- the token stream represented as token leaf nodes,
- the parsed tree,
- diagnostics created during lexing or parsing,
- optional semantic annotations keyed by node identity, and
- optional lowering metadata keyed by node identity.

Older snapshots may remain alive briefly so the UI, parser, and compiler can work without racing each other.

## Tree Shape

The unified tree should contain both structural nodes and token leaves.

### Required node data

Every node should store:

- a stable node kind,
- a byte span in the document,
- child links,
- a pointer or index to the parent when needed for navigation,
- flags for missing or recovered syntax,
- a snapshot version or generation identifier, and
- a stable node identity that can be used by semantic tables.

### Token leaves

Tokens should be stored as leaves in the same tree rather than in a disconnected token array. This keeps syntax highlighting tree-driven while still allowing precise token-level coloring.

Token leaves should include:

- token kind,
- byte span,
- trivia ownership policy,
- exact source text lookup through the snapshot, and
- error flags for malformed tokens.

### Node categories for the first implementation

The bootstrap parser only needs these node kinds:

- translation unit,
- function definition,
- function declarator,
- parameter list,
- compound statement,
- return statement,
- identifier,
- type specifier,
- literal expression,
- error node,
- missing node, and
- token leaf kinds.

After that milestone works, the next subset can grow into the following broader node kinds:

- translation unit,
- function definition,
- parameter list,
- compound statement,
- declaration,
- declarator,
- identifier,
- type specifier,
- return statement,
- if statement,
- binary expression,
- unary expression,
- call expression,
- literal expression,
- member expression,
- assignment expression,
- error node,
- missing node, and
- token leaf kinds.

## Incremental Parsing Strategy

The parser should be designed around structural reuse rather than full reparsing of the whole file after every keystroke.

### Edit flow

1. The editor produces a text change range.
2. The document model creates a new snapshot.
3. The lexer re-lexes only the affected region plus a small recovery window.
4. The parser finds the smallest enclosing reparse boundary.
5. Unchanged subtrees outside that boundary are reused.
6. New nodes are created only for affected regions.
7. Highlighting, outline data, diagnostics, and semantic work are refreshed from the new tree.

### Reparse boundaries

For the first version, reparsing can stop at declaration or function boundaries instead of trying to repair arbitrary interior nodes. That still counts as incremental parsing and is much safer than whole-file reparsing.

Suggested early boundaries:

- translation unit item,
- function definition,
- declaration statement, and
- compound statement.

## Error Tolerance

The parser must accept incomplete code because the editor will spend most of its life in syntactically invalid states.

The tree therefore needs:

- missing nodes for expected constructs that are not yet present,
- error nodes for malformed regions,
- token retention for skipped or unexpected tokens, and
- diagnostics that can be updated incrementally.

Compilation and lowering should only run on regions or snapshots that satisfy the minimum validity rules for the current stage.

## Highlighting Model

Highlighting should be produced from the unified tree, not from raw text patterns.

The final classification of each displayed word or token should come from these sources, in this order:

1. Token leaf kind.
2. Structural parentage in the tree.
3. Resolved symbol or type information attached to the same tree snapshot.

Examples:

- An identifier token under a function declarator is highlighted as a function name.
- An identifier token under a type specifier is highlighted as a type.
- An identifier token in an expression is highlighted as a variable, parameter, field, or unresolved symbol depending on semantic resolution.
- A keyword-looking token inside a malformed region remains classified by its actual parsed role, not by a text-only rule.

The important point is that tokens are still highlighted at token granularity, but their meaning is inherited from the AST path and node annotations.

## Semantic Model Without a Second AST

Semantic analysis should walk the same tree used by the editor.

Instead of building a separate bound tree, semantic analysis should produce side tables attached to node identities in the snapshot, such as:

- symbol definitions,
- symbol references,
- scopes,
- resolved types,
- constant values,
- storage classes, and
- overload or callable metadata if the language later grows in that direction.

This keeps one source of truth while still allowing semantic information to be cached, invalidated, and recomputed independently.

## Compilation Model

Lowering and code generation should consume the same tree snapshot.

The compilation pipeline for an accepted snapshot should look like this:

1. Parse the document into the unified tree.
2. Run semantic analysis and attach semantic tables to node identities.
3. Lower directly from the unified tree into IR.
4. Generate assembly from IR.
5. Feed assembly, diagnostics, and source mapping back into the editor.

No separate compiler-only AST should be introduced between steps 2 and 3.

## Proposed Source Layout

When implementation starts, add dedicated frontend code under `src/compiler/`.

Suggested layout:

```text
src/compiler/
  document/
    document_snapshot.h
    document_snapshot.cpp
    text_change.h
    line_index.h
  syntax/
    syntax_kind.h
    syntax_node.h
    syntax_token.h
    syntax_tree.h
    lexer.h
    lexer.cpp
    parser.h
    parser.cpp
    parse_result.h
  semantic/
    semantic_model.h
    semantic_model.cpp
    scope_table.h
    symbol_table.h
    type_table.h
  lowering/
    lowering_context.h
    lowering_context.cpp
```

The existing UI code should depend on the frontend through small interfaces rather than knowing parser internals.

## Implementation Stages

### Stage 1: Snapshot and Tree Foundations

Goal: define the persistent document and node model.

Deliverables:

- immutable document snapshot type,
- text change representation,
- line index,
- syntax kind enumeration,
- base syntax node and token types,
- syntax tree container, and
- stable node identity scheme.

Done when:

- a document snapshot can be created from source text,
- nodes and tokens can represent spans without copying source text, and
- a tree can be inspected by editor code without compiler-specific dependencies.

### Stage 2: Incremental Lexer

Goal: tokenize edited text regions and reuse unaffected token ranges.

Deliverables:

- lexer for the initial C subset,
- token leaf creation,
- trivia handling,
- malformed token diagnostics, and
- incremental re-lex window logic.

Done when:

- single-line edits do not force full-file re-lexing,
- comments, literals, identifiers, punctuation, and keywords are tokenized correctly for the subset, and
- token spans remain stable outside the edited window.

### Stage 3: Incremental Parser for the Initial C Subset

Goal: build the unified tree and recover from incomplete input.

The first checkpoint for this stage is the bootstrap program `int main() { return 1; }`.

Deliverables:

- parser support for the bootstrap program shape,
- parser for declarations, function definitions, statements, and basic expressions,
- missing and error node support,
- subtree reuse across edits, and
- parse diagnostics.

Done when:

- `int main() { return 1; }` parses into the unified tree and survives small edits such as changing `1` to `2` without reparsing unrelated regions,
- edits inside one function do not rebuild unrelated top-level functions,
- incomplete constructs still produce a navigable tree, and
- the tree is precise enough to distinguish function names, type positions, declarations, and expression identifiers.

### Stage 4: Editor Integration

Goal: drive editor views from the tree instead of hardcoded sample data.

Deliverables:

- editor document state object,
- token classification derived from tree structure,
- outline generation from syntax nodes,
- status bar parse state, and
- invalid region highlighting or diagnostics display.

Done when:

- the editor panel renders real source from a snapshot,
- visible words are colored from the tree,
- the outline panel is generated from parsed declarations, and
- updates happen live while typing.

### Stage 5: Semantic Passes on the Same Tree

Goal: resolve names and types without introducing a second AST.

Deliverables:

- scope builder,
- symbol tables,
- name resolution,
- type resolution for the initial subset, and
- semantic token classification.

Done when:

- identifiers can be classified as type, function, parameter, local, field, or unresolved symbol, and
- those classifications are attached to the same snapshot used by the editor.

### Stage 6: Lowering and Live Compilation

Goal: compile directly from the unified tree.

Deliverables:

- lowering context that walks syntax nodes plus semantic tables,
- IR generation for the initial subset,
- source-to-IR mapping,
- assembly generation hooks, and
- editor feedback for compile success or failure.

Done when:

- a valid function in the subset can be parsed, resolved, lowered, and shown in the assembly panel from the same snapshot.

### Stage 7: Expand the Language Surface

Goal: grow from the initial subset toward broader C coverage.

Priority order:

- more declaration forms,
- pointer and array declarators,
- structs and enums,
- initializer expressions,
- loops and switch,
- typedefs and user-defined type names,
- preprocessor strategy, and
- multi-file translation unit support.

Each expansion should preserve the same snapshot and tree model rather than adding a parallel representation.

## Non-Goals for the First Implementation

The first staged implementation should not try to solve all frontend problems at once.

Explicit non-goals:

- full C language coverage,
- macro expansion correctness beyond a minimal placeholder strategy,
- cross-file semantic resolution,
- full optimizer design,
- debugger metadata generation, and
- background compilation of every invalid snapshot.

## Initial Success Criteria

The plan is working if the following become true in sequence:

1. Typing in the editor creates new snapshots rather than mutating global parser state.
2. The bootstrap program `int main() { return 1; }` parses into a single unified tree.
3. The parser reuses most of the previous tree after small edits.
4. Highlighting is derived from token leaves plus tree structure.
5. Outline and diagnostics are derived from the same tree.
6. Semantic classification enriches highlighting without replacing the tree.
7. The compiler lowers directly from that tree snapshot.

## Practical Notes

- Store source as UTF-8 and measure spans in bytes first. Display code can convert to columns separately.
- Prefer immutable nodes with structural sharing because that makes incremental parsing and background work much simpler.
- Keep the parser deterministic and non-allocating in hot loops where practical, but do not optimize before the data model is stable.
- Build test inputs around partial and broken code early, not only valid code.

## Immediate Next Step

Start with Stage 1 and implement the snapshot, syntax kind, token, node, and tree types before touching editor rendering. Until those types exist, the parser and UI will not have a stable contract.
