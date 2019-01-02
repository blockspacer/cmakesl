#include "sema/sema_builder_ast_visitor.hpp"

#include "ast/infix_nodes.hpp"

#include "errors/errors_observer.hpp"

#include "test/ast/mock/ast_context_mock.hpp"
#include "test/common/tokens.hpp"
#include "test/errors_observer_mock/errors_observer_mock.hpp"

#include <gmock/gmock.h>

namespace cmsl
{
    namespace sema
    {
        namespace test
        {
            using ::testing::StrictMock;
            using ::testing::Return;
            using ::testing::IsNull;
            using ::testing::NotNull;
            using ::testing::Eq;
            using ::testing::_;

            using namespace cmsl::test::common;

            // Todo: to a common fole
            struct errors_observer_and_mock
            {
                StrictMock<errors::test::errors_observer_mock> mock;
                errors::errors_observer observer;
            };
            using errs_t = errors_observer_and_mock;

            const ast::type valid_type{ token_kw_int() };

            MATCHER(IsValidType, "")
            {
                return arg == valid_type;
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IntValue_GetCorrectIntValue)
            {
                errs_t errs;
                StrictMock<ast::test::ast_context_mock> ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer };

                // Todo: use int alias
                const auto initialization_value = std::int64_t{ 42 };
                const auto initialization_token = token_integer("42");
                ast::int_value_node int_node(initialization_token);

                visitor.visit(int_node);

                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_initialization_node = dynamic_cast<const int_value_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_initialization_node, NotNull());
                EXPECT_THAT(casted_initialization_node->value(), Eq(initialization_value));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_VariableDeclarationWithoutInitialization_GetVariableDeclarationNodeWithoutInitialization)
            {
                errs_t errs;
                StrictMock<ast::test::ast_context_mock> ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer };
                const auto type_ref = ast::type_reference{ token_identifier(), token_identifier() };
                const auto name_token = token_identifier("foo");

                ast::variable_declaration_node variable_node(type_ref, name_token, nullptr);

                EXPECT_CALL(ctx, find_type(_))
                        .WillOnce(Return(&valid_type));

                visitor.visit(variable_node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<variable_declaration_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                EXPECT_THAT(casted_node->name(), Eq(name_token));
                EXPECT_THAT(casted_node->initialization(), IsNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_VariableDeclarationWithInitialization_GetVariableDeclarationNodeWithInitialization)
            {
                errs_t errs;
                StrictMock<ast::test::ast_context_mock> ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer };
                const auto type_ref = ast::type_reference{ token_identifier(), token_identifier() };
                const auto name_token = token_identifier("foo");

                // Todo: use int alias
                const auto initialization_value = std::int64_t{ 42 };
                const auto initialization_token = token_integer("42");
                auto initializaton_node = std::make_unique<ast::int_value_node>(initialization_token);

                ast::variable_declaration_node variable_node(type_ref, name_token, std::move(initializaton_node));

                EXPECT_CALL(ctx, find_type(_))
                        .WillOnce(Return(&valid_type));

                visitor.visit(variable_node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<variable_declaration_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                EXPECT_THAT(casted_node->name(), Eq(name_token));
                EXPECT_THAT(casted_node->initialization(), NotNull());

                const auto casted_initialization_node = dynamic_cast<const int_value_node*>(casted_node->initialization());
                ASSERT_THAT(casted_initialization_node, NotNull());

                EXPECT_THAT(casted_initialization_node->value(), Eq(initialization_value));
            }
        }
    }
}