#pragma once

namespace cmsl::sema {
class add_declarative_file_node;
class add_subdirectory_node;
class add_subdirectory_with_declarative_script_node;
class add_subdirectory_with_old_script_node;
class binary_operator_node;
class block_node;
class bool_value_node;
class break_node;
class cast_to_reference_node;
class cast_to_reference_to_base_node;
class cast_to_base_value_node;
class cast_to_value_node;
class class_member_access_node;
class class_node;
class conditional_node;
class constructor_call_node;
class designated_initializers_node;
class double_value_node;
class enum_constant_access_node;
class enum_node;
class for_node;
class function_call_node;
class function_node;
class id_node;
class if_else_node;
class implicit_member_function_call_node;
class implicit_return_node;
class import_node;
class initializer_list_node;
class int_value_node;
class member_function_call_node;
class namespace_node;
class return_node;
class sema_node;
class string_value_node;
class ternary_operator_node;
class translation_unit_node;
class unary_operator_node;
class variable_declaration_node;
class while_node;

class sema_node_visitor
{
public:
  virtual ~sema_node_visitor() = default;
  virtual void visit(const initializer_list_node& node) = 0;

  virtual void visit(const add_declarative_file_node& node) = 0;
  virtual void visit(const add_subdirectory_node& node) = 0;
  virtual void visit(
    const add_subdirectory_with_declarative_script_node& node) = 0;
  virtual void visit(const add_subdirectory_with_old_script_node& node) = 0;
  virtual void visit(const binary_operator_node& node) = 0;
  virtual void visit(const block_node& node) = 0;
  virtual void visit(const bool_value_node& node) = 0;
  virtual void visit(const break_node& node) = 0;
  virtual void visit(const cast_to_reference_node& node) = 0;
  virtual void visit(const cast_to_value_node& node) = 0;
  virtual void visit(const cast_to_reference_to_base_node& node) = 0;
  virtual void visit(const cast_to_base_value_node& node) = 0;
  virtual void visit(const class_member_access_node& node) = 0;
  virtual void visit(const class_node& node) = 0;
  virtual void visit(const conditional_node& node) = 0;
  virtual void visit(const constructor_call_node& node) = 0;
  virtual void visit(const designated_initializers_node& node) = 0;
  virtual void visit(const double_value_node& node) = 0;
  virtual void visit(const enum_constant_access_node& node) = 0;
  virtual void visit(const enum_node& node) = 0;
  virtual void visit(const for_node& node) = 0;
  virtual void visit(const function_call_node& node) = 0;
  virtual void visit(const function_node& node) = 0;
  virtual void visit(const id_node& node) = 0;
  virtual void visit(const if_else_node& node) = 0;
  virtual void visit(const implicit_member_function_call_node& node) = 0;
  virtual void visit(const implicit_return_node& node) = 0;
  virtual void visit(const import_node& node) = 0;
  virtual void visit(const int_value_node& node) = 0;
  virtual void visit(const member_function_call_node& node) = 0;
  virtual void visit(const namespace_node& node) = 0;
  virtual void visit(const return_node& node) = 0;
  virtual void visit(const string_value_node& node) = 0;
  virtual void visit(const ternary_operator_node& node) = 0;
  virtual void visit(const translation_unit_node& node) = 0;
  virtual void visit(const unary_operator_node& node) = 0;
  virtual void visit(const variable_declaration_node& node) = 0;
  virtual void visit(const while_node& node) = 0;
};

class empty_sema_node_visitor : public sema_node_visitor
{
public:
  virtual ~empty_sema_node_visitor() = default;

  virtual void visit(const add_declarative_file_node&) override {}
  virtual void visit(const add_subdirectory_node&) override {}
  virtual void visit(const add_subdirectory_with_old_script_node&) override {}
  virtual void visit(
    const add_subdirectory_with_declarative_script_node& node) override
  {
  }
  virtual void visit(const binary_operator_node&) override {}
  virtual void visit(const block_node&) override {}
  virtual void visit(const bool_value_node&) override {}
  virtual void visit(const break_node&) override {}
  virtual void visit(const cast_to_reference_to_base_node&) override {}
  virtual void visit(const cast_to_base_value_node&) override {}
  virtual void visit(const cast_to_reference_node&) override {}
  virtual void visit(const cast_to_value_node&) override {}
  virtual void visit(const class_member_access_node&) override {}
  virtual void visit(const class_node&) override {}
  virtual void visit(const conditional_node&) override {}
  virtual void visit(const constructor_call_node&) override {}
  virtual void visit(const designated_initializers_node&) override {}
  virtual void visit(const double_value_node&) override {}
  virtual void visit(const enum_constant_access_node&) override {}
  virtual void visit(const enum_node&) override {}
  virtual void visit(const for_node&) override {}
  virtual void visit(const function_call_node&) override {}
  virtual void visit(const function_node&) override {}
  virtual void visit(const id_node&) override {}
  virtual void visit(const if_else_node&) override {}
  virtual void visit(const implicit_member_function_call_node&) override {}
  virtual void visit(const implicit_return_node&) override {}
  virtual void visit(const import_node&) override {}
  virtual void visit(const initializer_list_node&) override {}
  virtual void visit(const int_value_node&) override {}
  virtual void visit(const member_function_call_node&) override {}
  virtual void visit(const namespace_node&) override {}
  virtual void visit(const return_node&) override {}
  virtual void visit(const string_value_node&) override {}
  virtual void visit(const ternary_operator_node&) override {}
  virtual void visit(const translation_unit_node&) override {}
  virtual void visit(const unary_operator_node&) override {}
  virtual void visit(const variable_declaration_node&) override {}
  virtual void visit(const while_node&) override {}
};
}
