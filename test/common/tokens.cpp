#include "test/common/tokens.hpp"
#include "common/enum_class_utils.hpp"


namespace cmsl
{
    namespace test
    {
        namespace common
        {
            using token_type_t = lexer::token::token_type;
            using token_t = lexer::token::token;

            namespace details
            {
                using tokens_str_t = enum_unordered_map<token_type_t, cmsl::string_view>;

                const tokens_str_t& operators_str()
                {
                    static tokens_str_t map = {
                            { token_type_t::dot, "." },
                            { token_type_t::open_brace, "{" },
                            { token_type_t::close_brace, "}" },
                            { token_type_t::open_square, "[" },
                            { token_type_t::close_square, "]" },
                            { token_type_t::open_paren, "(" },
                            { token_type_t::close_paren, ")" },
                            { token_type_t::equal, "=" },
                            { token_type_t::equalequal, "==" },
                            { token_type_t::minus, "-" },
                            { token_type_t::minusminus, "--" },
                            { token_type_t::minusequal, "-=" },
                            { token_type_t::plus, "+" },
                            { token_type_t::plusplus, "++" },
                            { token_type_t::plusequal, "+=" },
                            { token_type_t::amp, "&" },
                            { token_type_t::ampamp, "&&" },
                            { token_type_t::ampequal, "&=" },
                            { token_type_t::pipe, "|" },
                            { token_type_t::pipepipe, "||" },
                            { token_type_t::pipeequal, "|=" },
                            { token_type_t::slash, "/" },
                            { token_type_t::slashequal, "/=" },
                            { token_type_t::star, "*" },
                            { token_type_t::starequal, "*=" },
                            { token_type_t::percent, "%" },
                            { token_type_t::percentequal, "%=" },
                            { token_type_t::exclaim, "!" },
                            { token_type_t::exclaimequal, "!=" },
                            { token_type_t::xor_, "^" },
                            { token_type_t::xorequal, "^=" },
                            { token_type_t::less, "<" },
                            { token_type_t::lessequal, "<=" },
                            { token_type_t::greater, ">" },
                            { token_type_t::greaterequal, ">=" },
                            { token_type_t::semicolon, ";" },
                            { token_type_t::comma, "," },
                            { token_type_t::kw_int, "int" },
                            { token_type_t::kw_double, "double_" },
                            { token_type_t::kw_return, "return" },
                            { token_type_t::kw_class, "class" },
                            { token_type_t::kw_if, "if" },
                            { token_type_t::kw_else, "else" },
                            { token_type_t::kw_while, "while" },
                    };

                    return map;
                }

                token_t token_from_source(token_type_t type, cmsl::string_view source)
                {
                    const auto begin = source_location{};
                    source_location end;
                    end.line = 1u;
                    end.column = source.size();
                    end.absolute = source.size();
                    const auto source_range = cmsl::source_range{ begin, end };

                    return token_t{ type, source_range, source };
                }

                token_t simple_token(token_type_t type)
                {
                    const auto& map = operators_str();
                    const auto source = map.at(type);
                    return token_from_source(type, source);
                }
            }

            token_t token_dot() { return details::simple_token(token_type_t::dot); }
            token_t token_open_brace() { return details::simple_token(token_type_t::open_brace); }
            token_t token_close_brace() { return details::simple_token(token_type_t::close_brace); }
            token_t token_open_square() { return details::simple_token(token_type_t::open_square); }
            token_t token_close_square() { return details::simple_token(token_type_t::close_square); }
            token_t token_open_paren() { return details::simple_token(token_type_t::open_paren); }
            token_t token_close_paren() { return details::simple_token(token_type_t::close_paren); }
            token_t token_equal() { return details::simple_token(token_type_t::equal); }
            token_t token_equalequal() { return details::simple_token(token_type_t::equalequal); }
            token_t token_minus() { return details::simple_token(token_type_t::minus); }
            token_t token_minusminus() { return details::simple_token(token_type_t::minusminus); }
            token_t token_minusequal() { return details::simple_token(token_type_t::minusequal); }
            token_t token_plus() { return details::simple_token(token_type_t::plus); }
            token_t token_plusplus() { return details::simple_token(token_type_t::plusplus); }
            token_t token_plusequal() { return details::simple_token(token_type_t::plusequal); }
            token_t token_amp() { return details::simple_token(token_type_t::amp); }
            token_t token_ampamp() { return details::simple_token(token_type_t::ampamp); }
            token_t token_ampequal() { return details::simple_token(token_type_t::ampequal); }
            token_t token_pipe() { return details::simple_token(token_type_t::pipe); }
            token_t token_pipepipe() { return details::simple_token(token_type_t::pipepipe); }
            token_t token_pipeequal() { return details::simple_token(token_type_t::pipeequal); }
            token_t token_slash() { return details::simple_token(token_type_t::slash); }
            token_t token_slashequal() { return details::simple_token(token_type_t::slashequal); }
            token_t token_star() { return details::simple_token(token_type_t::star); }
            token_t token_starequal() { return details::simple_token(token_type_t::starequal); }
            token_t token_percent() { return details::simple_token(token_type_t::percent); }
            token_t token_percentequal() { return details::simple_token(token_type_t::percentequal); }
            token_t token_exclaim() { return details::simple_token(token_type_t::exclaim); }
            token_t token_exclaimequal() { return details::simple_token(token_type_t::exclaimequal); }
            token_t token_xor() { return details::simple_token(token_type_t::xor_); }
            token_t token_xorequal() { return details::simple_token(token_type_t::xorequal); }
            token_t token_less() { return details::simple_token(token_type_t::less); }
            token_t token_lessequal() { return details::simple_token(token_type_t::lessequal); }
            token_t token_greater() { return details::simple_token(token_type_t::greater); }
            token_t token_greaterequal() { return details::simple_token(token_type_t::greaterequal); }
            token_t token_semicolon() { return details::simple_token(token_type_t::semicolon); }
            token_t token_comma() { return details::simple_token(token_type_t::comma); }
            token_t token_t_int() { return details::simple_token(token_type_t::kw_int); }
            token_t token_t_real() { return details::simple_token(token_type_t::kw_double); }
            token_t token_kw_return() { return details::simple_token(token_type_t::kw_return); }
            token_t token_kw_class() { return details::simple_token(token_type_t::kw_class); }
            token_t token_kw_if() { return details::simple_token(token_type_t::kw_if); }
            token_t token_kw_else() { return details::simple_token(token_type_t::kw_else); }
            token_t token_kw_while() { return details::simple_token(token_type_t::kw_while); }


            token_t token_undef() { return token_t{ token_type_t::undef }; }
            token_t token_real() { return token_t{ token_type_t::double_ }; }
            token_t token_string() { return token_t{ token_type_t::string }; }
            token_t token_integer(cmsl::string_view str)
            { 
                return details::token_from_source(token_type_t::integer, str);
            }
            token_t token_identifier(cmsl::string_view str)
            {
                return details::token_from_source(token_type_t::identifier, str);
            }
        }
    }
}