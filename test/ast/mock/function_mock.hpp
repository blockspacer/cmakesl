#pragma once

#include "ast/function.hpp"

#include <gmock/gmock.h>

namespace cmsl
{
    namespace test
    {
        namespace ast
        {
            class function_mock : public cmsl::ast::function
            {
            public:
                MOCK_CONST_METHOD0(get_name, cmsl::string_view());
                MOCK_CONST_METHOD0(get_params_declarations, const params_declaration_t&());
            };
        }
    }
}
