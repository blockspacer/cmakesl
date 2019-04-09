#include "ast/while_node.hpp"
#include "ast/conditional_node.hpp"
#include "ast/ast_node_visitor.hpp"
#include "while_node.hpp"


namespace cmsl::ast
{

        while_node::while_node(token_t while_kw, std::unique_ptr<conditional_node> node)
            : m_while_kw{ while_kw }
            , m_node{ std::move(node) }
        {}

        while_node::~while_node()
        {}

        const conditional_node &while_node::get_node() const
        {
            return *m_node;
        }

        void while_node::visit(ast_node_visitor &visitor) const
        {
            visitor.visit(*this);
        }

    ast_node::token_t while_node::while_kw() const
    {
        return m_while_kw;
    }
}