#include "sema/sema_builder_ast_visitor.hpp"

#include "ast/block_node.hpp"
#include "ast/break_node.hpp"
#include "ast/class_node.hpp"
#include "ast/conditional_node.hpp"
#include "ast/enum_node.hpp"
#include "ast/for_node.hpp"
#include "ast/if_else_node.hpp"
#include "ast/import_node.hpp"
#include "ast/infix_nodes.hpp"
#include "ast/namespace_node.hpp"
#include "ast/return_node.hpp"
#include "ast/ternary_operator_node.hpp"
#include "ast/translation_unit_node.hpp"
#include "ast/user_function_node.hpp"
#include "ast/variable_declaration_node.hpp"
#include "ast/while_node.hpp"
#include "builtin_function_kind.hpp"
#include "common/algorithm.hpp"
#include "common/assert.hpp"
#include "common/overloaded.hpp"
#include "errors/error.hpp"
#include "errors/errors_observer.hpp"
#include "sema/add_declarative_file_semantic_handler.hpp"
#include "sema/add_subdirectory_semantic_handler.hpp"
#include "sema/block_node_manipulator.hpp"
#include "sema/builtin_sema_function.hpp"
#include "sema/enum_creator.hpp"
#include "sema/enum_values_context.hpp"
#include "sema/factories.hpp"
#include "sema/factories_provider.hpp"
#include "sema/failed_initialization_errors_reporters.hpp"
#include "sema/functions_context.hpp"
#include "sema/identifiers_context.hpp"
#include "sema/identifiers_index_provider.hpp"
#include "sema/import_handler.hpp"
#include "sema/overload_resolution.hpp"
#include "sema/sema_context.hpp"
#include "sema/sema_nodes.hpp"
#include "sema/sema_type.hpp"
#include "sema/type_builder.hpp"
#include "sema/types_context.hpp"
#include "sema/user_sema_function.hpp"
#include "sema/variable_initialization_checker.hpp"

#include <ast/designated_initializers_node.hpp>
#include <stack>
#include <unordered_set>

