#include "cmsl_index.hpp"

#include "sema/identifiers_context.hpp"
#include "sema/sema_node_visitor.hpp"
#include "sema/sema_nodes.hpp"

#include <vector>
#include <ast/variable_declaration_node.hpp>
#include <cstring>

namespace cmsl
{
    cmsl_index_entry make_entry(const lexer::token::token& entry_token,
                                cmsl_index_entry_type type,
                                string_view destination_path,
                                unsigned destination_position)
    {
        cmsl_index_entry entry{};

        entry.begin_pos = entry_token.src_range().begin.absolute;
        entry.end_pos = entry_token.src_range().end.absolute;

        entry.type = type;

        entry.source_path = new char[destination_path.size() + 1u];
        std::strcpy(entry.source_path, destination_path.data());

        entry.position = destination_position;

        return entry;
    }


class indexer : public sema::sema_node_visitor
{
public:
    void visit(const sema::translation_unit_node& node) override
    {
        for(const auto& sub_node : node.nodes())
        {
            sub_node->visit(*this);
        }
    }

    void visit(const sema::variable_declaration_node& node) override
    {
        const auto& ast_node = node.ast_node();
        const auto variable_decl = dynamic_cast<const ast::variable_declaration_node*>(&ast_node);

        add_entry(variable_decl->get_type_representation().primary_name(),
                  cmsl_index_entry_type::type,
                  node.type().name().primary_name().source().path(),
                  node.type().name().primary_name().src_range().begin.absolute);


        m_identifiers_context.register_identifier(variable_decl->get_name(), &node.type());

        if(node.initialization())
        {
            node.initialization()->visit(*this);
        }
    }


    void visit(const sema::bool_value_node& node) override {}
    void visit(const sema::int_value_node& node) override {}
    void visit(const sema::double_value_node& node) override {}
    void visit(const sema::string_value_node& node) override {}

    void visit(const sema::id_node& node) override
    {
        const auto declaration_node = *m_identifiers_context.declaration_token_of(node.id().str());
        add_entry(node.id(),
                                      cmsl_index_entry_type::identifier,
                                      declaration_node.source().path(),
                                      declaration_node.src_range().begin.absolute);
    }

    void visit(const sema::return_node& node) override
    {
        node.expression().visit(*this);
    }

    void visit(const sema::binary_operator_node& node) override
    {
        node.lhs().visit(*this);
        node.rhs().visit(*this);

        const auto& operator_signature = node.operator_function().signature();
        add_entry(node.op(),
                                      cmsl_index_entry_type::operator_function,
                                      operator_signature.name.source().path(),
                                      operator_signature.name.src_range().begin.absolute);
    }

    void visit(const sema::function_call_node& node) override
    {
        visit_call_node(node);
    }

    void visit(const sema::member_function_call_node& node) override
    {
        node.lhs().visit(*this);
        visit_call_node(node);
    }

    void visit(const sema::implicit_member_function_call_node& node) override
    {
        visit_call_node(node);
    }

    void visit(const sema::constructor_call_node& node) override
    {
        visit_call_node(node);
    }

    void visit(const sema::block_node& node) override
    {
        m_identifiers_context.enter_ctx();

        for(const auto& sub_node : node.nodes())
        {
            sub_node->visit(*this);
        }

        m_identifiers_context.leave_ctx();
    }


    void visit(const sema::function_node& node) override
    {
        m_identifiers_context.enter_ctx();

        for(const auto& param_declaration : node.signature().params)
        {
            add_entry(param_declaration.name,
                      cmsl_index_entry_type::parameter_declaration,
                      param_declaration.ty.name().primary_name().source().path(),
                      param_declaration.ty.name().primary_name().src_range().begin.absolute);
        }

        node.body().visit(*this);

        m_identifiers_context.leave_ctx();
    }

    void visit(const sema::class_node& node) override
    {
        m_identifiers_context.enter_ctx();

        for(const auto& member : node.members())
        {
            member->visit(*this);
        }

        for(const auto& function : node.functions())
        {
            function->visit(*this);
        }

        m_identifiers_context.leave_ctx();
    }

    void visit(const sema::conditional_node& node) override
    {
        node.get_condition().visit(*this);
        node.get_body().visit(*this);
    }

    void visit(const sema::while_node& node) override
    {
        node.condition().visit(*this);
    }

    void visit(const sema::if_else_node& node) override
    {
        for(const auto& if_ : node.ifs())
        {
            if_->visit(*this);
        }

        if(const auto else_body = node.else_body())
        {
            else_body->visit(*this);
        }
    }

    void visit(const sema::class_member_access_node& node) override
    {
        const auto& lhs = node.lhs();
        lhs.visit(*this);

        const auto member = lhs.type().find_member(node.member_name().str());
        add_entry(node.member_name(),
                  cmsl_index_entry_type::class_member_identifier,
                  member->name.source().path(),
                  member->name.src_range().begin.absolute);
    }


    void visit(const sema::cast_to_reference_node& node) override
    {
        node.expression().visit(*this);
    }

    void visit(const sema::cast_to_value_node& node) override
    {
        node.expression().visit(*this);
    }

    void visit(const sema::initializer_list_node& node) override
    {
        for(const auto& sub_node : node.values())
        {
            sub_node->visit(*this);
        }
    }

    void visit(const sema::add_subdirectory_node& node) override
    {
        // Todo: this could point to main function of added subdirectory.
    }

private:
    void visit_call_node(const sema::call_node& node)
    {
        const auto name_node = node.name();
        const auto& signature = node.function().signature();
        add_entry(name_node,
                  cmsl_index_entry_type::function_call_name,
                  signature.name.source().path(),
                  signature.name.src_range().begin.absolute);

        for(const auto& param : node.param_expressions())
        {
            param->visit(*this);
        }
    }

    void add_entry(const lexer::token::token& entry_token,
                   cmsl_index_entry_type type,
                   string_view destination_path,
                   unsigned destination_position)
    {
        const auto entry = make_entry(entry_token, type, destination_path, destination_position);
        m_intermediate_entries.emplace_back(entry);
    }

private:
    std::vector<cmsl_index_entry> m_intermediate_entries;
    sema::identifiers_context_impl m_identifiers_context;
};
}


struct cmsl_index_entries* cmsl_index(const struct cmsl_parsed_source* parsed_source, unsigned absolute_position)
{

}

void cmsl_index(struct cmsl_index_entries* index_entries)
{
    for(auto i = 0u; i < index_entries->num_entries; ++i )
    {
        delete [] index_entries->entries[i].source_path;
    }

    delete [] index_entries->entries;
    delete index_entries;
}
