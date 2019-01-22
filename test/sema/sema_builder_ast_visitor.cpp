#include "sema/sema_builder_ast_visitor.hpp"

#include "ast/infix_nodes.hpp"
#include "ast/return_node.hpp"

#include "errors/errors_observer.hpp"

#include "test/ast/mock/ast_context_mock.hpp"
#include "test/ast/mock/function_mock.hpp"
#include "test/common/tokens.hpp"
#include "test/errors_observer_mock/errors_observer_mock.hpp"
#include "test/sema/mock/expression_node_mock.hpp"
#include "test/sema/mock/identifiers_context_mock.hpp"
#include "test/sema/mock/sema_context_mock.hpp"

#include <gmock/gmock.h>

namespace cmsl
{
    namespace sema
    {
        namespace test
        {
            using ::testing::StrictMock;
            using ::testing::Return;
            using ::testing::ReturnRef;
            using ::testing::IsNull;
            using ::testing::NotNull;
            using ::testing::Eq;
            using ::testing::_;

            using namespace cmsl::test::common;

            // Todo: to a common file
            struct errors_observer_and_mock
            {
                StrictMock<errors::test::errors_observer_mock> mock;
                errors::errors_observer observer;
            };
            using errs_t = errors_observer_and_mock;


            const sema_context valid_context;
            const sema_type valid_type(valid_context, token_identifier("foo"), {});

            MATCHER(IsValidType, "")
            {
                return arg == valid_type;
            }

            MATCHER(ParamDeclarations, "")
            {
                return arg == valid_type;
            }

            TEST(SemaBuilderAstVisitorTest, Visit_BoolValue_GetCorrectBoolValue)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                identifiers_context_mock ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                // Todo: use int alias
                const auto value = true;
                const auto token = token_kw_true();
                ast::bool_value_node node(token);

