#pragma once

#include "ast/ast_node_type.hpp"

namespace cmsl
{
    namespace ast
    {
        class ast_node
        {
        public:
            ast_node(ast_node_type type)
                : m_type{ type }
            {}

            virtual ~ast_node()
            {}

            ast_node_type get_type() const
            {
                return m_type;
            }

        private:
            ast_node_type m_type;
        };
    }
}
