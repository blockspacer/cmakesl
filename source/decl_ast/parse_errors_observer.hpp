#pragma once

#include "errors/error.hpp"
#include "errors/errors_observer.hpp"
#include "parse/parse_errors_observer.hpp"

namespace cmsl::decl_ast {
class parse_errors_observer : public parse::parse_errors_observer
{
public:
  virtual ~parse_errors_observer() = default;

  virtual void raise_expected_component_or_property(lexer::token token) = 0;
  virtual void raise_unexpected_list_in_list(lexer::token token) = 0;
};

class parse_errors_reporter : public parse_errors_observer
{
public:
  explicit parse_errors_reporter(errors::errors_observer& errs_observer,
                                 cmsl::source_view source)
    : m_errors_observer{ errs_observer }
    , m_source{ source }
  {
  }

  void raise_unexpected_end_of_file(lexer::token token) override
  {
    raise(token, "unexpected end of file");
  }

  void raise_expected_token(lexer::token token,
                            lexer::token_type type) override
  {
    raise(token, "expected " + to_string(type));
  }

  void raise_unexpected_token(lexer::token token) override
  {
    raise(token, "unexpected token");
  }

  void raise_expected_keyword(lexer::token token,
                              lexer::token_type keyword) override
  {
    raise(token, "expected " + to_string(keyword));
  }

  void raise_expected_expression(lexer::token token) override
  {
    raise(token, "expected expression");
  }

  void raise_expected_component_or_property(lexer::token token) override
  {
    raise(token, "expected component or property");
  }

  void raise_unexpected_list_in_list(lexer::token token) override
  {
    raise(token, "list can not contain another list");
  }

private:
  void raise(lexer::token token, std::string message)
  {
    const auto line_info = m_source.line(token.src_range().begin.line);
    auto err = errors::error{ m_source.path(),           line_info.line,
                              line_info.start_pos,       std::move(message),
                              errors::error_type::error, token.src_range() };
    m_errors_observer.notify_error(std::move(err));
  }

private:
  errors::errors_observer& m_errors_observer;
  cmsl::source_view m_source;
};
}