                EXPECT_CALL(ctx, find_type(cmsl::string_view{"bool"}))
                        .WillOnce(Return(&valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_node = dynamic_cast<const bool_value_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
                EXPECT_THAT(casted_node->value(), Eq(value));
                EXPECT_THAT(casted_node->type(), IsValidType());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IntValue_GetCorrectIntValue)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                identifiers_context_mock ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                // Todo: use int alias
                const auto value = std::int64_t{ 42 };
                const auto token = token_integer("42");
                ast::int_value_node node(token);

                EXPECT_CALL(ctx, find_type(cmsl::string_view{"int"}))
                        .WillOnce(Return(&valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_node = dynamic_cast<const int_value_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
                EXPECT_THAT(casted_node->value(), Eq(value));
                EXPECT_THAT(casted_node->type(), IsValidType());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_DoubleValue_GetCorrectDoubleValue)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                identifiers_context_mock ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                // Todo: use int alias
                const auto value = double{ 4.2 };
                const auto token = token_double("4.2");
                ast::double_value_node node(token);

                EXPECT_CALL(ctx, find_type(cmsl::string_view{"double"}))
                        .WillOnce(Return(&valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_node = dynamic_cast<const double_value_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
                EXPECT_NEAR(casted_node->value(), value, 0.00001);
                EXPECT_THAT(casted_node->type(), IsValidType());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_StringValue_GetCorrectStringValue)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                identifiers_context_mock ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                // Todo: use int alias
                const auto value = cmsl::string_view{ "\"42\"" };
                const auto token = token_string("\"42\"");
                ast::string_value_node node(token);

                EXPECT_CALL(ctx, find_type(cmsl::string_view{"string"}))
                        .WillOnce(Return(&valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_node = dynamic_cast<const string_value_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
                EXPECT_THAT(casted_node->value(), Eq(value));
                EXPECT_THAT(casted_node->type(), IsValidType());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_Identifier_GetCorrectIdentifierNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                const auto id_token = token_identifier("foo");
                const ast::id_node node{ id_token };

                EXPECT_CALL(ids_ctx, type_of(id_token.str()))
                    .WillOnce(Return(&valid_type));

                visitor.visit(node);


                ASSERT_THAT(visitor.m_result_node, NotNull());
                const auto casted_node = dynamic_cast<const id_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
                EXPECT_THAT(casted_node->id(), Eq(id_token));
                EXPECT_THAT(casted_node->type(), IsValidType()); // Todo: compare by reference
            }

            TEST(SemaBuilderAstVisitorTest, Visit_VariableDeclarationWithoutInitialization_GetVariableDeclarationNodeWithoutInitialization)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };
                const auto type_ref = ast::type_reference{ token_identifier(), token_identifier() };
                const auto name_token = token_identifier("foo");

                ast::variable_declaration_node variable_node(type_ref, name_token, nullptr);

                EXPECT_CALL(ids_ctx, register_identifier(name_token, &valid_type));

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
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };
                const auto type_ref = ast::type_reference{ token_identifier(), token_identifier() };
                const auto name_token = token_identifier("foo");

                // Todo: use int alias
                const auto initialization_value = std::int64_t{ 42 };
                const auto initialization_token = token_integer("42");
                auto initializaton_node = std::make_unique<ast::int_value_node>(initialization_token);

                ast::variable_declaration_node variable_node(type_ref, name_token, std::move(initializaton_node));

                EXPECT_CALL(ctx, find_type(_))
                        .WillRepeatedly(Return(&valid_type));

                EXPECT_CALL(ids_ctx, register_identifier(name_token, &valid_type));

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

            TEST(SemaBuilderAstVisitorTest, Visit_Return)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                auto expr_node = std::make_unique<ast::int_value_node>( token_integer("42") );
                ast::return_node ret_node(std::move(expr_node));

                EXPECT_CALL(ctx, find_type(_))
                .WillOnce(Return(&valid_type));

                visitor.visit(ret_node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<return_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                // Todo check for void type
                EXPECT_THAT(casted_node->type(), IsValidType());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_FunctionCallWithoutParameters_GetFunctionCallNodeWithoutParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                StrictMock<ast::test::function_mock> function_mock;
                const ast::function::params_declaration_t param_declarations;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                const auto fun_name_token = token_identifier("foo");
                const ast::function_call_node node{fun_name_token, {}};

                EXPECT_CALL(ctx, find_function(fun_name_token.str()))
                        .WillOnce(Return(&function_mock));

                EXPECT_CALL(function_mock, get_params_declarations())
                        .WillOnce(ReturnRef(param_declarations));

                EXPECT_CALL(function_mock, get_type())
                        .WillOnce(ReturnRef(valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<function_call_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                EXPECT_THAT(casted_node->param_expressions().size(), Eq(0u));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_FunctionCallWithParameters_GetFunctionCallNodeWithParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                StrictMock<ast::test::function_mock> function_mock;
                sema_builder_ast_visitor visitor{ ctx, errs.observer, ids_ctx };

                const auto fun_name_token = token_identifier("foo");
                const auto param1_id_token = token_identifier("bar");
                const auto param2_id_token = token_identifier("baz");

                sema_function::params_declaration_t param_declarations;
                param_declarations.emplace_back(ast::parameter_declaration{&valid_type, param1_id_token});
                param_declarations.emplace_back(ast::parameter_declaration{&valid_type, param2_id_token});

                EXPECT_CALL(ctx, find_function(fun_name_token.str()))
                        .WillOnce(Return(&function_mock));

                EXPECT_CALL(function_mock, get_params_declarations())
                        .WillOnce(ReturnRef(param_declarations));

                EXPECT_CALL(function_mock, get_type())
                        .WillOnce(ReturnRef(valid_type));

                EXPECT_CALL(ids_ctx, type_of(param1_id_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, type_of(param2_id_token.str()))
                        .WillOnce(Return(&valid_type));

                // Todo: use mocks
                auto param1_ast_node = std::make_unique<ast::id_node>(param1_id_token);
                auto param2_ast_node = std::make_unique<ast::id_node>(param2_id_token);

                ast::function_call_node::params_t ast_params;
                ast_params.emplace_back(std::move(param1_ast_node));
                ast_params.emplace_back(std::move(param2_ast_node));
                const ast::function_call_node node{fun_name_token, std::move(ast_params)};

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<function_call_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                const auto expected_number_of_params{ 2u };
                EXPECT_THAT(casted_node->param_expressions().size(), Eq(expected_number_of_params));

                // Todo: consider checking params one by one
            }

            TEST(SemaBuilderAstVisitorTest, Visit_MemberFunctionCallWithoutParameters_GetMemberFunctionCallNodeWithoutParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                StrictMock<ast::test::function_mock> function_mock;
                const ast::function::params_declaration_t param_declarations;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto lhs_id_token = token_identifier("foo");
                const ast::type lhs_type{ lhs_id_token.str(), ast::type_kind::k_user, &ctx };
                auto lhs_ast_node = std::make_unique<ast::id_node>(lhs_id_token);

                const auto fun_name_token = token_identifier("bar");
                const ast::member_function_call_node node{ std::move(lhs_ast_node), fun_name_token, {} };

                EXPECT_CALL(ids_ctx, type_of(lhs_id_token.str()))
                        .WillOnce(Return(&lhs_type));

                EXPECT_CALL(ctx, find_function(fun_name_token.str()))
                        .WillOnce(Return(&function_mock));

                EXPECT_CALL(function_mock, get_params_declarations())
                        .WillOnce(ReturnRef(param_declarations));

                EXPECT_CALL(function_mock, get_type())
                        .WillOnce(ReturnRef(valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<member_function_call_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                const auto expected_number_of_params{ 0u };
                EXPECT_THAT(casted_node->param_expressions().size(), Eq(expected_number_of_params));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_MemberFunctionCallWithParameters_GetMemberFunctionCallNodeWithParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                StrictMock<ast::test::function_mock> function_mock;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto lhs_id_token = token_identifier("foo");
                const ast::type lhs_type{ lhs_id_token.str(), ast::type_kind::k_user, &ctx };
                auto lhs_ast_node = std::make_unique<ast::id_node>(lhs_id_token);

                const auto param1_id_token = token_identifier("baz");
                const auto param2_id_token = token_identifier("qux");

                ast::function::params_declaration_t param_declarations;
                param_declarations.emplace_back(ast::parameter_declaration{&valid_type, param1_id_token});
                param_declarations.emplace_back(ast::parameter_declaration{&valid_type, param2_id_token});
                // Todo: use mocks
                auto param1_ast_node = std::make_unique<ast::id_node>(param1_id_token);
                auto param2_ast_node = std::make_unique<ast::id_node>(param2_id_token);
                ast::function_call_node::params_t ast_params;
                ast_params.emplace_back(std::move(param1_ast_node));
                ast_params.emplace_back(std::move(param2_ast_node));

                const auto fun_name_token = token_identifier("bar");
                const ast::member_function_call_node node{ std::move(lhs_ast_node), fun_name_token, std::move(ast_params) };

                EXPECT_CALL(ids_ctx, type_of(lhs_id_token.str()))
                        .WillOnce(Return(&lhs_type));

                EXPECT_CALL(ctx, find_function(fun_name_token.str()))
                        .WillOnce(Return(&function_mock));

                EXPECT_CALL(function_mock, get_params_declarations())
                        .WillOnce(ReturnRef(param_declarations));

                EXPECT_CALL(ids_ctx, type_of(param1_id_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, type_of(param2_id_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(function_mock, get_type())
                        .WillOnce(ReturnRef(valid_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<member_function_call_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                const auto expected_number_of_params{ 2u };
                EXPECT_THAT(casted_node->param_expressions().size(), Eq(expected_number_of_params));

                // Todo: consider checking params one by one
            }

            // Todo: consider test with nodes inside block
            TEST(SemaBuilderAstVisitorTest, Visit_Block_GetBlockNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                ast::block_node block{ {} };

                EXPECT_CALL(ids_ctx, enter_ctx());
                EXPECT_CALL(ids_ctx, leave_ctx());

                visitor.visit(block);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<block_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_nodes{ 0u };
                EXPECT_THAT(casted_node->nodes().size(), Eq(expected_number_of_nodes));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_FunctionWithoutParameters_GetFunctionNodeWithoutParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                auto return_type_token = token_identifier("foo");
                auto return_type_reference = ast::type_reference{ return_type_token, return_type_token};
                auto name_token = token_identifier("bar");
                auto block = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                ast::user_function_node2 node{ return_type_reference, name_token, {}, std::move(block) };

                EXPECT_CALL(ctx, find_type(return_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));

                // Two scopes: parameters and block
                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(2);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(2);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<function_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_params{ 0u };
                EXPECT_THAT(casted_node->params().size(), Eq(expected_number_of_params));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_FunctionWithParameters_GetFunctionNodeWithParameters)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                auto return_type_token = token_identifier("foo");
                auto return_type_reference = ast::type_reference{ return_type_token, return_type_token};
                auto name_token = token_identifier("bar");
                auto param_type_token = token_identifier("baz");
                auto param_type_reference = ast::type_reference{ param_type_token, param_type_token};
                auto param_name_token = token_identifier("baz");

                ast::user_function_node2::params_t params;
                params.emplace_back(ast::param_declaration{param_type_reference, param_name_token});

                auto block = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                ast::user_function_node2 node{ return_type_reference, name_token, std::move(params), std::move(block) };


                EXPECT_CALL(ctx, find_type(return_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ctx, find_type(param_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));

                // Two scopes: parameters and block
                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(2);
                EXPECT_CALL(ids_ctx, register_identifier(param_name_token, &valid_type));
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(2);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<function_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_params{ 1u };
                EXPECT_THAT(casted_node->params().size(), Eq(expected_number_of_params));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_ClassEmpty_GetEmptyClassNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                auto class_name_token = token_identifier("foo");

                ast::class_node2 node{class_name_token, {}};

                EXPECT_CALL(ctx, find_type_in_this_scope(class_name_token.str()))
                        .WillOnce(Return(nullptr));

                EXPECT_CALL(ctx, add_type(_));

                EXPECT_CALL(ids_ctx, enter_ctx());
                EXPECT_CALL(ids_ctx, leave_ctx());

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<class_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->name().str(), Eq(class_name_token.str()));
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_members{ 0u };
                EXPECT_THAT(casted_node->members().size(), Eq(expected_number_of_members));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_ClassWithMembers_GetClassNodeWithMembers)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                auto class_name_token = token_identifier("foo");
                const auto member_name_token = token_identifier("bar");
                const auto member_type_token = token_kw_int();
                const auto member_type_reference = ast::type_reference{ member_type_token, member_type_token };

                auto member_declaration = std::make_unique<ast::variable_declaration_node>(member_type_reference, member_name_token, nullptr);
                ast::class_node2::nodes_t nodes;
                nodes.emplace_back(std::move(member_declaration));

                ast::class_node2 node{class_name_token, std::move(nodes)};

                EXPECT_CALL(ctx, find_type_in_this_scope(class_name_token.str()))
                        .WillOnce(Return(nullptr));

                EXPECT_CALL(ctx, find_type(member_type_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ctx, add_type(_));

                EXPECT_CALL(ids_ctx, enter_ctx());
                EXPECT_CALL(ids_ctx, register_identifier(member_name_token, &valid_type));
                EXPECT_CALL(ids_ctx, leave_ctx());

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<class_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->name().str(), Eq(class_name_token.str()));
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_members{ 1u };
                EXPECT_THAT(casted_node->members().size(), Eq(expected_number_of_members));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_ClassWithFunctions_GetClassNodeWithFunctions)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto class_name_token = token_identifier("foo");

                const auto function_return_type_token = token_kw_int();
                const auto function_return_type_reference = ast::type_reference{ function_return_type_token, function_return_type_token};
                const auto function_name_token = token_identifier("bar");
                auto function_body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto function = std::make_unique<ast::user_function_node2>(function_return_type_reference,
                                                                           function_name_token,
                                                                           ast::user_function_node2::params_t{},
                                                                           std::move(function_body));
                ast::class_node2::nodes_t nodes;
                nodes.emplace_back(std::move(function));
                ast::class_node2 node{ class_name_token, std::move(nodes)};

                // Class type lookup
                EXPECT_CALL(ctx, find_type_in_this_scope(class_name_token.str()))
                        .WillOnce(Return(nullptr));

                // Function return type lookup
                EXPECT_CALL(ctx, find_type(function_return_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ctx, add_type(_));

                // There are three identifier contextes: class, function parameters and function body.
                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(3);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(3);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<class_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->name().str(), Eq(class_name_token.str()));
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_members{ 0u };
                EXPECT_THAT(casted_node->members().size(), Eq(expected_number_of_members));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_ClassWithFunctionsAndMembers_GetClassNodeWithFunctionsAndMembers)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto class_name_token = token_identifier("foo");

                const auto member_name_token = token_identifier("baz");
                const auto member_type_token = token_kw_int();
                const auto member_type_reference = ast::type_reference{ member_type_token, member_type_token };
                auto member_declaration = std::make_unique<ast::variable_declaration_node>(member_type_reference, member_name_token, nullptr);

                const auto function_return_type_token = token_kw_int();
                const auto function_return_type_reference = ast::type_reference{ function_return_type_token, function_return_type_token};
                const auto function_name_token = token_identifier("bar");
                auto function_body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto function = std::make_unique<ast::user_function_node2>(function_return_type_reference,
                                                                           function_name_token,
                                                                           ast::user_function_node2::params_t{},
                                                                           std::move(function_body));

                ast::class_node2::nodes_t nodes;
                nodes.emplace_back(std::move(member_declaration));
                nodes.emplace_back(std::move(function));
                ast::class_node2 node{ class_name_token, std::move(nodes)};

                // Class type lookup.
                EXPECT_CALL(ctx, find_type_in_this_scope(class_name_token.str()))
                        .WillOnce(Return(nullptr));

                // Function return type and member type lookup.
                EXPECT_CALL(ctx, find_type(member_type_reference.to_string()))
                        .Times(2)
                        .WillOnce(Return(&valid_type))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ctx, add_type(_));

                // There are three identifier contextes: class, function parameters and function body. Member is registered in class context.
                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(3);
                EXPECT_CALL(ids_ctx, register_identifier(member_name_token, &valid_type));
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(3);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<class_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->name().str(), Eq(class_name_token.str()));
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_members{ 1u };
                EXPECT_THAT(casted_node->members().size(), Eq(expected_number_of_members));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_WhileNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto condition_identifier_token = token_identifier("foo");
                auto condition_ast_node = std::make_unique<ast::id_node>(condition_identifier_token);
                auto body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node = std::make_unique<ast::conditional_node>(std::move(condition_ast_node), std::move(body));

                ast::while_node node(std::move(conditional_ast_node));

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(2);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(2);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<while_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IfNode_GetIfNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto condition_identifier_token = token_identifier("foo");
                auto condition_ast_node = std::make_unique<ast::id_node>(condition_identifier_token);
                auto body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node = std::make_unique<ast::conditional_node>(std::move(condition_ast_node), std::move(body));

                ast::if_else_node::ifs_t ifs;
                ifs.emplace_back(std::move(conditional_ast_node));

                ast::if_else_node node(std::move(ifs), nullptr);

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(2);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(2);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<if_else_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_ifs{ 1u };
                EXPECT_THAT(casted_node->ifs().size(), Eq(expected_number_of_ifs));

                EXPECT_THAT(casted_node->else_body(), IsNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IfElseNode_GetIfElseNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto condition_identifier_token = token_identifier("foo");
                auto condition_ast_node = std::make_unique<ast::id_node>(condition_identifier_token);
                auto body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node = std::make_unique<ast::conditional_node>(std::move(condition_ast_node), std::move(body));

                ast::if_else_node::ifs_t ifs;
                ifs.emplace_back(std::move(conditional_ast_node));

                auto else_body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});

                ast::if_else_node node(std::move(ifs), std::move(else_body));

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(4);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(4);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<if_else_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_ifs{ 1u };
                EXPECT_THAT(casted_node->ifs().size(), Eq(expected_number_of_ifs));

                ASSERT_THAT(casted_node->else_body(), NotNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IfElseIfNode_GetIfElseIfNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto condition_identifier_token = token_identifier("foo");
                auto condition_ast_node = std::make_unique<ast::id_node>(condition_identifier_token);
                auto body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node = std::make_unique<ast::conditional_node>(std::move(condition_ast_node), std::move(body));

                const auto condition_identifier_token_2 = token_identifier("bar");
                auto condition_ast_node_2 = std::make_unique<ast::id_node>(condition_identifier_token_2);
                auto body_2 = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node_2 = std::make_unique<ast::conditional_node>(std::move(condition_ast_node_2), std::move(body_2));

                ast::if_else_node::ifs_t ifs;
                ifs.emplace_back(std::move(conditional_ast_node));
                ifs.emplace_back(std::move(conditional_ast_node_2));

                ast::if_else_node node(std::move(ifs), nullptr);

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token_2.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(4);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(4);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<if_else_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_ifs{ 2u };
                EXPECT_THAT(casted_node->ifs().size(), Eq(expected_number_of_ifs));

                EXPECT_THAT(casted_node->else_body(), IsNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_IfElseIfElseNode_GetIfElseIfElseNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto condition_identifier_token = token_identifier("foo");
                auto condition_ast_node = std::make_unique<ast::id_node>(condition_identifier_token);
                auto body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node = std::make_unique<ast::conditional_node>(std::move(condition_ast_node), std::move(body));

                const auto condition_identifier_token_2 = token_identifier("bar");
                auto condition_ast_node_2 = std::make_unique<ast::id_node>(condition_identifier_token_2);
                auto body_2 = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto conditional_ast_node_2 = std::make_unique<ast::conditional_node>(std::move(condition_ast_node_2), std::move(body_2));

                ast::if_else_node::ifs_t ifs;
                ifs.emplace_back(std::move(conditional_ast_node));
                ifs.emplace_back(std::move(conditional_ast_node_2));

                auto else_body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});

                ast::if_else_node node(std::move(ifs), std::move(else_body));

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, type_of(condition_identifier_token_2.str()))
                        .WillOnce(Return(&valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(6);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(6);

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<if_else_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto expected_number_of_ifs{ 2u };
                EXPECT_THAT(casted_node->ifs().size(), Eq(expected_number_of_ifs));

                EXPECT_THAT(casted_node->else_body(), NotNull());
            }

            TEST(SemaBuilderAstVisitorTest, Visit_ClassMemberAccess_GetClassMemberAccessNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto lhs_id_token = token_identifier("foo");
                auto lhs_node = std::make_unique<ast::id_node>(lhs_id_token);

                const auto member_name_token = token_identifier("bar");
                // Todo: use initializer list
                std::vector<ast::type::member_info> members;
                members.emplace_back(ast::type::member_info{member_name_token, valid_type});
                ast::type lhs_type{ "baz", ast::type_kind::k_user, &ctx, std::move(members)};

                ast::class_member_access_node node{ std::move(lhs_node), member_name_token };

                EXPECT_CALL(ids_ctx, type_of(lhs_id_token.str()))
                        .WillOnce(Return(&lhs_type));

                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<class_member_access_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                EXPECT_THAT(casted_node->type(), IsValidType());
                EXPECT_THAT(casted_node->member_name(), Eq(member_name_token));
            }

            TEST(SemaBuilderAstVisitorTest, Visit_TranslationUnit_GetTranslationUnitNode)
            {
                errs_t errs;
                StrictMock<sema_context_mock> ctx;
                StrictMock<identifiers_context_mock> ids_ctx;
                sema_builder_ast_visitor visitor{ctx, errs.observer, ids_ctx};

                const auto variable_name_token = token_identifier("foo");
                const auto variable_type_token = token_kw_int();
                const auto variable_type_reference = ast::type_reference{ variable_type_token, variable_type_token};
                auto variable_declaration_ast_node = std::make_unique<ast::variable_declaration_node>(variable_type_reference, variable_name_token, nullptr);

                const auto function_return_type_token = token_kw_double();
                const auto function_return_type_reference = ast::type_reference{ function_return_type_token, function_return_type_token };
                const auto function_name_token = token_identifier("bar");
                auto function_body = std::make_unique<ast::block_node>(ast::block_node::expressions_t{});
                auto function_ast_node = std::make_unique<ast::user_function_node2>(function_return_type_reference, function_name_token, ast::user_function_node2::params_t{}, std::move(function_body));

                const auto class_name_token = token_identifier("baz");
                auto class_ast_node = std::make_unique<ast::class_node2>(class_name_token, ast::class_node2::nodes_t{});

                ast::translation_unit_node::nodes_t nodes;
                nodes.emplace_back(std::move(variable_declaration_ast_node));
                nodes.emplace_back(std::move(function_ast_node));
                nodes.emplace_back(std::move(class_ast_node));

                ast::translation_unit_node node{ std::move(nodes) };

                EXPECT_CALL(ctx, find_type(variable_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));
                EXPECT_CALL(ctx, find_type(function_return_type_reference.to_string()))
                        .WillOnce(Return(&valid_type));
                EXPECT_CALL(ctx, find_type_in_this_scope(class_name_token.str()))
                        .WillOnce(Return(nullptr));

                EXPECT_CALL(ctx, add_type(_));

                EXPECT_CALL(ids_ctx, register_identifier(variable_name_token, &valid_type));

                EXPECT_CALL(ids_ctx, enter_ctx())
                        .Times(3);
                EXPECT_CALL(ids_ctx, leave_ctx())
                        .Times(3);


                visitor.visit(node);

                ASSERT_THAT(visitor.m_result_node, NotNull());

                const auto casted_node = dynamic_cast<translation_unit_node*>(visitor.m_result_node.get());
                ASSERT_THAT(casted_node, NotNull());

                const auto& translation_unit_nodes = casted_node->nodes();
                const auto expected_number_of_nodes{ 3u };
                ASSERT_THAT(translation_unit_nodes.size(), Eq(expected_number_of_nodes));

                const auto casted_variable_node = dynamic_cast<variable_declaration_node*>(translation_unit_nodes[0].get());
                EXPECT_THAT(casted_variable_node, NotNull());

                const auto casted_function_node = dynamic_cast<function_node*>(translation_unit_nodes[1].get());
                EXPECT_THAT(casted_function_node, NotNull());

                const auto casted_class_node = dynamic_cast<class_node*>(translation_unit_nodes[2].get());
                EXPECT_THAT(casted_class_node, NotNull());

            }
        }
    }
}
