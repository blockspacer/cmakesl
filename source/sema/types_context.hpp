#pragma once

#include "sema/qualified_entries_finder.hpp"

#include <ast/type_representation.hpp>
#include <memory>

namespace cmsl {
namespace errors {
class errors_observer;
}

namespace sema {
class qualified_contexts_dumper;
class sema_type;

struct type_data
{
  const sema_type& ty;
};

class types_context
{
public:
  virtual ~types_context() = default;

  virtual void register_type(const lexer::token& name, const sema_type& ty,
                             bool exported) = 0;

  // Todo: move to its own file and reuse
  struct type_with_reference
  {
    const sema_type& ty;
    const sema_type& ref;
  };
  virtual std::optional<type_with_reference> find(
    const std::vector<ast::name_with_coloncolon>& names) const = 0;

  virtual const sema_type* find_generic(
    const ast::type_representation& name) const = 0;

  virtual const sema_type* find_in_current_scope(
    const lexer::token& name) const = 0;

  virtual void enter_global_ctx(const lexer::token& name, bool exported) = 0;
  virtual void leave_ctx() = 0;

  virtual std::unique_ptr<types_context> clone() const = 0;
  virtual std::unique_ptr<types_context> collect_exported_stuff() const = 0;

  virtual bool merge_imported_stuff(const types_context& imported,
                                    errors::errors_observer& errs) = 0;

  virtual void dump(qualified_contexts_dumper& dumper) const = 0;
};

class types_context_impl : public types_context
{
public:
  void register_type(const lexer::token& name, const sema_type& ty,
                     bool exported) override;

  std::optional<type_with_reference> find(
    const std::vector<ast::name_with_coloncolon>& names) const override;

  const sema_type* find_generic(
    const ast::type_representation& name) const override;

  const sema_type* find_in_current_scope(
    const lexer::token& name) const override;

  void enter_global_ctx(const lexer::token& name, bool exported) override;
  void leave_ctx() override;

  std::unique_ptr<types_context> clone() const override;
  std::unique_ptr<types_context> collect_exported_stuff() const override;

  bool merge_imported_stuff(const types_context& imported,
                            errors::errors_observer& errs) override;

  void dump(qualified_contexts_dumper& dumper) const override;

private:
  qualified_entries_finder<type_data> m_types_finder;
};
}
}
