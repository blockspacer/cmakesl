#pragma once

#include "ast/type_representation.hpp"

#include "sema/function_signature.hpp"
#include "sema_context.hpp"

namespace cmsl::sema {
struct member_info;
class sema_context_impl;
class sema_context;
class factories_provider;
class types_context;
enum class builtin_function_kind;

class type_builder
{
public:
  struct builtin_function_info
  {
    const sema_type& return_type;
    function_signature signature;
    builtin_function_kind kind;
  };

  explicit type_builder(factories_provider& factories,
                        types_context& types_ctx, sema_context& parent_ctx,
                        ast::type_representation name,
                        sema_context::context_type ctx_type =
                          sema::sema_context::context_type::class_);

  type_builder& with_member(const member_info& member);
  type_builder& with_user_function(const sema_type& return_type,
                                   function_signature s);
  type_builder& with_builtin_function(const sema_type& return_type,
                                      function_signature s,
                                      builtin_function_kind kind);

  type_builder& with_exported(bool exported = true);

  type_builder& with_derived_type(const sema_type& ty);

  template <typename Functions>
  type_builder& with_builtin_functions(Functions&& functions)
  {
    for (const auto& fun : functions) {
      with_builtin_function(fun.return_type, fun.signature, fun.kind);
    }

    return *this;
  }

  const sema_type& build_and_register_in_context(
    sema_type::flags_t flags = {});
  const sema_type& build_enum_and_register_in_context(
    std::vector<lexer::token> enumerators,
    sema_type::flags_t additional_flags = {});
  const sema_type& build_builtin_and_register_in_context();
  const sema_type& build_homogeneous_generic_and_register_in_context(
    const sema_type& value_type);

  const sema_context& context();

  struct built_type_info
  {
    const sema_type& ty;
    const sema_type& reference;
  };
  built_type_info built_type() const;

private:
  factories_provider& m_factories;
  types_context& m_types_ctx;
  sema_context& m_current_ctx;
  sema_context& m_type_ctx;
  ast::type_representation m_name;
  std::vector<member_info> m_members;
  const sema_type* m_derived_type{ nullptr };
  const sema_type* m_built_type;
  const sema_type* m_built_type_ref;
  bool m_exported{ false };
};
}
