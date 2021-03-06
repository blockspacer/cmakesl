#pragma once

#include "ast/ast_node.hpp"

#include <memory>

namespace cmsl::ast {
class block_node;

class conditional_node : public ast_node
{
public:
  explicit conditional_node(token_t open_paren,
                            std::unique_ptr<ast_node> condition,
                            token_t close_paren,
                            std::unique_ptr<block_node> body);
  ~conditional_node();

  token_t open_paren() const;
  token_t close_paren() const;

  const ast_node& condition() const;
  const block_node& body() const;

  void visit(ast_node_visitor& visitor) const override;
  source_location begin_location() const override;
  source_location end_location() const override;

private:
  token_t m_open_paren;
  std::unique_ptr<ast_node> m_condition;
  token_t m_close_paren;
  std::unique_ptr<block_node> m_body;
};
}
