#pragma once

#include <string>
#include <vector>

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"

class Parser {
public:
    explicit Parser(Lexer lexer);

    void next_token();
    [[nodiscard]] auto parse_program() -> Program;
    [[nodiscard]] auto errors() const -> const std::vector<std::string>&;

private:
    [[nodiscard]] auto current_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto peek_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto expect_peek(TokenType type) -> bool;
    void peek_error(TokenType type);
    [[nodiscard]] auto parse_statement() -> std::unique_ptr<Statement>;
    [[nodiscard]] auto parse_let_statement() -> std::unique_ptr<LetStatement>;
    [[nodiscard]] auto parse_return_statement() -> std::unique_ptr<ReturnStatement>;

    Lexer lexer_;
    Token current_token_;
    Token peek_token_;
    std::vector<std::string> errors_ {};
};
