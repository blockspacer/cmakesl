#pragma once

#include "ast/parameter_declaration.hpp"
#include "ast/parse_errors_observer.hpp"
#include "ast/parser_utils.hpp"
#include "ast/qualified_name.hpp"

#include <memory>

namespace cmsl {
class strings_container;

namespace errors {
class errors_observer;
}

namespace ast {
class ast_node;
class conditional_node;
class block_node;
class type_representation;
class variable_declaration_node;
class break_node;
struct name_with_coloncolon;
class namespace_node;
class id_node;
class ternary_operator_node;
class designated_initializers_node;
class enum_node;
class import_node;
class string_value_node;

class parser : public parser_utils
{
public:
  parser(errors::errors_observer& err_observer,
         strings_container& strings_container, cmsl::source_view source,
         const token_container_t& tokens);

  std::unique_ptr<ast_node> parse_translation_unit();
  std::unique_ptr<ast_node> parse_standalone_variable_declaration(
    std::optional<token_t> export_kw);
  std::unique_ptr<variable_declaration_node> parse_variable_declaration(
    std::optional<token_t> export_kw = {});
  std::unique_ptr<ast_node> parse_function(
    std::optional<token_t> export_kw = {});
  std::unique_ptr<ast_node> parse_class(std::optional<token_t> export_kw);
  std::unique_ptr<ast_node> parse_factor();
  std::unique_ptr<ast_node> parse_initializer_list();

  // parse_expr() doesn't eat terminating semicolon. Callee has to handle that
  // if there is a need.
  std::unique_ptr<ast_node> parse_expr();
  std::unique_ptr<ast_node> parse_if_else_node();
  std::unique_ptr<ast_node> parse_while_node();
  std::unique_ptr<ast_node> parse_for_node();
  std::unique_ptr<ast_node> parse_return_node();
  std::optional<type_representation> parse_type();
  std::unique_ptr<block_node> parse_block();
  std::unique_ptr<break_node> parse_break();
  std::unique_ptr<namespace_node> parse_namespace();
  std::unique_ptr<id_node> parse_identifier();

  // Called when condition is already parsed, co current() is question.
  std::unique_ptr<ternary_operator_node> parse_rest_of_ternary_operator(
    std::unique_ptr<ast_node> condition, token_t question);

  std::unique_ptr<ast_node> parse_expression_starting_from_brace();
  std::unique_ptr<designated_initializers_node>
  parse_designated_initializers();

  std::unique_ptr<ast_node> parse_unary_operator();
  std::unique_ptr<enum_node> parse_enum(std::optional<token_t> export_kw);
  std::unique_ptr<import_node> parse_import();

  std::unique_ptr<string_value_node> parse_string(const token_t& token);

private:
  struct function_call_values
  {
    token_t open_paren;
    std::vector<std::unique_ptr<ast_node>> params;
    token_t close_paren;
  };

  struct member_function_call_values : public function_call_values
  {
    token_t name;
  };

  struct standalone_function_call_values : public function_call_values
  {
    std::vector<name_with_coloncolon> names;
  };

  std::optional<token_t> eat_function_call_name();

  bool current_is_class_member_access() const;
  bool current_is_function_call() const;
  bool current_is_fundamental_value() const;
  bool current_is_possibly_qualified_name() const;
  bool current_is_generic_type_constructor_call() const;
  bool function_declaration_starts() const;
  bool declaration_starts() const;

  static constexpr auto k_min_precedence{ 2u };
  static constexpr auto k_max_precedence{ 16u };
  std::unique_ptr<ast_node> parse_operator(
    unsigned precedence = k_max_precedence);

  std::unique_ptr<ast_node> fundamental_value();
  std::unique_ptr<ast_node> function_call();
  std::unique_ptr<ast_node> parse_generic_type_constructor_call();

  std::optional<std::vector<std::unique_ptr<ast_node>>>
  comma_separated_expression_list(token_type_t valid_end_of_list_token);

  struct call_param_list_values
  {
    token_t open_paren;
    std::vector<std::unique_ptr<ast_node>> params;
    token_t close_paren;
  };
  std::optional<call_param_list_values> parameter_list();
  std::optional<member_function_call_values> get_member_function_call_values();
  std::optional<standalone_function_call_values>
  get_standalone_function_call_values();

  bool prepare_for_next_parameter_declaration();
  std::optional<param_declaration> get_param_declaration();

  struct param_list_values
  {
    token_t open_paren;
    std::vector<param_declaration> params;
    token_t close_paren;
  };
  std::optional<param_list_values> param_declarations();

  std::unique_ptr<conditional_node> get_conditional_node();

  std::unique_ptr<ast_node> constructor(token_t class_name);

  std::optional<std::unique_ptr<ast_node>> parse_for_init();
  std::optional<std::unique_ptr<ast_node>> parse_for_condition();
  std::optional<std::unique_ptr<ast_node>> parse_for_iteration();

  template <typename StopCondition>
  std::optional<std::vector<std::unique_ptr<ast_node>>> parse_top_level_nodes(
    StopCondition&& stop_condition);

  std::vector<name_with_coloncolon> parse_namespace_declaration_names();

  std::string build_string_value(const std::vector<token_t>& tokens) const;

private:
  parse_errors_reporter m_errors_reporter;
  strings_container& m_strings_container;
};
}
}