namespace cmsl::sema {
sema_builder_ast_visitor::sema_builder_ast_visitor(
  sema_builder_ast_visitor_members& members)
  : m_{ members }
{
}

void sema_builder_ast_visitor::visit(const ast::block_node& node)
{
  auto ig = m_.qualified_ctxs.local_ids_guard();
  std::vector<std::unique_ptr<sema_node>> nodes;

  for (const auto& n : node.nodes()) {
    auto child = visit_child(*n);
    if (!child) {
      return;
    }

    nodes.emplace_back(std::move(child));
  }

  m_result_node = std::make_unique<block_node>(node, std::move(nodes));
}

void sema_builder_ast_visitor::visit(const ast::class_node& node)
{
  const auto name = node.name();
  const auto class_name_representation =
    ast::type_representation{ ast::qualified_name{ name } };
  if (m_.qualified_ctxs.types.find_in_current_scope(name) != nullptr) {
    raise_error(name, "Type redefinition");
    return;
  }

  const auto is_exported = node.export_().has_value();

  auto class_ids_guard = m_.qualified_ctxs.global_ids_guard(name, is_exported);
  auto functions_guard =
    m_.qualified_ctxs.functions_ctx_guard(name, is_exported);
  auto enums_guard = m_.qualified_ctxs.enums_ctx_guard(name, is_exported);

  auto& class_context = m_.factories.context_factory().create_class(
    std::string{ name.str() }, &m_.ctx);

  auto members =
    collect_class_members_and_add_functions_to_ctx(node, class_context);

  if (!members) {
    return;
  }

  auto type_factory = m_.factories.type_factory(m_.qualified_ctxs.types);
  auto& class_type =
    type_factory.create(class_context, class_name_representation,
                        std::move(members->info), is_exported);
  auto& class_type_reference =
    type_factory.create_reference(class_type, is_exported);

  auto types_guard = m_.qualified_ctxs.types_ctx_guard(name, is_exported);

  // Same as with ast context. class_type raw pointer will be valid. We move it
  // to context to make this class findable as a regular type (so e.g. inside
  // this class methods, the type will appear as a regular type).
  m_.ctx.add_type(class_type);
  m_.ctx.add_type(class_type_reference);

  std::vector<std::unique_ptr<function_node>> functions;

  for (auto function_declaration : members->functions) {
    auto function_params_guard = m_.qualified_ctxs.local_ids_guard();
    for (const auto& param_decl :
         function_declaration.fun->signature().params) {
      m_.qualified_ctxs.ids.register_identifier(
        param_decl.name, { param_decl.ty, param_decl.index },
        /*exported=*/false);
    }

    m_.parsing_ctx.function_parsing_ctx.function = function_declaration.fun;
    m_.parsing_ctx.function_parsing_ctx.return_nodes.clear();
    auto body = visit_child_node<block_node>(
      function_declaration.body_to_visit, class_context);
    m_.parsing_ctx.function_parsing_ctx.function = nullptr;
    if (!body) {
      return;
    }

    add_implicit_return_node_if_need(*body);

    if (function_declaration.should_deduce_return_type) {
      const auto return_type =
        try_deduce_currently_parsed_function_return_type();
      if (!return_type) {
        return;
      }
      function_declaration.fun->set_return_type(*return_type);
    }

    function_declaration.fun->set_body(*body);
    functions.emplace_back(std::make_unique<function_node>(
      function_declaration.ast_function_node, *function_declaration.fun,
      std::move(body)));
  }

  add_user_type_default_methods(class_type, class_context);

  m_result_node = std::make_unique<class_node>(
    node, name, std::move(members->declarations), std::move(functions));
}

void sema_builder_ast_visitor::visit(const ast::conditional_node& node)
{
  auto ig = m_.qualified_ctxs.local_ids_guard();

  auto condition = visit_child_expr(node.condition());
  if (!condition) {
    return;
  }

  auto body = visit_child_node<block_node>(node.body());
  if (!body) {
    return;
  }

  m_result_node = std::make_unique<conditional_node>(
    node, std::move(condition), std::move(body));
}

void sema_builder_ast_visitor::visit(const ast::if_else_node& node)
{
  std::vector<std::unique_ptr<conditional_node>> ifs;

  for (const auto& if_ : node.ifs()) {
    auto cond = visit_child_node<conditional_node>(*if_.conditional);
    if (!cond) {
      return;
    }

    ifs.emplace_back(std::move(cond));
  }

  std::unique_ptr<block_node> else_body;

  if (auto else_node = node.else_body()) {
    auto ig = m_.qualified_ctxs.local_ids_guard();

    else_body = visit_child_node<block_node>(*else_node);
    if (!else_body) {
      return;
    }
  }

  m_result_node =
    std::make_unique<if_else_node>(node, std::move(ifs), std::move(else_body));
}

void sema_builder_ast_visitor::visit(const ast::binary_operator_node& node)
{
  auto lhs = visit_child_expr(node.lhs());
  if (!lhs) {
    return;
  }

  // Todo: handle operators like ++ and ++(int)
  const auto op = node.operator_();
  auto lookup_result = lhs->type().find_member_function(op);
  if (lookup_result.empty()) {
    raise_error(op,
                lhs->type().name().to_string() + " doesn't support operator " +
                  std::string{ op.str() });
    return;
  }

  auto rhs = visit_child_expr(node.rhs());
  if (!rhs) {
    return;
  }

  overload_resolution over_resolution{ m_.errors_observer, op };
  const auto chosen_function = over_resolution.choose(lookup_result, *rhs);
  if (!chosen_function) {
    return;
  }

  m_result_node = std::make_unique<binary_operator_node>(
    node, std::move(lhs), node.operator_(), *chosen_function, std::move(rhs),
    chosen_function->return_type());
}

void sema_builder_ast_visitor::visit(const ast::class_member_access_node& node)
{
  auto lhs = visit_child_expr(node.lhs());
  if (!lhs) {
    return;
  }

  const auto& lhs_type = lhs->type();
  const auto member_name = node.member_name();
  auto member_info = lhs_type.find_member(member_name.str());
  if (!member_info) {
    raise_error(member_name,
                lhs_type.name().to_string() + " does not have member " +
                  std::string{ member_name.str() });
    return;
  }

  m_result_node = std::make_unique<class_member_access_node>(
    node, std::move(lhs), member_name, *member_info);
}

std::unique_ptr<expression_node> sema_builder_ast_visitor::build_function_call(
  const ast::function_call_node& node)
{
  auto params = get_function_call_params(node.param_nodes());
  if (!params) {
    return nullptr;
  }

  const auto function_lookup_result = node.is_generic_type_constructor_call()
    ? find_generict_type_constructor_functions(node.generic_type_name())
    : find_functions(node.names());
  overload_resolution over_resolution{ m_.errors_observer, node.name() };
  const auto chosen_function =
    over_resolution.choose(function_lookup_result, *params);
  if (!chosen_function) {
    return nullptr;
  }

  const auto chosen_function_return_type_not_known_yet =
    chosen_function->try_return_type() == nullptr;
  if (chosen_function == m_.parsing_ctx.function_parsing_ctx.function) {
    if (chosen_function_return_type_not_known_yet) {
      const auto source = node.name().source();
      const auto range = node.name().src_range();
      raise_error(
        source, range,
        "recursively calling function with 'auto' return type is forbidden");
      return nullptr;
    }
  }

  if (chosen_function_return_type_not_known_yet) {
    const auto source = node.name().source();
    const auto range = node.name().src_range();
    raise_error(source, range,
                "using function with 'auto' return type before its definition "
                "is forbidden");
    return nullptr;
  }

  // Convert call parameter nodes if need, e.g. to a cast_to_reference_node, if
  // function accepts a reference.
  params = convert_params_to_cast_nodes_if_need(chosen_function->signature(),
                                                std::move(*params));

  switch (chosen_function->context().type()) {
    case sema_context::context_type::namespace_: {
      return std::make_unique<function_call_node>(
        node, *chosen_function, std::move(*params), node.name());
    } break;
    case sema_context::context_type::class_: {
      const auto& name = chosen_function->return_type().name();
      const auto& function_name =
        name.is_generic() ? name.primary_name_token().str() : name.to_string();
      const auto is_constructor =
        chosen_function->signature().name.str() == function_name;

      if (is_constructor) {
        return std::make_unique<constructor_call_node>(
          node, chosen_function->return_type(), *chosen_function,
          std::move(*params), node.name());
      } else {
        return std::make_unique<implicit_member_function_call_node>(
          node, *chosen_function, std::move(*params), node.name());
      }
    } break;

    default:
      CMSL_UNREACHABLE("Unknown context type");
  }

  return nullptr;
}

std::unique_ptr<expression_node>
sema_builder_ast_visitor::build_add_subdirectory_call(
  const ast::function_call_node& node)
{
  auto params_opt = get_function_call_params(node.param_nodes());
  if (!params_opt) {
    return nullptr;
  }

  auto& params = *params_opt;
  if (params.empty()) {
    raise_error(node.name(), "expected subdirectory name");
    return nullptr;
  }

  auto casted = dynamic_cast<string_value_node*>(params[0].get());
  if (!casted) {
    raise_error(node.name(), "expected string literal as a subdirectory name");
    return nullptr;
  }

  (void)params[0].release();
  auto name_string_node = std::unique_ptr<string_value_node>(casted);
  const auto name = name_string_node->value();

  std::vector<std::unique_ptr<expression_node>> params_but_name;
  std::transform(std::next(std::begin(params)), std::end(params),
                 std::back_inserter(params_but_name),
                 [](auto& param) { return std::move(param); });

  const auto result =
    m_.add_subdir_handler.handle_add_subdirectory(name, params_but_name);

  const auto visitor = overloaded{
    [&](const add_subdirectory_semantic_handler::contains_cmakesl_script& val)
      -> std::unique_ptr<expression_node> {
      return std::make_unique<add_subdirectory_node>(
        node, std::move(name_string_node), *val.main_function,
        std::move(params_but_name), node.name());
    },
    [&](const add_subdirectory_semantic_handler::
          contains_declarative_cmakesl_script& val)
      -> std::unique_ptr<expression_node> {
      return std::make_unique<add_subdirectory_with_declarative_script_node>(
        node, std::move(name_string_node), *val.component_creation_function);
    },
    [&](
      const add_subdirectory_semantic_handler::contains_old_cmake_script& val)
      -> std::unique_ptr<expression_node> {
      return std::make_unique<add_subdirectory_with_old_script_node>(
        node, std::move(name_string_node), m_.builtin_types.void_);
    },
    [](auto) -> std::unique_ptr<expression_node> { return nullptr; }
  };

  return std::visit(visitor, result);
}

std::unique_ptr<expression_node>
sema_builder_ast_visitor::build_add_declarative_file_call(
  const ast::function_call_node& node)
{
  auto params_opt = get_function_call_params(node.param_nodes());
  if (!params_opt) {
    return nullptr;
  }

  auto& params = *params_opt;
  if (params.empty()) {
    raise_error(node.name(), "expected declarative file name");
    return nullptr;
  }

  auto casted = dynamic_cast<string_value_node*>(params[0].get());
  if (!casted) {
    raise_error(node.name(),
                "expected string literal as a declarative file name");
    return nullptr;
  }

  (void)params[0].release();
  auto file_path_string_node = std::unique_ptr<string_value_node>(casted);
  const auto name = file_path_string_node->value();

  const auto result =
    m_.add_declarative_file_handler.handle_add_declarative_file(name);
  (void)result;

  const auto visitor =
    overloaded{ [&](const add_declarative_file_semantic_handler::
                      contains_declarative_cmakesl_script& val)
                  -> std::unique_ptr<expression_node> {
                 return std::make_unique<add_declarative_file_node>(
                   node, std::move(file_path_string_node),
                   *val.target_creation_function, node.name());
               },
                [](auto) -> std::unique_ptr<expression_node> {
                  return nullptr;
                } };

  return std::visit(visitor, result);
}

void sema_builder_ast_visitor::visit(const ast::function_call_node& node)
{
  if (node.name().str() == "add_subdirectory") {
    m_result_node = build_add_subdirectory_call(node);
  } else if (node.name().str() == "add_declarative_file") {
    m_result_node = build_add_declarative_file_call(node);
  } else {
    m_result_node = build_function_call(node);
  }
}

void sema_builder_ast_visitor::visit(
  const ast::member_function_call_node& node)
{
  const auto name = node.name();
  auto lhs = visit_child_expr(node.lhs());
  if (!lhs) {
    return;
  }

  auto params = get_function_call_params(node.param_nodes());
  if (!params) {
    return;
  }

  const auto member_functions = lhs->type().find_member_function(name);
  overload_resolution over_resolution{ m_.errors_observer, node.name() };
  const auto chosen_function =
    over_resolution.choose(member_functions, *params);
  if (!chosen_function) {
    return;
  }

  // Convert call parameter nodes if need, e.g. to a cast_to_reference_node, if
  // function accepts a reference.
  params = convert_params_to_cast_nodes_if_need(chosen_function->signature(),
                                                std::move(*params));

  m_result_node = std::make_unique<member_function_call_node>(
    node, std::move(lhs), *chosen_function, std::move(*params), node.name());
}

void sema_builder_ast_visitor::visit(const ast::bool_value_node& node)
{
  const auto& type = m_.builtin_types.bool_;
  const auto value = node.token().str() == "true";
  m_result_node = std::make_unique<bool_value_node>(node, type, value);
}

void sema_builder_ast_visitor::visit(const ast::int_value_node& node)
{
  const auto& type = m_.builtin_types.int_;
  const auto token = node.token();
  char* end;
  const auto value =
    std::strtol(token.str().data(), &end, 10); // todo: handle hex etc
  m_result_node = std::make_unique<int_value_node>(node, type, value);
}

void sema_builder_ast_visitor::visit(const ast::double_value_node& node)
{
  const auto& type = m_.builtin_types.double_;
  const auto token = node.token();
  char* end;
  const auto value =
    std::strtod(token.str().data(), &end); // todo: handle hex etc
  m_result_node = std::make_unique<double_value_node>(node, type, value);
}

void sema_builder_ast_visitor::visit(const ast::string_value_node& node)
{
  const auto& type = m_.builtin_types.string;

  // At this point node contains string value including "". We need to get rid
  // of them.
  const auto node_string = node.view();
  const auto string_without_quotation_marks =
    cmsl::string_view{ std::next(node_string.data()),
                       node_string.size() - 2u };

  m_result_node = std::make_unique<string_value_node>(
    node, type, string_without_quotation_marks);
}

void sema_builder_ast_visitor::visit(const ast::id_node& node)
{
  const auto& names = node.names();
  const auto id_token = names.back().name;

  if (const auto info = m_.qualified_ctxs.ids.info_of(names)) {
    m_result_node =
      std::make_unique<id_node>(node, info->type, names, info->index);
    return;
  } else if (const auto enum_info = m_.qualified_ctxs.enums.info_of(names)) {
    m_result_node = std::make_unique<enum_constant_access_node>(
      node, enum_info->type, names, enum_info->value, enum_info->index);
    return;
  }

  raise_error(id_token,
              '\'' + std::string{ id_token.str() } + "' identifier not found");
}

void sema_builder_ast_visitor::visit(const ast::return_node& node)
{
  auto v = clone();
  node.expression().visit(v);
  if (!v.m_result_node) {
    return;
  }

  auto expr = to_expression(std::move(v.m_result_node));

  if (!check_function_return_type(node.return_kw(), *expr)) {
    return;
  }

  expr = convert_to_cast_return_node_if_need(std::move(expr));
  auto ret_node = std::make_unique<return_node>(node, std::move(expr));
  m_.parsing_ctx.function_parsing_ctx.return_nodes.emplace_back(
    ret_node.get());
  m_result_node = std::move(ret_node);
}

void sema_builder_ast_visitor::visit(const ast::translation_unit_node& node)
{
  std::vector<std::unique_ptr<sema_node>> nodes;

  for (const auto& n : node.nodes()) {
    auto visited_node = visit_child(*n);
    if (!visited_node) {
      return;
    }

    nodes.emplace_back(std::move(visited_node));
  }

  m_result_node =
    std::make_unique<translation_unit_node>(node, m_.ctx, std::move(nodes));
}

void sema_builder_ast_visitor::visit(const ast::user_function_node& node)
{
  if (node.is_exported() && !is_export_allowed()) {
    raise_error(*node.export_(), "exporting member functions is not allowed");
    return;
  }

  const auto& return_type_representation = node.return_type_representation();
  const auto should_deduce_return_type = return_type_representation.is_auto();

  const sema_type* return_type{ nullptr };

  if (!should_deduce_return_type) {
    return_type =
      try_get_or_create_generic_type(m_.ctx, return_type_representation);
    if (!return_type) {
      return;
    }
  }

  // Todo: parameters need to be in the same ids context as function body.
  auto params_ids_guard = m_.qualified_ctxs.local_ids_guard();

  using param_decl_t = parameter_declaration;
  std::vector<param_decl_t> params;

  for (const auto& param_decl : node.param_declarations()) {
    auto param_type = try_get_or_create_generic_type(m_.ctx, param_decl.ty);
    if (!param_type) {
      return;
    }

    const auto identifier_index = identifiers_index_provider::get_next();
    params.emplace_back(
      param_decl_t{ *param_type, param_decl.name, identifier_index });
    m_.qualified_ctxs.ids.register_identifier(
      param_decl.name, { *param_type, identifier_index }, /*exported=*/false);
  }

  // Todo: add test for function redefinition.
  function_signature signature{ node.name(), std::move(params) };
  if (raise_error_if_function_redefined(signature)) {
    return;
  }
  auto& function = m_.factories.function_factory().create_user(
    m_.ctx, return_type, std::move(signature));

  // Add function (without a body yet) to context, so it will be visible inside
  // function body in case of a recursive call.
  m_.ctx.add_function(function);
  const auto is_exported = node.export_().has_value();
  m_.qualified_ctxs.functions.register_function(node.name(), function,
                                                is_exported);

  // Store pointer to function that is currently parsed,
  // so function body will be able to figure out function return type
  // and make casted return expression nodes as needed.
  m_.parsing_ctx.function_parsing_ctx.function = &function;
  m_.parsing_ctx.function_parsing_ctx.return_nodes.clear();
  auto block = visit_child_node<block_node>(node.body());
  m_.parsing_ctx.function_parsing_ctx.function = nullptr;
  if (!block) {
    return;
  }

  if (should_deduce_return_type) {
    return_type = try_deduce_currently_parsed_function_return_type();
    if (!return_type) {
      return;
    }
    function.set_return_type(*return_type);
  }

  add_implicit_return_node_if_need(*block);

  // And set the body.
  function.set_body(*block);

  m_result_node =
    std::make_unique<function_node>(node, function, std::move(block));
}

void sema_builder_ast_visitor::visit(
  const ast::standalone_variable_declaration_node& node)
{
  node.variable_declaration().visit(*this);
}

void sema_builder_ast_visitor::visit(const ast::while_node& node)
{
  ++m_.parsing_ctx.loop_parsing_counter;
  auto conditional = visit_child_node<conditional_node>(node.node());
  --m_.parsing_ctx.loop_parsing_counter;
  if (!conditional) {
    return;
  }

  m_result_node = std::make_unique<while_node>(node, std::move(conditional));
}

const sema_type* sema_builder_ast_visitor::try_get_or_create_generic_type(
  const sema_context& search_context, const ast::type_representation& name)
{
  if (!name.is_generic()) {
    const auto found = m_.qualified_ctxs.types.find(name.qual_name().names());

    if (!found) {
      raise_error(name.source(), name.src_range(),
                  name.to_string() + " type not found.");
      return nullptr;
    }

    if (name.is_reference()) {
      return &found->ref;
    }

    return &found->ty;
  }

  // Todo: introduce lexer::is_generic_type_token
  const auto proper_generic_token_types = { lexer::token_type::kw_list,
                                            lexer::token_type::kw_extern };

  const auto& primary_name = name.generic_name().primary_name();
  const auto is_proper_generic_type =
    contains(proper_generic_token_types, primary_name.get_type());
  if (!is_proper_generic_type) {
    raise_error(primary_name,
                "'" + std::string{ primary_name.str() } +
                  "' is not a generic type.");
    return nullptr;
  }

  if (const auto found = m_.qualified_ctxs.types.find_generic(name)) {
    return found;
  }

  auto factory = sema_generic_type_factory{
    m_.generic_types_context, search_context,    m_.factories,
    m_.errors_observer,       m_.builtin_tokens, m_.builtin_types,
    m_.qualified_ctxs.types
  };

  return factory.create_generic(name);
}

template <typename T>
std::unique_ptr<T> sema_builder_ast_visitor::to_node(
  std::unique_ptr<sema_node> node) const
{
  return std::unique_ptr<T>(dynamic_cast<T*>(node.release()));
}

std::unique_ptr<expression_node> sema_builder_ast_visitor::to_expression(
  std::unique_ptr<sema_node> node) const
{
  return std::unique_ptr<expression_node>(
    dynamic_cast<expression_node*>(node.release()));
}

void sema_builder_ast_visitor::raise_note(const lexer::token& token,
                                          const std::string& message)
{
  raise_note(token.source(), token.src_range(), message);
}

void sema_builder_ast_visitor::raise_note(const source_view& source,
                                          const source_range& src_range,
                                          const std::string& message)
{
  notify_error_observer(source, src_range, message, errors::error_type::note);
}

void sema_builder_ast_visitor::raise_error(const lexer::token& token,
                                           const std::string& message)
{
  raise_error(token.source(), token.src_range(), message);
}

void sema_builder_ast_visitor::raise_error(const source_view& source,
                                           const source_range& src_range,
                                           const std::string& message)
{
  notify_error_observer(source, src_range, message, errors::error_type::error);
}

void sema_builder_ast_visitor::notify_error_observer(
  const source_view& source, const source_range& src_range,
  const std::string& message, errors::error_type err_type)
{
  const auto line_info = source.line(src_range.begin.line);
  const auto err =
    errors::error{ source.path(), line_info.line, line_info.start_pos,
                   message,       err_type,       src_range };
  m_.errors_observer.notify_error(err);
}

sema_builder_ast_visitor sema_builder_ast_visitor::clone() const
{
  return sema_builder_ast_visitor{ m_ };
}

sema_builder_ast_visitor sema_builder_ast_visitor::clone(
  sema_context& ctx_to_visit) const
{
  auto members =
    sema_builder_ast_visitor_members{ m_.generic_types_context,
                                      ctx_to_visit,
                                      m_.errors_observer,
                                      m_.qualified_ctxs,
                                      m_.factories,
                                      m_.builtin_tokens,
                                      m_.parsing_ctx,
                                      m_.builtin_types,
                                      m_.add_subdir_handler,
                                      m_.add_declarative_file_handler,
                                      m_.imports_handler };

  return sema_builder_ast_visitor{ members };
}

std::unique_ptr<expression_node> sema_builder_ast_visitor::visit_child_expr(
  const ast::ast_node& node)
{
  return to_node<expression_node>(visit_child(node));
}

template <typename T>
std::unique_ptr<T> sema_builder_ast_visitor::visit_child_node(
  const ast::ast_node& node)
{
  return to_node<T>(visit_child(node));
}

template <typename T>
std::unique_ptr<T> sema_builder_ast_visitor::visit_child_node(
  const ast::ast_node& node, sema_context& ctx_to_visit)
{
  return to_node<T>(visit_child(node, ctx_to_visit));
}

std::unique_ptr<sema_node> sema_builder_ast_visitor::visit_child(
  const ast::ast_node& node)
{
  auto v = clone();
  node.visit(v);
  return std::move(v.m_result_node);
}

std::unique_ptr<sema_node> sema_builder_ast_visitor::visit_child(
  const ast::ast_node& node, sema_context& ctx_to_visit)
{
  auto members =
    sema_builder_ast_visitor_members{ m_.generic_types_context,
                                      ctx_to_visit,
                                      m_.errors_observer,
                                      m_.qualified_ctxs,
                                      m_.factories,
                                      m_.builtin_tokens,
                                      m_.parsing_ctx,
                                      m_.builtin_types,
                                      m_.add_subdir_handler,
                                      m_.add_declarative_file_handler,
                                      m_.imports_handler };
  auto v = sema_builder_ast_visitor{ members };
  node.visit(v);
  return std::move(v.m_result_node);
}

std::optional<sema_builder_ast_visitor::param_expressions_t>
sema_builder_ast_visitor::get_function_call_params(
  const ast::call_node::params_t& passed_params)
{
  std::vector<std::unique_ptr<expression_node>> params;

  for (const auto& passed_param : passed_params) {
    auto param = visit_child_expr(*passed_param);
    if (!param) {
      return {};
    }

    params.emplace_back(std::move(param));
  }

  return std::move(params);
}

std::optional<sema_builder_ast_visitor::function_declaration>
sema_builder_ast_visitor::get_function_declaration_and_add_to_ctx(
  const ast::user_function_node& node, sema_context_impl& ctx)
{
  const auto return_type_reference = node.return_type_representation();
  const auto should_deduce_return_type = return_type_reference.is_auto();

  const sema_type* return_type{ nullptr };

  if (!should_deduce_return_type) {
    return_type = try_get_or_create_generic_type(ctx, return_type_reference);
    if (!return_type) {
      return {};
    }
  }

  // Todo: parameters need to be in the same ids context as function body.
  auto params_ids_guard = m_.qualified_ctxs.local_ids_guard();

  std::vector<parameter_declaration> params;

  for (const auto& param_decl : node.param_declarations()) {
    auto param_type = try_get_or_create_generic_type(ctx, param_decl.ty);
    if (!param_type) {
      const auto& name = param_decl.ty;
      raise_error(name.source(), name.src_range(),
                  name.to_string() + " unknown parameter type");
      return {};
    }

    const auto param_identifier_index = identifiers_index_provider::get_next();
    params.emplace_back(parameter_declaration{ *param_type, param_decl.name,
                                               param_identifier_index });
    const auto idenfitied_context = identifiers_index_provider::get_next();
    m_.qualified_ctxs.ids.register_identifier(
      param_decl.name, { *param_type, idenfitied_context },
      /*exported=*/false);
  }

  function_signature signature{ node.name(), std::move(params) };
  if (raise_error_if_function_redefined(signature)) {
    return std::nullopt;
  }

  auto& function = m_.factories.function_factory().create_user(
    ctx, return_type, std::move(signature));
  const auto is_exported = node.export_().has_value();
  m_.qualified_ctxs.functions.register_function(signature.name, function,
                                                is_exported);
  ctx.add_function(function);

  return function_declaration{ node, &function, node.body(),
                               should_deduce_return_type };
}

std::optional<sema_builder_ast_visitor::class_members>
sema_builder_ast_visitor::collect_class_members_and_add_functions_to_ctx(
  const ast::class_node& node, sema_context_impl& class_context)
{
  class_members members;

  for (const auto& n : node.nodes()) {
    if (auto variable_decl =
          dynamic_cast<const ast::standalone_variable_declaration_node*>(
            n.get())) {
      auto member =
        visit_child_node<variable_declaration_node>(*variable_decl);
      if (!member) {
        return {};
      }

      members.info.emplace_back(
        member_info{ member->name(), member->type(), member->index() });
      members.declarations.emplace_back(std::move(member));
    } else if (auto fun_node =
                 dynamic_cast<const ast::user_function_node*>(n.get())) {
      auto function_declaration =
        get_function_declaration_and_add_to_ctx(*fun_node, class_context);
      if (!function_declaration) {
        return {};
      }

      members.functions.emplace_back(*function_declaration);
    }
  }

  return std::move(members);
}

template <unsigned N>
lexer::token sema_builder_ast_visitor::make_token(lexer::token_type token_type,
                                                  const char (&tok)[N])
{
  // N counts also '\0'
  const auto src_range = source_range{ source_location{ 1u, 1u, 0u },
                                       source_location{ 1u, N, N - 1u } };
  return lexer::token{ token_type, src_range, source_view{ tok } };
}

std::unique_ptr<expression_node>
sema_builder_ast_visitor::convert_to_cast_return_node_if_need(
  std::unique_ptr<expression_node> expression)
{
  if (!m_.parsing_ctx.function_parsing_ctx.function ||
      !m_.parsing_ctx.function_parsing_ctx.function->try_return_type()) {
    return expression;
  }

  const auto& expected_result_type =
    m_.parsing_ctx.function_parsing_ctx.function->return_type();
  return convert_to_cast_node_if_need(expected_result_type,
                                      std::move(expression));
}

std::unique_ptr<expression_node>
sema_builder_ast_visitor::convert_to_cast_node_if_need(
  const sema_type& expected_result_type,
  std::unique_ptr<expression_node> expression)
{
  // Inside this function, we're assured that the initialization is
  // semantically valid.

  // Designated initializers never need to be casted. Semantic for them has
  // been already checked by the variable_initialization_checker.
  if (expression->type().is_designated_initializer()) {
    return expression;
  }

  if (expected_result_type == expression->type()) {
    return expression;
  }

  const auto& ast_node = expression->ast_node();

  const auto cast_to_base_is_needed =
    (expected_result_type.decayed() != expression->type().decayed());
  if (cast_to_base_is_needed) {
    if (expected_result_type.is_reference()) {
      return std::make_unique<cast_to_reference_to_base_node>(
        ast_node, expected_result_type, std::move(expression));

    } else {
      return std::make_unique<cast_to_base_value_node>(
        ast_node, expected_result_type, std::move(expression));
    }
  }

  if (expected_result_type.is_reference()) {
    return std::make_unique<cast_to_reference_node>(
      ast_node, expected_result_type, std::move(expression));
  } else {
    return std::make_unique<cast_to_value_node>(ast_node, expected_result_type,
                                                std::move(expression));
  }
}

sema_builder_ast_visitor::param_expressions_t
sema_builder_ast_visitor::convert_params_to_cast_nodes_if_need(
  const function_signature& signature, param_expressions_t params)
{
  for (auto i = 0u; i < params.size(); ++i) {
    const auto& declared_param_type = signature.params[i].ty;
    params[i] =
      convert_to_cast_node_if_need(declared_param_type, std::move(params[i]));
  }

  return params;
}

void sema_builder_ast_visitor::visit(const ast::initializer_list_node& node)
{
  if (node.values().empty()) {
    raise_error(node.open_brace(), "initializer lsit must contain values");
    return;
  }

  std::vector<std::unique_ptr<expression_node>> expression_values;
  const sema_type* value_type{ nullptr };

  for (const auto& value : node.values()) {
    auto expression = visit_child_expr(*value);
    if (!expression) {
      return;
    }

    if (!expression->produces_temporary_value()) {
      const auto& desired_type = expression->type().is_reference()
        ? expression->type().referenced_type()
        : expression->type();
      expression = std::make_unique<cast_to_value_node>(
        expression->ast_node(), desired_type, std::move(expression));
    }

    const auto& expr_type = expression->type();
    if (value_type && *value_type != expr_type) {
      raise_error(node.open_brace(),
                  "all values of initializer list must have the same type");
      return;
    }

    if (!value_type) {
      value_type = &expr_type;
    }

    expression_values.emplace_back(std::move(expression));
  }

  // Todo: Introduce utility to create generic name representations
  using token_type_t = lexer::token_type;
  lexer::token_container_t tokens{ make_token(token_type_t::kw_list, "list"),
                                   make_token(token_type_t::less, "<") };
  const auto& value_type_tokens = value_type->name().tokens();
  tokens.insert(std::end(tokens), std::cbegin(value_type_tokens),
                std::cend(value_type_tokens));
  tokens.emplace_back(make_token(token_type_t::greater, ">"));

  auto generic_name =
    ast::type_representation::generic_type_name{ std::move(tokens),
                                                 { value_type->name() } };
  const auto list_type_representation =
    ast::type_representation{ std::move(generic_name) };

  auto type = try_get_or_create_generic_type(m_.ctx, list_type_representation);
  if (!type) {
    // Should be impossible to get here.
    raise_error(node.open_brace(), "could not create initalizer_list type");
    return;
  }

  m_result_node = std::make_unique<initializer_list_node>(
    node, *type, std::move(expression_values));
}

void sema_builder_ast_visitor::add_implicit_return_node_if_need(
  block_node& block)
{
  if (is_last_node_return_node(block)) {
    return;
  }

  const auto& ast_node = block.ast_node();
  const auto& void_type = m_.builtin_types.void_;
  auto ret_node = std::make_unique<implicit_return_node>(ast_node, void_type);

  auto manipulator = block_node_manipulator{ block };
  manipulator.append_expression(std::move(ret_node));
}

bool sema_builder_ast_visitor::is_last_node_return_node(
  const block_node& block) const
{
  const auto& nodes = block.nodes();
  if (nodes.empty()) {
    return false;
  }

  const auto& last_node = nodes.back();
  const auto last_node_is_return =
    dynamic_cast<const return_node*>(last_node.get()) != nullptr;

  return last_node_is_return;
}

bool sema_builder_ast_visitor::check_function_return_type(
  const lexer::token& return_kw, const expression_node& return_expression)
{
  if (m_.parsing_ctx.function_parsing_ctx.function == nullptr) {
    CMSL_UNREACHABLE("Return statement outside of function");
  }

  // If currently parsed function has 'auto' return type, return true.
  // Return types are going to be check later.
  const auto function_has_auto_return_type =
    m_.parsing_ctx.function_parsing_ctx.function->try_return_type() == nullptr;
  if (function_has_auto_return_type) {
    return true;
  }

  const auto& function_return_type =
    m_.parsing_ctx.function_parsing_ctx.function->return_type();

  variable_initialization_checker checker;
  const auto init_issues =
    checker.check(function_return_type, return_expression);

  if (init_issues.empty()) {
    return true;
  }

  for (const auto& issue : init_issues) {
    function_return_value_failed_initialization_errors_reporter reporter{
      m_.errors_observer, return_kw
    };
    std::visit(reporter, issue);
  }

  return false;
}

const sema_type*
sema_builder_ast_visitor::try_deduce_currently_parsed_function_return_type()
{
  if (m_.parsing_ctx.function_parsing_ctx.return_nodes.empty()) {
    return &m_.builtin_types.void_;
  }

  const auto& first_type =
    m_.parsing_ctx.function_parsing_ctx.return_nodes.front()->type();

  const auto all_expression_types_are_the_same = std::all_of(
    std::cbegin(m_.parsing_ctx.function_parsing_ctx.return_nodes),
    std::cend(m_.parsing_ctx.function_parsing_ctx.return_nodes),
    [&first_type](const auto& node) { return first_type == node->type(); });

  if (all_expression_types_are_the_same) {
    return &first_type;
  }

  raise_error(m_.parsing_ctx.function_parsing_ctx.function_node->name(),
              "incosistent deduction for auto return type");

  const auto source =
    m_.parsing_ctx.function_parsing_ctx.function_node->name().source();
  for (const auto& ret_node :
       m_.parsing_ctx.function_parsing_ctx.return_nodes) {
    raise_note(source, ret_node->ast_node().src_range(),
               "deduced as \'" + ret_node->type().name().to_string() + "\'");
  }

  return nullptr;
}

void sema_builder_ast_visitor::visit(
  const ast::variable_declaration_node& node)
{
  if (node.is_exported() && !is_export_allowed()) {
    raise_error(*node.export_(), "exporting local variables is not allowed");
    return;
  }

  const auto& type_representation = node.type();
  const auto should_deduce_type_from_initialization =
    type_representation.is_auto();

  const sema_type* type = nullptr;
  if (!should_deduce_type_from_initialization) {
    type = try_get_or_create_generic_type(m_.ctx, type_representation);
    if (!type) {
      // If type has not been found and that's a not an auto type declaration.
      return;
    }

    // If type has been found, check whether it is not void.
    const auto& void_type = m_.builtin_types.void_;
    auto void_name = void_type.fully_qualified_name();
    auto name = type->fully_qualified_name();
    if (*type == void_type) {
      raise_error(type_representation.source(),
                  type_representation.src_range(),
                  "Variable can not be of void type");
      return;
    }
  }

  if (should_deduce_type_from_initialization &&
      node.initialization() == nullptr) {
    raise_error(
      type_representation.source(), type_representation.src_range(),
      "Declaration of a variable with 'auto' type requires an initializer");
    return;
  }

  if (type && type->is_reference() && node.initialization() == nullptr) {
    raise_error(type_representation.source(), type_representation.src_range(),
                "Declaration of a reference variable requires an initializer");
    return;
  }

  std::unique_ptr<expression_node> initialization;
  if (auto init_node = node.initialization()) {
    initialization = visit_child_expr(*init_node);
    if (!initialization) {
      return;
    }

    if (!should_deduce_type_from_initialization) {
      variable_initialization_checker checker;
      const auto init_issues = checker.check(*type, *initialization);

      if (!init_issues.empty()) {
        for (const auto& issue : init_issues) {
          variable_failed_initialization_errors_reporter reporter{
            m_.errors_observer, *node.equal()
          };
          std::visit(reporter, issue);
        }
        return;
      }
    }

    if (should_deduce_type_from_initialization) {
      const auto& init_type = initialization->type();
      type =
        &(init_type.is_reference() ? init_type.referenced_type() : init_type);

      // If there was auto with a reference, we need to declare variable with a
      // reference type, e.g. auto& foo = bar();
      if (type_representation.is_reference()) {
        type = m_.ctx.find_reference_for(*type);
      }
    }

    initialization =
      convert_to_cast_node_if_need(*type, std::move(initialization));
  }

  const auto identifier_index = identifiers_index_provider::get_next();
  const auto is_exported = node.export_().has_value();
  m_.qualified_ctxs.ids.register_identifier(
    node.name(), { *type, identifier_index }, is_exported);
  m_result_node = std::make_unique<variable_declaration_node>(
    node, *type, node.name(), std::move(initialization), identifier_index);
}

void sema_builder_ast_visitor::visit(const ast::for_node& node)
{
  auto guard = m_.qualified_ctxs.local_ids_guard();

  std::unique_ptr<sema_node> init;
  if (node.init()) {
    init = visit_child(*node.init());
    if (!init) {
      return;
    }
  }

  std::unique_ptr<expression_node> condition;
  if (node.condition()) {
    condition = visit_child_expr(*node.condition());
    if (!condition) {
      return;
    }
  }

  std::unique_ptr<expression_node> iteration;
  if (node.iteration()) {
    iteration = visit_child_expr(*node.iteration());
    if (!iteration) {
      return;
    }
  }

  ++m_.parsing_ctx.loop_parsing_counter;
  auto body = visit_child_node<block_node>(node.body());
  --m_.parsing_ctx.loop_parsing_counter;
  if (!body) {
    return;
  }

  m_result_node =
    std::make_unique<for_node>(node, std::move(init), std::move(condition),
                               std::move(iteration), std::move(body));
}

void sema_builder_ast_visitor::visit(const ast::break_node& node)
{
  if (!is_inside_loop()) {
    raise_error(node.break_(), "break can be used only inside a loop");
    return;
  }

  m_result_node = std::make_unique<break_node>(node);
}

bool sema_builder_ast_visitor::is_inside_loop() const
{
  return m_.parsing_ctx.loop_parsing_counter > 0u;
}

void sema_builder_ast_visitor::visit(const ast::namespace_node& node)
{
  std::stack<qualified_contextes_refs::all_qualified_contextes_guard> guards;
  for (const auto& name_with_colon : node.names()) {
    auto guard = m_.qualified_ctxs.all_qualified_ctxs_guard(
      name_with_colon.name, /*exported=*/false);
    guards.emplace(std::move(guard));
  }

  namespace_node::nodes_t nodes;

  for (const auto& ast_node : node.nodes()) {
    auto child = visit_child(*ast_node);
    if (!child) {
      return;
    }

    nodes.emplace_back(std::move(child));
  }

  m_result_node =
    std::make_unique<namespace_node>(node, node.names(), std::move(nodes));

  while (!guards.empty()) {
    guards.pop();
  }
}

void sema_builder_ast_visitor::visit(const ast::ternary_operator_node& node)
{
  auto condition = visit_child_expr(node.condition());
  if (!condition) {
    return;
  }

  const auto& bool_type = m_.builtin_types.bool_;
  const auto& bool_ref_type = m_.builtin_types.bool_ref;
  if (condition->type() != bool_type && condition->type() != bool_ref_type) {
    const auto src_view = node.question().source();
    const auto range = source_range{ node.condition().src_range() };
    raise_error(src_view, range,
                "expected bool value, got " +
                  condition->type().name().to_string());
    return;
  }

  auto true_ = visit_child_expr(node.true_());
  if (!true_) {
    return;
  }

  auto false_ = visit_child_expr(node.false_());
  if (!false_) {
    return;
  }

  const auto& true_expr_type = true_->type();
  const auto& false_expr_type = false_->type();

  const auto different_types_error_raiser = [&node, &true_expr_type,
                                             &false_expr_type, this] {
    raise_error(node.question(),
                "incompatible operand types. " +
                  true_expr_type.name().to_string() + " and " +
                  false_expr_type.name().to_string());
  };

  if (true_expr_type != false_expr_type) {
    if (!true_expr_type.is_reference() && !false_expr_type.is_reference()) {
      different_types_error_raiser();
      return;
    }

    if (true_expr_type.is_reference()) {
      if (true_expr_type.referenced_type() != false_expr_type) {
        different_types_error_raiser();
        return;
      }
      true_ = convert_to_cast_node_if_need(true_expr_type.referenced_type(),
                                           std::move(true_));
    } else if (false_expr_type.is_reference()) {
      if (false_expr_type.referenced_type() != true_expr_type) {
        different_types_error_raiser();
        return;
      }
      false_ = convert_to_cast_node_if_need(false_expr_type.referenced_type(),
                                            std::move(false_));
    }
  }

  m_result_node = std::make_unique<ternary_operator_node>(
    node, std::move(condition), std::move(true_), std::move(false_));
}

void sema_builder_ast_visitor::visit(
  const ast::designated_initializers_node& node)
{
  const auto name = ast::type_representation{ ast::qualified_name{
    make_token(lexer::token_type::identifier, "designated_initializer") } };
  auto type_factory = m_.factories.type_factory(m_.qualified_ctxs.types);
  const auto& designated_type = type_factory.create_designated_initializer(
    m_.ctx, name, /*exported=*/false);

  designated_initializers_node::initializers_t initializers;

  for (const auto& init : node.initializers()) {
    designated_initializers_node::initializer initializer;
    initializer.name = init.name;
    initializer.init = visit_child_expr(*init.initialization);
    if (!initializer.init) {
      return;
    }
    initializers.emplace_back(std::move(initializer));
  }

  m_result_node = std::make_unique<designated_initializers_node>(
    node, designated_type, std::move(initializers));
}

bool sema_builder_ast_visitor::raise_error_if_function_redefined(
  const function_signature& signature)
{
  if (const auto found_same_function =
        m_.qualified_ctxs.functions.find_in_current_scope(signature)) {
    raise_error(signature.name, "function redefinition");
    raise_note(found_same_function->signature().name,
               "previous definition is here");
    return true;
  }

  return false;
}

function_lookup_result_t sema_builder_ast_visitor::find_functions(
  const std::vector<ast::name_with_coloncolon>& names)
{
  auto result = m_.qualified_ctxs.functions.find(names);
  if (const auto found_type = m_.qualified_ctxs.types.find(names)) {
    // If no function was found, we still need to add constructor functions.
    if (result.empty()) {
      result.emplace_back();
    }

    const auto& type_ctx = found_type->ty.context();
    const auto ctors = type_ctx.find_function_in_this_scope(names.back().name);
    result.front().insert(std::end(result.front()), std::cbegin(ctors),
                          std::cend(ctors));
  }

  return result;
}

function_lookup_result_t
sema_builder_ast_visitor::find_generict_type_constructor_functions(
  const ast::type_representation& name)
{
  const auto ty = try_get_or_create_generic_type(m_.ctx, name);
  if (!ty) {
    return {};
  }
  return { ty->find_member_function(name.primary_name_token()) };
}

void sema_builder_ast_visitor::visit(const ast::unary_operator& node)
{
  auto expression = visit_child_expr(node.expression());
  if (!expression) {
    return;
  }

  const auto found_functions =
    expression->type().context().find_function_in_this_scope(node.operator_());
  overload_resolution resolution{ m_.errors_observer, node.operator_() };
  const auto chosen_function = resolution.choose(found_functions);
  if (!chosen_function) {
    return;
  }

  m_result_node = std::make_unique<unary_operator_node>(
    node, node.operator_(), std::move(expression), *chosen_function);
}

void sema_builder_ast_visitor::visit(const ast::enum_node& node)
{
  const auto& name = node.name();
  const auto name_representation =
    ast::type_representation{ ast::qualified_name{ name } };

  if (const auto found = m_.qualified_ctxs.types.find_in_current_scope(name);
      found != nullptr) {
    raise_error(name, "type redefinition");
    raise_note(found->name().primary_name_token(), "previously defined here");
    return;
  }

  // Create context and type
  const auto& enum_type = create_enum_type(node);
  const auto is_exported = node.export_().has_value();
  auto functions_guard =
    m_.qualified_ctxs.functions_ctx_guard(name, is_exported);

  // Create enum constants
  {
    auto guard = m_.qualified_ctxs.enums_ctx_guard(name, is_exported);

    std::unordered_set<lexer::token> seen_enumerators;
    unsigned value{};

    for (const auto& enumerator : node.enumerators()) {
      const auto found = seen_enumerators.find(enumerator);
      if (found != std::cend(seen_enumerators)) {
        raise_error(enumerator, "enumerator redefinition");
        raise_note(*found, "previously defined here");
        return;
      }

      seen_enumerators.insert(enumerator);

      const auto enum_value_index = identifiers_index_provider::get_next();
      const auto enum_value = value++;
      m_.qualified_ctxs.enums.register_identifier(
        enumerator, enum_value_info{ enum_type, enum_value, enum_value_index },
        is_exported);
    }
  }

  m_result_node = std::make_unique<enum_node>(node, name, node.enumerators());
}

const sema_type& sema_builder_ast_visitor::create_enum_type(
  const ast::enum_node& node)
{
  enum_creator creator{ m_.factories, m_.qualified_ctxs.types, m_.ctx,
                        m_.builtin_types };
  return creator.create(node.name(), node.enumerators());
}

void sema_builder_ast_visitor::add_user_type_default_methods(
  const sema_type& class_ty, sema_context& class_ctx)
{
  const auto& class_ref_ty = *class_ctx.find_reference_for(class_ty);

  const auto dummy_param_name_token =
    lexer::make_token(lexer::token_type::identifier, "");

  auto functions = {
    type_builder::builtin_function_info{
      // operator=(val)
      class_ref_ty,
      function_signature{
        lexer::make_token(lexer::token_type::identifier, "="),
        { parameter_declaration{ class_ref_ty, dummy_param_name_token,
                                 identifiers_index_provider::get_next() } } },
      builtin_function_kind::user_type_operator_equal },
  };

  for (auto& f : functions) {
    const auto& function = m_.factories.function_factory().create_builtin(
      class_ctx, f.return_type, std::move(f.signature), f.kind);
    class_ctx.add_function(function);
  }
}

void sema_builder_ast_visitor::visit(const ast::import_node& node)
{
  const auto file_path_view = node.file_path().str();

  // -2 to remove "".
  const auto path = cmsl::string_view{ file_path_view.data() + 1u,
                                       file_path_view.size() - 2u };
  const auto imported = m_.imports_handler.handle_import(path);
  if (!imported) {
    raise_error(node.import_(), "Importing failed");
    return;
  }

  const auto enums_succeed = m_.qualified_ctxs.enums.merge_imported_stuff(
    *imported->enums, m_.errors_observer);
  const auto functions_succeed =
    m_.qualified_ctxs.functions.merge_imported_stuff(*imported->functions,
                                                     m_.errors_observer);
  const auto ids_succeed = m_.qualified_ctxs.ids.merge_imported_stuff(
    *imported->ids, m_.errors_observer);
  const auto types_succeed = m_.qualified_ctxs.types.merge_imported_stuff(
    *imported->types, m_.errors_observer);

  const auto all_succeed =
    enums_succeed && functions_succeed && ids_succeed && types_succeed;
  if (!all_succeed) {
    raise_error(node.import_(), "Importing failed");
    return;
  }

  m_result_node = std::make_unique<import_node>(node, node.file_path());
}

bool sema_builder_ast_visitor::is_export_allowed() const
{
  return m_.qualified_ctxs.ids.is_in_global_ctx();
}
}
