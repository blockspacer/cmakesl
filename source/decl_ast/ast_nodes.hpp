#pragma once

#include "decl_ast/ast_node.hpp"
#include "decl_ast/ast_node_visitor.hpp"

#include <memory>

#define VISIT_MEHTOD                                                          \
  void visit(ast_node_visitor& visitor) const override                        \
  {                                                                           \
    visitor.visit(*this);                                                     \
  }

namespace cmsl::decl_ast {
class component_node : public ast_node
{
public:
  using nodes_t = std::vector<std::unique_ptr<ast_node>>;

  explicit component_node(const token_t& name, const token_t& open_brace,
                          nodes_t nodes, const token_t& close_brace)
    : m_name{ name }
    , m_open_brace{ open_brace }
    , m_nodes{ std::move(nodes) }
    , m_close_brace{ close_brace }
  {
  }

  const token_t& name() const { return m_name; }
  const token_t& open_brace() const { return m_open_brace; }

  const nodes_t& nodes() const { return m_nodes; }

  const token_t& close_brace() const { return m_close_brace; }

  source_location begin_location() const override
  {
    return m_name.src_range().begin;
  }

  source_location end_location() const override
  {
    return m_close_brace.src_range().end;
  }

  VISIT_MEHTOD

private:
  token_t m_name;
  token_t m_open_brace;
  nodes_t m_nodes;
  token_t m_close_brace;
};

class property_node : public ast_node
{
public:
  explicit property_node(const token_t& name, const token_t& assignment,
                         std::unique_ptr<ast_node> value)
    : m_name{ name }
    , m_assignment{ assignment }
    , m_value{ std::move(value) }
  {
  }

  const token_t& name() const { return m_name; }
  const token_t& assignment() const { return m_assignment; }
  const ast_node& value() const { return *m_value; }

  source_location begin_location() const override
  {
    return m_name.src_range().begin;
  }

  source_location end_location() const override
  {
    return m_value->end_location();
  }

  VISIT_MEHTOD

private:
  token_t m_name;
  token_t m_assignment;
  std::unique_ptr<ast_node> m_value;
};

class fundamental_value_node : public ast_node
{
public:
  explicit fundamental_value_node(const token_t& token)
    : m_token{ token }
  {
  }

  token_t token() const { return m_token; }

  source_location begin_location() const override
  {
    return m_token.src_range().begin;
  }

  source_location end_location() const override
  {
    return m_token.src_range().end;
  }

protected:
  const token_t m_token;
};

class bool_value_node : public fundamental_value_node
{
public:
  explicit bool_value_node(const token_t& token)
    : fundamental_value_node{ token }
  {
  }

  VISIT_MEHTOD
};

class int_value_node : public fundamental_value_node
{
public:
  explicit int_value_node(const token_t& token)
    : fundamental_value_node{ token }
  {
  }

  VISIT_MEHTOD
};

class double_value_node : public fundamental_value_node
{
public:
  explicit double_value_node(const token_t& token)
    : fundamental_value_node{ token }
  {
  }

  VISIT_MEHTOD
};

class string_value_node : public fundamental_value_node
{
public:
  explicit string_value_node(const token_t& token)
    : fundamental_value_node{ token }
  {
  }

  VISIT_MEHTOD
};

class list_node : public ast_node
{
public:
  using nodes_t = std::vector<std::unique_ptr<ast_node>>;

  explicit list_node(const token_t& open_square, nodes_t nodes,
                     const token_t& close_square)
    : m_open_square{ open_square }
    , m_nodes{ std::move(nodes) }
    , m_close_square{ close_square }
  {
  }

  const token_t& open_square() const { return m_open_square; }
  const nodes_t& nodes() const { return m_nodes; }
  const token_t& close_square() const { return m_close_square; }

  source_location begin_location() const override
  {
    return m_open_square.src_range().begin;
  }

  source_location end_location() const override
  {
    return m_close_square.src_range().end;
  }

  VISIT_MEHTOD

private:
  token_t m_open_square;
  nodes_t m_nodes;
  token_t m_close_square;
};

}