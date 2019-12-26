#include "exec/instance/complex_unnamed_instance.hpp"

#include "common/assert.hpp"
#include "complex_unnamed_instance.hpp"
#include "exec/instance/instance_factory.hpp"
#include "sema/sema_type.hpp"

#include <algorithm>
#include <iterator>

namespace cmsl::exec::inst {
complex_unnamed_instance::complex_unnamed_instance(const sema::sema_type& type)
  : m_sema_type{ type }
  , m_members{ get_init_data() }
{
}

complex_unnamed_instance::complex_unnamed_instance(const sema::sema_type& type,
                                                   instance_members_t members)
  : m_sema_type{ type }
  , m_members{ std::move(members) }
{
}

complex_unnamed_instance::instance_members_t
complex_unnamed_instance::get_init_data() const
{
  return create_init_members();
}

complex_unnamed_instance::instance_members_t
complex_unnamed_instance::get_init_data(instance_members_t members) const
{
  return members;
}

instance_value_variant complex_unnamed_instance::value() const
{
  CMSL_UNREACHABLE("Getting value of a complex type");
  return instance_value_variant{};
}

instance_value_accessor complex_unnamed_instance::value_accessor()
{
  CMSL_UNREACHABLE("Getting value of a complex type");
  static instance_value_variant dummy;
  return instance_value_accessor{ type(), dummy };
}

const instance_value_variant& complex_unnamed_instance::value_cref() const
{
  CMSL_UNREACHABLE("Getting value of a complex type");
  static instance_value_variant dummy;
  return dummy;
}

void complex_unnamed_instance::assign(instance_value_variant)
{
  CMSL_UNREACHABLE("Assigning value to a complex type");
}

void complex_unnamed_instance::assign_member(unsigned index,
                                             std::unique_ptr<instance> val)
{
  auto found = m_members.find(index);
  if (found == std::cend(m_members)) {
    CMSL_UNREACHABLE(
      "Assinging a member that does not belong to type of the instance");
    return;
  }

  found->second = std::move(val);
}

instance* complex_unnamed_instance::find_member(unsigned index)
{
  auto found = m_members.find(index);
  return found != std::end(m_members) ? found->second.get() : nullptr;
}

const instance* complex_unnamed_instance::find_cmember(unsigned index) const
{
  auto found = m_members.find(index);
  return found != std::end(m_members) ? found->second.get() : nullptr;
}

bool complex_unnamed_instance::is_fundamental() const
{
  return m_kind == kind::builtin;
}

std::unique_ptr<instance> complex_unnamed_instance::copy() const
{
  return std::make_unique<complex_unnamed_instance>(m_sema_type,
                                                    copy_members());
}

complex_unnamed_instance::instance_members_t
complex_unnamed_instance::copy_members() const
{
  instance_members_t m;

  std::transform(std::cbegin(m_members), std::cend(m_members),
                 std::inserter(m, m.end()), [](const auto& member_pair) {
                   return std::make_pair(member_pair.first,
                                         member_pair.second->copy());
                 });

  return m;
}

sema::single_scope_function_lookup_result_t
complex_unnamed_instance::find_function(lexer::token name) const
{
  return m_sema_type.find_member_function(name);
}

const sema::sema_type& complex_unnamed_instance::type() const
{
  return m_sema_type;
}

complex_unnamed_instance::instance_members_t
complex_unnamed_instance::create_init_members() const
{
  return create_members_of_type(m_sema_type);
}

void complex_unnamed_instance::assign(std::unique_ptr<instance> val)
{
  // If we're here, we know that the val is an object of
  // complex_unnamed_instance type, so the static_cast is safe.
  auto casted_instance = static_cast<complex_unnamed_instance*>(val.get());
  m_members = std::move(casted_instance->m_members);
}

complex_unnamed_instance::instance_members_t
complex_unnamed_instance::create_members_of_type(
  const sema::sema_type& ty) const
{
  instance_members_t members;

  if (const auto derived_type = ty.derived_type()) {
    members = create_members_of_type(*derived_type);
  }

  const auto& member_declarations = ty.members();

  std::transform(
    std::cbegin(member_declarations), std::cend(member_declarations),
    std::inserter(members, std::end(members)), [](const auto& member_decl) {
      auto member_inst = instance_factory2{}.create(member_decl.ty);
      return std::make_pair(member_decl.index, std::move(member_inst));
    });

  return members;
}
}
