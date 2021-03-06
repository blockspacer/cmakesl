#include "completion_contextes_visitor.hpp"
#include "cmsl_complete.hpp"
#include "completer.hpp"
#include "identifier_names_collector.hpp"
#include "type_names_collector.hpp"

#include "cmsl_parsed_source.hpp"
#include "sema/sema_nodes.hpp"

#include <cstring>

namespace cmsl::tools {
completion_contextes_visitor::completion_contextes_visitor(
  const cmsl_parsed_source& parsed_source, cmsl_complete_results& results)
  : m_parsed_source{ parsed_source }
  , m_results{ results }
{
}

void completion_contextes_visitor::operator()(
  const standalone_expression_context& ctx)
{
  if (const auto block =
        dynamic_cast<const sema::block_node*>(&ctx.node.get())) {
    add_standalone_expression_keywords();

    const auto type_names =
      type_names_collector{}.collect(*m_parsed_source.builtin_context, *block);
    add_results(type_names);

    const auto identifiers = identifier_names_collector{}.collect(*block);
    add_results(identifiers);
  }
}

void completion_contextes_visitor::operator()(
  const top_level_declaration_context& ctx)
{
  add_top_level_declaration_keywords();

  const auto type_names =
    type_names_collector{}.collect(*m_parsed_source.builtin_context, ctx.node);
  add_results(type_names);
}

void completion_contextes_visitor::operator()(
  const class_member_declaration_context& ctx)
{
  // In class context it's either a member declaration or a function
  // declaration. Both start with a type, so type collecting is enough.

  m_intermediate_results.emplace_back(ctx.node.get().name().str());

  const auto type_names =
    type_names_collector{}.collect(*m_parsed_source.builtin_context, ctx.node);
  add_results(type_names);
}

void completion_contextes_visitor::add_standalone_expression_keywords()
{
  const auto keywords = { "if", "for", "while" };

  add_results(keywords);
}

void completion_contextes_visitor::add_top_level_declaration_keywords()
{
  const auto keywords = { "class" };

  add_results(keywords);
}

void completion_contextes_visitor::finalize()
{
  if (m_intermediate_results.empty()) {
    return;
  }

  m_results.num_results = static_cast<unsigned>(m_intermediate_results.size());
  m_results.results = new char*[m_intermediate_results.size()];

  for (auto i = 0u; i < m_intermediate_results.size(); ++i) {
    const auto& str = m_intermediate_results[i];
    m_results.results[i] = new char[str.size() + 1];
    std::strcpy(m_results.results[i], str.c_str());
  }
}

template <typename Collection>
void completion_contextes_visitor::add_results(Collection&& collection)
{
  for (const auto& entry : collection) {
    m_intermediate_results.emplace_back(entry);
  }
}
}
