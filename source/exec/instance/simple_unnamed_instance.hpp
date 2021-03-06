#pragma once

#include "exec/instance/instance.hpp"

namespace cmsl::exec::inst {
class simple_unnamed_instance : public instance
{
public:
  explicit simple_unnamed_instance(const sema::sema_type& type);

  explicit simple_unnamed_instance(const sema::sema_type& type,
                                   instance_value_variant&& value);

  virtual ~simple_unnamed_instance() = default;

  instance_value_variant value() const override;
  instance_value_accessor value_accessor() override;
  const instance_value_variant& value_cref() const override;

  void assign(instance_value_variant val) override;
  void assign(std::unique_ptr<instance> val) override;
  void assign_member(unsigned index, std::unique_ptr<instance> val) override;

  std::unique_ptr<instance> copy() const override;

  instance* find_member(unsigned index) override;
  const instance* find_cmember(unsigned index) const override;
  sema::single_scope_function_lookup_result_t find_function(
    lexer::token name) const override;

  bool is_fundamental() const;

  const sema::sema_type& type() const override;

private:
  instance_value_variant create_init_data() const;
  instance_value_variant create_init_data(instance_value_variant val) const;

private:
  kind m_kind;
  const sema::sema_type& m_sema_type;
  instance_value_variant m_data;
};
}
