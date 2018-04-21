#include "lexer/token/token.hpp"

namespace cmsl
{
    namespace lexer
    {
        namespace token
        {
            token::token(token_type_t type)
                : token{ type, source_range{} }
            {}

            token::token(token_type_t type, source_range src_range)
                : m_type{ type }
                , m_source_range{ src_range }
            {}

            bool token::is_valid() const
            {
                return get_type() != token_type_t::undef;
            }

            token::token_type_t token::get_type() const
            {
                return m_type;
            }

            token token::undef()
            {
                return token{ token_type_t::undef };
            }
        }
    }
}
