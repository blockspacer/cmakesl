#pragma once

#include "ast/ast_node_visitor.hpp"
#include "ast/ast_context.hpp"
#include "ast/type_operator_support_check.hpp"
#include "ast/common_type_finder.hpp"
#include "ast/class_type.hpp"
#include "ast/while_node.hpp"
#include "ast/conditional_node.hpp"
#include "ast/if_else_node.hpp"
#include "ast/translation_unit_node.hpp"


#include "ast/variable_declaration_node.hpp"
#include "ast/infix_nodes.hpp"
#include "ast/return_node.hpp"
#include "ast/class_node.hpp"

#include "common/algorithm.hpp"

#include "errors/error.hpp"
#include "errors/errors_observer.hpp"

#include "sema/sema_nodes.hpp"
#include "sema/identifiers_context.hpp"
#include "sema/sema_function_builder.hpp"
#include "sema/sema_function_impl.hpp"
#include "sema/sema_function_factory.hpp"
#include "sema/sema_context.hpp"
#include "sema/sema_type.hpp"

namespace cmsl
{
    namespace sema
    {
        class sema_builder_ast_visitor : public ast::ast_node_visitor
        {
        private:
            using param_expressions_t = std::vector<std::unique_ptr<expression_node>>;

        public:
            explicit sema_builder_ast_visitor(sema_context_interface& ctx, errors::errors_observer& errs, identifiers_context& ids_context, sema_function_factory& function_factory)
                : m_ctx{ ctx }
                , m_errors_observer{ errs }
                , m_ids_context{ ids_context }
                , m_function_factory{ function_factory }
            {}

            void visit(const ast::block_node& node) override
            {
                auto ig = ids_guard();

                std::vector<std::unique_ptr<sema_node>> nodes;

                for(auto n : node.get_expressions())
                {
                    auto child = visit_child(*n);
                    if(!child)
                    {
                        return;
                    }

                    nodes.emplace_back(std::move(child));
                }

                m_result_node = std::make_unique<block_node>(std::move(nodes));
            }

            void visit(const ast::class_node2& node) override
            {
                const auto name = node.get_name();
                if(const auto found_type = m_ctx.find_type_in_this_scope(name.str()))
                {
                    // Todo: type redefinition
                    raise_error();
                    return;
                }

                auto class_ids_guard = ids_guard();

                auto created_class_sema_ctx = std::make_unique<sema_context>(&m_ctx);
                auto class_sema_ctx = created_class_sema_ctx.get();

                auto members = collect_class_members_and_add_functions_to_ctx(node);

                if(!members)
                {
                    return;
                }

                // We move created ast ctx ownership but it will live for the whole program lifetime, so it is safe to use class_ast_ctx raw pointer.
                // Todo: ast shouldn't store ast ctx somewhere else?
                auto created_class_type = std::make_unique<sema_type>(*class_sema_ctx, name, std::move(members->info));
                auto class_type = created_class_type.get();

                // Same as with ast context. class_type raw pointer will be valid. We move it to context to make this class findable as a regular type (so e.g. inside this class methods, the type will appear as a regular type).
                m_ctx.add_type(std::move(created_class_type));

                std::vector<std::unique_ptr<function_node>> functions;

                for(auto function_declaration : members->functions)
                {
                    auto body = visit_child_node<block_node>(function_declaration.body_to_visit);
                    if(!body)
                    {
                        return;
                    }

                    function_declaration.fun->set_body(*body);
                    functions.emplace_back(std::make_unique<function_node>(*function_declaration.fun));
                }

                m_result_node = std::make_unique<class_node>(name, std::move(members->declarations));
            }

            void visit(const ast::conditional_node& node) override
            {
                auto ig = ids_guard();

                auto condition = visit_child_expr(node.get_condition());
                if(!condition)
                {
                    return;
                }

                auto body = visit_child_node<block_node>(node.get_block());
                if(!body)
                {
                    return;
                }

                m_result_node = std::make_unique<conditional_node>(std::move(condition), std::move(body));
            }

            void visit(const ast::if_else_node& node) override
            {
                std::vector<std::unique_ptr<conditional_node>> ifs;

                for(const auto& cond_node : node.get_ifs())
                {
                    auto cond = visit_child_node<conditional_node>(*cond_node);
                    if(!cond)
                    {
                        return;
                    }

                    ifs.emplace_back(std::move(cond));
                }

                std::unique_ptr<block_node> else_body;

                if(auto else_node = node.get_else())
                {
                    auto ig = ids_guard();

                    else_body = visit_child_node<block_node>(*else_node);
                    if(!else_body)
                    {
                        return;
                    }
                }

                m_result_node = std::make_unique<if_else_node>(std::move(ifs), std::move(else_body));
            }

