#pragma once

#include "ast/ast_node.hpp"
#include "ast/type_representation.hpp"
#include "ast/ast_node_visitor.hpp" // todo: to cpp

namespace cmsl::ast
{
        class variable_declaration_node : public ast_node
        {
        public:
            explicit variable_declaration_node(type_representation ty, lexer::token::token name, std::unique_ptr<ast_node> initialization)
                : m_ty{ std::move(ty) }
                , m_name{ name }
                , m_initialization{ std::move(initialization) }
            {}

            const type_representation& get_type_representation() const
            {
                return m_ty;
            }

            lexer::token::token get_name() const
            {
                return m_name;
            }

            const ast_node* get_initialization() const
            {
                return m_initialization.get();
            }

            void visit(ast_node_visitor &visitor) const override
            {
                visitor.visit(*this);
            }

        private:
            type_representation m_ty;
            lexer::token::token m_name;
            std::unique_ptr<ast_node> m_initialization;
        };
}
