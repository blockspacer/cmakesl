#include "exec/stmt/return_statement.hpp"

#include "ast/return_node.hpp"
#include "exec/exec.hpp"

namespace cmsl
{
    namespace exec
    {
        namespace stmt
        {
            return_statement::return_statement(ast::return_node& node)
                : m_node{ node }
            {}

            void return_statement::execute(executor& e)
            {
                auto& expr = m_node.get_expression();
                const auto result = expr.evaluate();
                e.set_function_return_value(result);
            }
        }
    }
}