            void visit(const ast::binary_operator_node& node) override
            {
                const auto op = node.get_operator().get_type();

                ast::type_operator_support_check check;

                auto lhs = visit_child_expr(node.get_lhs());
                if(!lhs)
                {
                    return;
                }

                // Todo: add check for whether operator is supported
                //if(!check.type_supports_operator(lhs->type().get_kind(), op))
                //{
                //    // Todo: lhs type doesn't support this operator
                //    raise_error();
                //    return;
                //}

                auto rhs = visit_child_expr(node.get_rhs());
                if(!rhs)
                {
                    return;
                }

                // Todo: add check for whether operator is supported
                //if(!check.type_supports_operator(rhs->type().get_kind(), op))
                //{
                //    // Todo: lhs type doesn't support this operator
                ///    raise_error();
                //    return;
                //}

                // Todo: introduce cast nodes
                //const auto* common_type = &lhs->type();
                //if(lhs->type() != rhs->type())
                //{
                //    common_type = ast::common_type_finder{}.find_common_type(lhs->type(), rhs->type());
                //    if(common_type == nullptr)
                //    {
                //       // Todo: can't apply operator to these types
                //      raise_error();
                //      return;
                //  }
                //}

                m_result_node = std::make_unique<binary_operator_node>(std::move(lhs), node.get_operator(), std::move(rhs), lhs->type());
            }

            void visit(const ast::class_member_access_node& node) override
            {
                auto lhs = visit_child_expr(node.get_lhs());
                if(!lhs)
                {
                    return;
                }

                const auto& lhs_type = lhs->type();
                auto member_info = lhs_type.find_member(node.get_member_name().str());
                if(!member_info)
                {
                    // Todo: type does not have such member.
                    raise_error();
                    return;
                }

                m_result_node = std::make_unique<class_member_access_node>(std::move(lhs), *member_info);
            }

            void visit(const ast::function_call_node& node) override
            {
                const auto name = node.get_name();
                const auto found_fun = m_ctx.find_function(name.str());
                if(!found_fun)
                {
                    // Todo: can't find function with such name.
                    raise_error();
                    return;
                }

                auto params = get_function_call_params(found_fun->signature(), node.get_param_nodes());
                if(!params)
                {
                    return;
                }

                m_result_node = std::make_unique<function_call_node>(*found_fun, std::move(*params));
            }

            void visit(const ast::member_function_call_node& node) override
            {
                const auto name = node.get_name();
                auto lhs = visit_child_expr(node.get_lhs());
                if(!lhs)
                {
                    return;
                }

                auto function = lhs->type().find_member_function(name.str());
                if(!function)
                {
                    // Todo: type has no such function
                    raise_error();
                    return;
                }

                auto params = get_function_call_params(function->signature(), node.get_param_nodes());
                if(!params)
                {
                    return;
                }

                m_result_node = std::make_unique<member_function_call_node>(std::move(lhs), *function, std::move(*params));
            }

            void visit(const ast::bool_value_node& node) override
            {
                const auto& type = *m_ctx.find_type("bool");
                const auto value = node.get_token().str() == "true";
                m_result_node = std::make_unique<bool_value_node>(type, value);
            }

            void visit(const ast::int_value_node& node) override
            {
                const auto& type = *m_ctx.find_type("int");
                const auto token = node.get_token();
                char* end;
                const auto value = std::strtol(token.str().data(), &end, 10); // todo: handle hex etc
                m_result_node = std::make_unique<int_value_node>(type, value);
            }

            void visit(const ast::double_value_node& node) override
            {
                const auto& type = *m_ctx.find_type("double");
                const auto token = node.get_token();
                char* end;
                const auto value = std::strtod(token.str().data(), &end); // todo: handle hex etc
                m_result_node = std::make_unique<double_value_node>(type, value);
            }

            void visit(const ast::string_value_node& node) override
            {
                const auto& type = *m_ctx.find_type("string");
                m_result_node = std::make_unique<string_value_node>(type, node.get_token().str());
            }

