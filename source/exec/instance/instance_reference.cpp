#include "exec/instance/instance_reference.hpp"
#include "exec/execution_context.hpp"
#include "lexer/token.hpp"

namespace cmsl::exec::inst {
instance_reference::instance_reference(unsigned index, execution_context& ctx)
  : instance_reference{ *ctx.get_variable(index) }
{
}

instance_reference::instance_reference(
  instance& referenced_instance, const sema::sema_type* referenced_base_type)
  : m_instance{ referenced_instance }
  , m_referenced_base_type{ referenced_base_type }
{
}

instance_value_variant instance_reference::value() const
{
  return m_instance.value();
}

instance_value_accessor instance_reference::value_accessor()
{
  return m_instance.value_accessor();
}

const instance_value_variant& instance_reference::value_cref() const
{
  return m_instance.value_cref();
}

void instance_reference::assign(instance_value_variant val)
{
  m_instance.assign(std::move(val));
}

void instance_reference::assign_member(unsigned index,
                                       std::unique_ptr<instance> val)
{
  m_instance.assign_member(index, std::move(val));
}

std::unique_ptr<instance> instance_reference::copy() const
{
  return std::make_unique<instance_reference>(m_instance);
}

instance* instance_reference::find_member(unsigned index)
{
  return m_instance.find_member(index);
}

const instance* instance_reference::find_cmember(unsigned index) const
{
  return m_instance.find_cmember(index);
}

sema::single_scope_function_lookup_result_t instance_reference::find_function(
  lexer::token name) const
{
  return m_instance.find_function(name);
}

const sema::sema_type& instance_reference::type() const
{
  return m_referenced_base_type ? *m_referenced_base_type : m_instance.type();
}

void instance_reference::assign(std::unique_ptr<instance> val)
{
  m_instance.assign(std::move(val));
}
}
