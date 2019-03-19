#pragma once

#include "ast/type.hpp"
#include "exec/instance/instance.hpp"

namespace cmsl
{
    namespace exec
    {
        namespace inst
        {
            // Todo: consider rename to builtin_unnamed_instance
            class simple_unnamed_instance : public instance
            {
            public:
                explicit simple_unnamed_instance(const ast::type &type);
                explicit simple_unnamed_instance(kind k, const ast::type &type);
                explicit simple_unnamed_instance(const ast::type &type, instance_value_t value);

                explicit simple_unnamed_instance(const sema::sema_type &type);
                explicit simple_unnamed_instance(const sema::sema_type &type, instance_value_t value);


                virtual ~simple_unnamed_instance() = default;

                instance_value_t get_value() const override;
                instance_value_t& get_value_ref() override;
                const instance_value_t& get_value_cref() const override;
                void assign(instance_value_t val) override;
                std::unique_ptr<instance> copy() const override;
                instance *get_member(cmsl::string_view name) override;
                const instance *get_cmember(cmsl::string_view name) const override;
                bool has_function(cmsl::string_view name) const override;
                bool is_ctor(cmsl::string_view name) const override;
                bool is_fundamental() const;
                const ast::function* get_function(cmsl::string_view name) const override;
                const ast::type& get_type() const override;


                sema::single_scope_function_lookup_result_t get_sema_function(lexer::token::token name) const override;
                const sema::sema_type& get_sema_type() const override;

            private:
                instance_value_t get_init_data() const;
                instance_value_t get_init_data(instance_value_t val) const;
                instance_value_t get_init_list_data() const;

            private:
                kind m_kind;
                const ast::type *m_type{nullptr};
                const sema::sema_type* m_sema_type{nullptr}; // Todo: change to a reference
                instance_value_t m_data;
            };
        }
    }
}
