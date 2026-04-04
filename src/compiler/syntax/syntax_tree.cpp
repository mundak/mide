#include "syntax_tree.h"

#include <stdexcept>
#include <unordered_set>

namespace
{
  void visit_node(
    const std::vector<compiler::syntax::syntax_node>& nodes,
    const std::vector<compiler::syntax::syntax_token>& tokens,
    size_t node_index,
    std::vector<uint8_t>& node_states,
    std::vector<bool>& visited_tokens)
  {
    if (node_states[node_index] == 1)
    {
      throw std::invalid_argument("syntax tree cycle");
    }

    if (node_states[node_index] == 2)
    {
      throw std::invalid_argument("syntax node referenced more than once");
    }

    node_states[node_index] = 1;

    for (const compiler::syntax::syntax_child_reference& child : nodes[node_index].children)
    {
      if (child.is_node())
      {
        visit_node(nodes, tokens, child.index, node_states, visited_tokens);
        continue;
      }

      if (visited_tokens[child.index])
      {
        throw std::invalid_argument("syntax token referenced more than once");
      }

      visited_tokens[child.index] = true;
    }

    node_states[node_index] = 2;
  }

  void validate_parent_index(size_t node_count, const std::optional<size_t>& parent_index)
  {
    if (parent_index.has_value() && (*parent_index >= node_count))
    {
      throw std::out_of_range("parent_index");
    }
  }

  void validate_child_reference(
    const std::vector<compiler::syntax::syntax_node>& nodes,
    const std::vector<compiler::syntax::syntax_token>& tokens,
    const compiler::syntax::syntax_child_reference& child)
  {
    if (child.is_node())
    {
      if (child.index >= nodes.size())
      {
        throw std::out_of_range("node child index");
      }

      return;
    }

    if (child.is_token())
    {
      if (child.index >= tokens.size())
      {
        throw std::out_of_range("token child index");
      }

      return;
    }

    throw std::invalid_argument("child kind");
  }

  void validate_identity(std::unordered_set<uint64_t>& seen_ids, const compiler::syntax::syntax_node_id& id)
  {
    if (!id.is_valid())
    {
      throw std::invalid_argument("syntax_node_id");
    }

    if (!seen_ids.insert(id.value).second)
    {
      throw std::invalid_argument("duplicate syntax_node_id");
    }
  }
}

compiler::syntax::syntax_tree::syntax_tree(
  uint64_t source_generation,
  std::vector<syntax_node> nodes,
  std::vector<syntax_token> tokens,
  std::optional<size_t> root_node_index)
  : m_source_generation(source_generation)
  , m_nodes(std::move(nodes))
  , m_tokens(std::move(tokens))
  , m_root_node_index(root_node_index)
{
  if (m_root_node_index.has_value() && (*m_root_node_index >= m_nodes.size()))
  {
    throw std::out_of_range("root_node_index");
  }

  if (m_nodes.empty())
  {
    if (!m_tokens.empty())
    {
      throw std::invalid_argument("token-only syntax tree");
    }

    if (m_root_node_index.has_value())
    {
      throw std::invalid_argument("empty syntax tree root");
    }

    return;
  }

  if (!m_root_node_index.has_value())
  {
    throw std::invalid_argument("non-empty syntax tree requires a root");
  }

  std::unordered_set<uint64_t> seen_ids;
  size_t root_count = 0;

  for (size_t node_index = 0; node_index < m_nodes.size(); ++node_index)
  {
    const syntax_node& node = m_nodes[node_index];
    if (is_token_kind(node.kind))
    {
      throw std::invalid_argument("syntax_node kind");
    }

    validate_identity(seen_ids, node.id);
    validate_parent_index(m_nodes.size(), node.parent_index);
    if (!node.parent_index.has_value())
    {
      ++root_count;
    }

    for (const syntax_child_reference& child : node.children)
    {
      validate_child_reference(m_nodes, m_tokens, child);

      if (child.is_node() && (m_nodes[child.index].parent_index != node_index))
      {
        throw std::invalid_argument("node child parent mismatch");
      }

      if (child.is_token() && (m_tokens[child.index].parent_index != node_index))
      {
        throw std::invalid_argument("token child parent mismatch");
      }
    }
  }

  for (const syntax_token& token : m_tokens)
  {
    if (!is_token_kind(token.kind))
    {
      throw std::invalid_argument("syntax_token kind");
    }

    validate_identity(seen_ids, token.id);
    validate_parent_index(m_nodes.size(), token.parent_index);
    if (!token.parent_index.has_value())
    {
      throw std::invalid_argument("syntax_token requires a parent");
    }
  }

  if (root_count != 1)
  {
    throw std::invalid_argument("syntax tree must have exactly one root");
  }

  if (m_root_node_index.has_value() && m_nodes[*m_root_node_index].parent_index.has_value())
  {
    throw std::invalid_argument("root node has parent");
  }

  size_t discovered_root_index = m_nodes.size();
  for (size_t node_index = 0; node_index < m_nodes.size(); ++node_index)
  {
    if (!m_nodes[node_index].parent_index.has_value())
    {
      discovered_root_index = node_index;
      break;
    }
  }

  if (*m_root_node_index != discovered_root_index)
  {
    throw std::invalid_argument("root node index mismatch");
  }

  std::vector<uint8_t> node_states(m_nodes.size(), 0);
  std::vector<bool> visited_tokens(m_tokens.size(), false);
  visit_node(m_nodes, m_tokens, *m_root_node_index, node_states, visited_tokens);

  for (uint8_t node_state : node_states)
  {
    if (node_state != 2)
    {
      throw std::invalid_argument("unreachable syntax node");
    }
  }

  for (bool token_visited : visited_tokens)
  {
    if (!token_visited)
    {
      throw std::invalid_argument("unreachable syntax token");
    }
  }
}

std::shared_ptr<const compiler::syntax::syntax_tree> compiler::syntax::syntax_tree::create_empty(
  uint64_t source_generation)
{
  return std::make_shared<const syntax_tree>(
    source_generation, std::vector<syntax_node> {}, std::vector<syntax_token> {}, std::nullopt);
}

const compiler::syntax::syntax_node& compiler::syntax::syntax_tree::get_node(size_t index) const
{
  if (index >= m_nodes.size())
  {
    throw std::out_of_range("index");
  }

  return m_nodes[index];
}

size_t compiler::syntax::syntax_tree::get_node_count() const
{
  return m_nodes.size();
}

const std::vector<compiler::syntax::syntax_node>& compiler::syntax::syntax_tree::get_nodes() const
{
  return m_nodes;
}

const compiler::syntax::syntax_node& compiler::syntax::syntax_tree::get_root() const
{
  if (!m_root_node_index.has_value())
  {
    throw std::logic_error("syntax tree has no root");
  }

  return get_node(*m_root_node_index);
}

std::optional<size_t> compiler::syntax::syntax_tree::get_root_index() const
{
  return m_root_node_index;
}

uint64_t compiler::syntax::syntax_tree::get_source_generation() const
{
  return m_source_generation;
}

const compiler::syntax::syntax_token& compiler::syntax::syntax_tree::get_token(size_t index) const
{
  if (index >= m_tokens.size())
  {
    throw std::out_of_range("index");
  }

  return m_tokens[index];
}

size_t compiler::syntax::syntax_tree::get_token_count() const
{
  return m_tokens.size();
}

const std::vector<compiler::syntax::syntax_token>& compiler::syntax::syntax_tree::get_tokens() const
{
  return m_tokens;
}

bool compiler::syntax::syntax_tree::has_root() const
{
  return m_root_node_index.has_value();
}
