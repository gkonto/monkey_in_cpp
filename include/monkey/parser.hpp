#pragma once

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"

class Parser {
public:
    explicit Parser(Lexer lexer);

    void next_token();
    [[nodiscard]] auto parse_program() -> Program;

private:
    [[nodiscard]] auto current_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto peek_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto expect_peek(TokenType type) -> bool;
    [[nodiscard]] auto parse_statement() -> std::unique_ptr<Statement>;
    [[nodiscard]] auto parse_let_statement() -> std::unique_ptr<LetStatement>;

    Lexer lexer_;
    Token current_token_;
    Token peek_token_;
};