            void visit(const ast::id_node& node) override
            {
                const auto id_token = node.get_identifier();
                const auto type = m_ids_context.type_of(node.get_identifier().str());

                if(!type)
                {
                    // Todo: identifier not found
                    raise_error();
                    return;
                }

                m_result_node = std::make_unique<id_node>(*type, id_token);
            }

            void visit(const ast::return_node& node) override
            {
                auto v = clone();
                node.get_expression().visit(v);
                if(!v.m_result_node)
                {
                    return;
                }

                auto expr = to_expression(std::move(v.m_result_node));
                m_result_node = std::make_unique<return_node>(std::move(expr));
            }

            void visit(const ast::translation_unit_node& node) override
            {
                std::vector<std::unique_ptr<sema_node>> nodes;

                for(auto n : node.get_nodes())
                {
                    auto visited_node = visit_child(*n);
                    if(!visited_node)
                    {
                        return;
                    }

                    nodes.emplace_back(std::move(visited_node));
                }

                m_result_node = std::make_unique<translation_unit_node>(std::move(nodes));
            }

            void visit(const ast::user_function_node2& node) override
            {
                auto return_type = m_ctx.find_type(node.get_return_type_reference().to_string());
                if(!return_type)
                {
                    // Todo: unknown return type
                    raise_error();
                    return;
                }

                // Todo: parameters need to be in the same ids context as function body.
                auto params_ids_guard = ids_guard();

                using param_decl_t = sema_function::parameter_declaration;
                std::vector<param_decl_t> params;

                for(const auto& param_decl : node.get_param_declarations())
                {
                    auto param_type = m_ctx.find_type(param_decl.ty.to_string());
                    if(!param_type)
                    {
                        //Todo: unknown parameter type
                        raise_error();
                        return;
                    }

                    params.emplace_back(param_decl_t{*param_type, param_decl.name});
                    m_ids_context.register_identifier(param_decl.name, param_type);
                }

                sema_function::function_signature signature{
                        node.get_name(),
                        std::move(params)
                };
                auto& function = m_function_factory.create( m_ctx, *return_type, std::move(signature) );

                // Add function (without a body yet) to context, so it will be visible inside function body in case of a recursive call.
                m_ctx.add_function(function);

                auto block = visit_child_node<block_node>(node.get_body());
                if(!block)
                {
                    return;
                }

                // And set the body.
                function.set_body(*block);

                m_result_node = std::make_unique<function_node>(function);
            }

            void visit(const ast::variable_declaration_node& node) override
            {
                const auto type = m_ctx.find_type(node.get_type_reference().to_string());

                // Todo: handle generic types
                if(!type)
                {
                    // Todo: type not found
                    raise_error();
                    return;
                }

                std::unique_ptr<expression_node> initialization;
                if(auto init_node = node.get_initialization())
                {
                    initialization = visit_child_expr(*init_node);
                    if(!initialization)
                    {
                        return;
                    }

                    // Todo: detect implicit conversion and raise a warning
                    //if(!ast::type_conversion_check{}.can_convert(initialization->type().get_kind(), type->get_kind()))
                    //{
                    //    // Todo cannot convert from init to declared type
                    //    raise_error();
                    //    return;
                    //}
                }

                m_ids_context.register_identifier(node.get_name(), type);
                m_result_node = std::make_unique<variable_declaration_node>(*type, node.get_name(), std::move(initialization));
            }

            void visit(const ast::while_node& node) override
            {
                auto conditional = visit_child_node<conditional_node>(node.get_node());
                if(!conditional)
                {
                    return;
                }

                m_result_node = std::make_unique<while_node>(std::move(conditional));
            }

        public:
            std::unique_ptr<sema_node> m_result_node;

        private:
            template <typename T>
            std::unique_ptr<T> to_node(std::unique_ptr<sema_node> node) const
            {
                return std::unique_ptr<T>(dynamic_cast<T*>(node.release()));
            }

            std::unique_ptr<expression_node> to_expression(std::unique_ptr<sema_node> node) const
            {
                return std::unique_ptr<expression_node>(dynamic_cast<expression_node*>(node.release()));
            }

            void raise_error()
            {
                m_errors_observer.nofify_error({}); // todo: get error via parameter
            }

            sema_builder_ast_visitor clone() const
            {
                return sema_builder_ast_visitor{ m_ctx, m_errors_observer, m_ids_context, m_function_factory };
            }

