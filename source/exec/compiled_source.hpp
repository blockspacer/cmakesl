#pragma once

#include "common/source_view.hpp"
#include "sema/builtin_types_accessor.hpp"

#include <memory>
#include <vector>

namespace cmsl {
namespace ast {
class ast_node;
}

namespace decl_ast {
class ast_node;
}

namespace sema {
class builtin_token_provider;
class sema_node;
class sema_function;
class builtin_sema_context;
class sema_context;
class identifiers_context;
class types_context;
class enum_values_context;
}

namespace decl_sema {
class sema_node;
class component_declaration_node;
}

namespace exec {
class compiled_source
{
public:
  explicit compiled_source(std::unique_ptr<ast::ast_node> ast_tree,
                           const sema::sema_context& global_context,
                           std::unique_ptr<sema::sema_node> sema_tree,
                           source_view source,
                           sema::builtin_types_accessor builtin_types);
  ~compiled_source();

  const sema::sema_function* get_main() const;
  const sema::sema_context& get_global_context() const;

  const sema::sema_node& sema_tree() const;

  sema::builtin_types_accessor builtin_types() const;

private:
  std::unique_ptr<ast::ast_node> m_ast_tree;
  const sema::sema_context& m_global_context;
  std::unique_ptr<sema::sema_node> m_sema_tree;
  source_view m_source;
  sema::builtin_types_accessor m_builtin_types;
};

class compiled_declarative_source
{
public:
  using functions_t =
    std::vector<std::reference_wrapper<const sema::sema_function>>;

  explicit compiled_declarative_source(
    std::unique_ptr<decl_ast::ast_node> ast_tree,
    std::unique_ptr<decl_sema::sema_node> sema_tree, source_view source,
    sema::builtin_types_accessor builtin_types,
    const sema::sema_function* creation_function,
    string_view_map<const decl_sema::component_declaration_node*>
      component_declarations);
  ~compiled_declarative_source();

  const sema::sema_function* get_target_creation_function() const;
  const string_view_map<const decl_sema::component_declaration_node*>&
  component_declarations() const;

private:
  std::unique_ptr<decl_ast::ast_node> m_ast_tree;
  std::unique_ptr<decl_sema::sema_node> m_sema_tree;
  source_view m_source;
  sema::builtin_types_accessor m_builtin_types;
  const sema::sema_function* m_creation_function;
  string_view_map<const decl_sema::component_declaration_node*>
    m_component_declarations;
};
}
}
