#pragma once

#include "lexer/token/token.hpp"

#include <boost/optional.hpp>

#include <utility>
#include <vector>

namespace cmsl
{
    namespace sema
    {
        class sema_context_interface;
        class sema_function;
        class sema_type_builder;

        class sema_type
        {
        private:
            using token_t = lexer::token::token;

        public:
            struct member_info
            {
                lexer::token::token name;
                const sema_type& ty;
            };

            explicit sema_type(const sema_context_interface& ctx, token_t name, std::vector<member_info> members);

            sema_type(const sema_type&) = delete;
            sema_type& operator=(sema_type&&) = delete;

            token_t name() const;
            const sema_context_interface& context() const;
            boost::optional<member_info> find_member(cmsl::string_view name) const;
            const sema_function* find_member_function(cmsl::string_view name) const;

            bool is_builtin() const;
            bool operator==(const sema_type& rhs) const;
            bool operator!=(const sema_type& rhs) const;

        private:
            friend class sema_type_builder;

            const sema_context_interface& m_ctx;
            token_t m_name;
            std::vector<member_info> m_members;
        };
    }
}