            std::unique_ptr<expression_node> visit_child_expr(const ast::ast_node& node)
            {
                return to_node<expression_node>(visit_child(node));
            }

            template <typename T>
            std::unique_ptr<T> visit_child_node(const ast::ast_node& node)
            {
                return to_node<T>(visit_child(node));
            }

            std::unique_ptr<sema_node> visit_child(const ast::ast_node& node)
            {
                auto v = clone();
                node.visit(v);
                return std::move(v.m_result_node);
            }

            boost::optional<param_expressions_t> get_function_call_params(const sema_function::function_signature& signature, const ast::call_node::params_t& passed_params)
            {
                const auto& param_declarations = signature.params;

                if(param_declarations.size() != passed_params.size())
                {
                    // Todo passed wrong number of parameters
                    raise_error();
                    return {};
                }

                std::vector<std::unique_ptr<expression_node>> params;

                for(auto i = 0u; i < param_declarations.size(); ++i)
                {
                    auto param = visit_child_expr(*passed_params[i]);
                    if(!param)
                    {
                        return {};
                    }

                    if(param_declarations[i].ty != param->type())
                    {
                        //Todo passed param type mismatch
                        raise_error();
                        return {};
                    }

                    params.emplace_back(std::move(param));
                }

                return params;
            }

            struct ids_ctx_guard
            {
                explicit ids_ctx_guard(identifiers_context& ids_context)
                    : m_ids_ctx{ ids_context }
                {
                    m_ids_ctx.enter_ctx();
                }

                ~ids_ctx_guard()
                {
                    m_ids_ctx.leave_ctx();
                }

                identifiers_context& m_ids_ctx;
            };

            ids_ctx_guard ids_guard()
            {
                return ids_ctx_guard{ m_ids_context };
            }

            struct function_declaration
            {
                sema_function_impl* fun{ nullptr };
                const ast::block_node& body_to_visit;
            };

            struct class_members
            {
                std::vector<sema_type::member_info> info;
                std::vector<std::unique_ptr<variable_declaration_node>> declarations;
                std::vector<function_declaration> functions;
            };

            boost::optional<function_declaration> get_function_declaration_and_add_to_ctx(const ast::user_function_node2& node)
            {
                auto return_type = m_ctx.find_type(node.get_return_type_reference().to_string());
                if(!return_type)
                {
                    // Todo: unknown return type
                    raise_error();
                    return {};
                }

                // Todo: parameters need to be in the same ids context as function body.
                auto params_ids_guard = ids_guard();

                using param_decl_t = sema_function::parameter_declaration;
                std::vector<param_decl_t> params;

                for(const auto& param_decl : node.get_param_declarations())
                {
                    auto param_type = m_ctx.find_type(param_decl.ty.to_string());
                    if(!param_type)
                    {
                        //Todo: unknown parameter type
                        raise_error();
                        return {};
                    }

                    params.emplace_back(param_decl_t{*param_type, param_decl.name});
                    m_ids_context.register_identifier(param_decl.name, param_type);
                }

                sema_function::function_signature signature{
                        node.get_name(),
                        std::move(params)
                };
                auto& function = m_function_factory.create( m_ctx, *return_type, std::move(signature) );
                m_ctx.add_function(function);

                return function_declaration{
                        &function,
                        node.get_body()
                };
            }

            boost::optional<class_members> collect_class_members_and_add_functions_to_ctx(const ast::class_node2& node)
            {
                class_members members;

                for(auto n : node.get_nodes())
                {
                    if (auto variable_decl = dynamic_cast<const ast::variable_declaration_node *>(n))
                    {
                        auto member = visit_child_node<variable_declaration_node>(*variable_decl);
                        if (!member)
                        {
                            return {};
                        }

                        members.info.emplace_back(sema_type::member_info{member->name(), member->type()});
                        members.declarations.emplace_back(std::move(member));
                    }
                    else if(auto fun_node = dynamic_cast<const ast::user_function_node2*>(n))
                    {
                        auto function_declaration = get_function_declaration_and_add_to_ctx(*fun_node);
                        if(!function_declaration)
                        {
                            return {};
                        }

                        members.functions.emplace_back(*function_declaration);
                    }
                }

                return members;
            }

        private:
            sema_context_interface& m_ctx;
            errors::errors_observer& m_errors_observer;
            identifiers_context& m_ids_context;
            sema_function_factory& m_function_factory;
        };
    }
}
