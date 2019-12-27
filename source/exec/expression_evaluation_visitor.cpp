#include "exec/expression_evaluation_visitor.hpp"

namespace cmsl::exec {
expression_evaluation_visitor::expression_evaluation_visitor(
  expression_evaluation_context& ctx)
  : m_ctx{ ctx }
{
}
void expression_evaluation_visitor::visit(const sema::bool_value_node& node)
{
  result = m_ctx.instances.create(node.value());
}

void expression_evaluation_visitor::visit(const sema::int_value_node& node)
{
  result = m_ctx.instances.create(node.value());
}

void expression_evaluation_visitor::visit(const sema::double_value_node& node)
{
  result = m_ctx.instances.create(node.value());
}

void expression_evaluation_visitor::visit(const sema::string_value_node& node)
{
  result = m_ctx.instances.create(std::string{ node.value() });
}

void expression_evaluation_visitor::visit(
  const sema::enum_constant_access_node& node)
{
  result = m_ctx.instances.create(node.type(),
                                  inst::enum_constant_value{ node.value() });
}

void expression_evaluation_visitor::visit(const sema::id_node& node)
{
  result = m_ctx.ids_context.lookup_identifier(node.index());
}

void expression_evaluation_visitor::visit(
  const sema::binary_operator_node& node)
{
  auto lhs_result = evaluate_child(node.lhs());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  auto rhs_result = evaluate_child(node.rhs());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& operator_function = node.operator_function();

  // Todo: use small vector.
  std::vector<inst::instance*> params;
  params.emplace_back(rhs_result);
  auto result_instance = m_ctx.caller.call_member(
    *lhs_result, operator_function, std::move(params), m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(const sema::function_call_node& node)
{
  auto evaluated_params =
    evaluate_call_parameters(node.function(), node.param_expressions());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& function = node.function();
  auto result_instance =
    m_ctx.caller.call(function, evaluated_params, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(
  const sema::add_subdirectory_node& node)
{

  auto evaluated_params =
    evaluate_call_parameters(node.function(), node.param_expressions());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto dir = std::string{ node.dir_name().value() };

  //    m_ctx.cmake_facade.go_into_subdirectory(dir);

  m_ctx.cmake_facade.prepare_for_add_subdirectory_with_cmakesl_script(dir);

  const auto& function = node.function();
  auto result_instance =
    m_ctx.caller.call(function, evaluated_params, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    m_ctx.cmake_facade.go_directory_up();
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));

  m_ctx.cmake_facade.finalize_after_add_subdirectory_with_cmakesl_script();
}

void expression_evaluation_visitor::visit(
  const sema::add_subdirectory_with_declarative_script_node& node)
{
  const auto dir = std::string{ node.dir_name().value() };
  m_ctx.cmake_facade.prepare_for_add_subdirectory_with_cmakesl_script(dir);

  const auto& function = node.component_creation_function();
  auto result_instance = m_ctx.caller.call(function, {}, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    m_ctx.cmake_facade.go_directory_up();
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));

  m_ctx.cmake_facade.finalize_after_add_subdirectory_with_cmakesl_script();
}

void expression_evaluation_visitor::visit(
  const sema::implicit_member_function_call_node& node)
{
  auto evaluated_params =
    evaluate_call_parameters(node.function(), node.param_expressions());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& function = node.function();
  auto class_instance = m_ctx.ids_context.get_class_instance();
  auto result_instance = m_ctx.caller.call_member(
    *class_instance, function, evaluated_params, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(
  const sema::constructor_call_node& node)
{
  auto evaluated_params =
    evaluate_call_parameters(node.function(), node.param_expressions());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& function = node.function();
  auto class_instance = m_ctx.instances.create(node.type());
  auto result_instance = m_ctx.caller.call_member(
    *class_instance, function, evaluated_params, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(
  const sema::member_function_call_node& node)
{
  auto lhs_result = evaluate_child(node.lhs());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  auto evaluated_params =
    evaluate_call_parameters(node.function(), node.param_expressions());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& function = node.function();
  auto result_instance = m_ctx.caller.call_member(
    *lhs_result, function, evaluated_params, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(
  const sema::class_member_access_node& node)
{
  auto lhs = evaluate_child(node.lhs());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }
  result = lhs->find_member(node.member_index());
}

void expression_evaluation_visitor::visit(const sema::return_node& node)
{
  result = evaluate_child(node.expression());
}

void expression_evaluation_visitor::visit(
  const sema::cast_to_reference_node& node)
{
  const auto evaluated = evaluate_child(node.expression());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }
  result = m_ctx.instances.create_reference(*evaluated);
}

void expression_evaluation_visitor::visit(
  const sema::cast_to_reference_to_base_node& node)
{
  const auto evaluated = evaluate_child(node.expression());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }
  result = m_ctx.instances.create_reference_to_base(*evaluated, node.type());
}

void expression_evaluation_visitor::visit(const sema::cast_to_value_node& node)
{
  const auto evaluated = evaluate_child(node.expression());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }
  const auto& value_type = evaluated->type();

  if (!value_type.is_complex() && value_type.is_builtin()) {
    result = m_ctx.instances.create(value_type, evaluated->value());
    return;
  }

  auto created_value = m_ctx.instances.create(value_type);
  for (const auto& member_info : value_type.members()) {
    auto copied = evaluated->find_cmember(member_info.index)->copy();
    created_value->assign_member(member_info.index, std::move(copied));
  }

  result = created_value;
}

void expression_evaluation_visitor::visit(
  const sema::cast_to_base_value_node& node)
{
  const auto evaluated = evaluate_child(node.expression());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& base_type = node.type();
  auto base_instance = m_ctx.instances.create(base_type);

  for (const auto& member_info : base_type.members()) {
    auto copied_member = evaluated->find_cmember(member_info.index)->copy();
    base_instance->assign_member(member_info.index, std::move(copied_member));
  }

  result = base_instance;
}

void expression_evaluation_visitor::visit(
  const sema::initializer_list_node& node)
{
  auto list_instance = m_ctx.instances.create(node.type());
  auto accessor = list_instance->value_accessor();
  auto& list = accessor.access().get_list_ref();

  for (const auto& value_expression : node.values()) {
    const auto evaluated = evaluate_child(*value_expression);
    if (m_ctx.cmake_facade.did_fatal_error_occure()) {
      return;
    }
    auto owned_value = m_ctx.instances.gather_ownership(evaluated);
    list.push_back(std::move(owned_value));
  }

  result = list_instance;
}

void expression_evaluation_visitor::visit(
  const sema::ternary_operator_node& node)
{
  const auto condition = evaluate_child(node.condition());
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  const auto& condition_result_value = condition->value_cref();

  if (condition_result_value.get_bool()) {
    result = evaluate_child(node.true_());
  } else {
    result = evaluate_child(node.false_());
  }
}

void expression_evaluation_visitor::visit(
  const sema::designated_initializers_node& node)
{
  const auto& expected_type = m_ctx.expected_types.top().get();
  auto instance = m_ctx.instances.create(expected_type);

  for (const auto& initializer : node.initializers()) {
    const auto member_info = expected_type.find_member(initializer.name.str());
    auto guard = set_expected_type(member_info->ty);

    auto initialization_value = evaluate_child(*initializer.init);
    if (m_ctx.cmake_facade.did_fatal_error_occure()) {
      return;
    }

    // It could probably be gathered from the instances.
    instance->assign_member(member_info->index, initialization_value->copy());
  }

  result = instance;
}

void expression_evaluation_visitor::visit(
  const sema::unary_operator_node& node)
{
  auto lhs_result = evaluate_child(node.expression());
  const auto& operator_function = node.function();

  // Todo: use small vector.
  std::vector<inst::instance*> params;
  auto result_instance = m_ctx.caller.call_member(
    *lhs_result, operator_function, std::move(params), m_ctx.instances);
  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

void expression_evaluation_visitor::visit(
  const sema::add_declarative_file_node& node)
{
  const auto& function = node.function();
  auto result_instance = m_ctx.caller.call(function, {}, m_ctx.instances);
  if (m_ctx.cmake_facade.did_fatal_error_occure()) {
    return;
  }

  result = result_instance.get();
  m_ctx.instances.store(std::move(result_instance));
}

template <typename T>
inst::instance* expression_evaluation_visitor::evaluate_child(const T& child)
{
  auto c = clone();
  child.visit(c);
  return c.result;
}

std::vector<inst::instance*>
expression_evaluation_visitor::evaluate_call_parameters(
  const sema::sema_function& function,
  const sema::call_node::param_expressions_t& params)
{
  std::vector<inst::instance*> evaluated_params;

  for (auto i = 0u; i < params.size(); ++i) {
    const auto& expected_type = function.signature().params[i].ty;
    auto guard = set_expected_type(expected_type);
    auto param = evaluate_child(*params[i]);

    if (m_ctx.cmake_facade.did_fatal_error_occure()) {
      return {};
    }

    evaluated_params.emplace_back(param);
  }

  return evaluated_params;
}

expression_evaluation_visitor expression_evaluation_visitor::clone()
{
  return expression_evaluation_visitor{ m_ctx };
}

std::string expression_evaluation_visitor::operator_to_str(lexer::token op)
{
  switch (op.get_type()) {
    case lexer::token_type::equal:
    case lexer::token_type::equalequal:
    case lexer::token_type::minus:
    case lexer::token_type::minusminus:
    case lexer::token_type::minusequal:
    case lexer::token_type::plus:
    case lexer::token_type::plusplus:
    case lexer::token_type::plusequal:
    case lexer::token_type::amp:
    case lexer::token_type::ampamp:
    case lexer::token_type::ampequal:
    case lexer::token_type::pipe:
    case lexer::token_type::pipepipe:
    case lexer::token_type::pipeequal:
    case lexer::token_type::slash:
    case lexer::token_type::slashequal:
    case lexer::token_type::star:
    case lexer::token_type::starequal:
    case lexer::token_type::percent:
    case lexer::token_type::percentequal:
    case lexer::token_type::exclaimequal:
    case lexer::token_type::xor_:
    case lexer::token_type::xorequal:
    case lexer::token_type::less:
    case lexer::token_type::lessequal:
    case lexer::token_type::greater:
    case lexer::token_type::greaterequal: {
      return std::string{ op.str() };
    }

    default:
      break;
  }

  CMSL_UNREACHABLE("Unknown operator token");
  return "";
}

expression_evaluation_visitor::expected_types_guard
expression_evaluation_visitor::set_expected_type(const sema::sema_type& ty)
{
  return expected_types_guard{ m_ctx.expected_types, ty };
}
}
