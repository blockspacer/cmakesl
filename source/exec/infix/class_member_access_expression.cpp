#include "exec/infix/class_member_access_expression.hpp"

#include "exec/instance/instance.hpp"

namespace cmsl
{
    namespace exec
    {
        namespace infix
        {
            class_member_access_expression::class_member_access_expression(std::unique_ptr<infix_expression> lhs, lexer::token::token member_name)
                : m_lhs{ std::move(lhs) }
                , m_member_name{ member_name }
            {}

            inst::instance *class_member_access_expression::evaluate(infix_evaluation_context &ctx) const
            {
                auto lhs_instance = m_lhs->evaluate(ctx);
                return lhs_instance->get_member(m_member_name.str());
            }
        }
    }
}
