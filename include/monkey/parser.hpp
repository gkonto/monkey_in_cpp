#pragma once

#include <functional>
#include <string>
#include <vector>

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"

class Parser {
public:
    enum class Precedence {
        Lowest,
        Equals,
        LessGreater,
        Sum,
        Product,
        Prefix,
        Call
    };

    explicit Parser(Lexer lexer);

    void next_token();
    [[nodiscard]] auto parse_program() -> Program;
    [[nodiscard]] auto errors() const -> const std::vector<std::string>&;

private:
    using ParsePrefixFn = std::function<std::unique_ptr<Expression>()>;
    using ParseInfixFn = std::function<std::unique_ptr<Expression>(std::unique_ptr<Expression>)>;

    [[nodiscard]] auto current_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto peek_token_is(TokenType type) const -> bool;
    [[nodiscard]] auto expect_peek(TokenType type) -> bool;
    void peek_error(TokenType type);
    void no_prefix_parse_fn_error(TokenType type);
    [[nodiscard]] auto parse_statement() -> std::unique_ptr<Statement>;
    [[nodiscard]] auto parse_expression_statement() -> std::unique_ptr<ExpressionStatement>;
    [[nodiscard]] auto parse_expression(Precedence precedence) -> std::unique_ptr<Expression>;
    [[nodiscard]] auto parse_identifier() -> std::unique_ptr<Identifier>;
    [[nodiscard]] auto parse_integer_literal() -> std::unique_ptr<IntegerLiteral>;
    [[nodiscard]] auto parse_boolean() -> std::unique_ptr<Boolean>;
    [[nodiscard]] auto parse_prefix_expression() -> std::unique_ptr<PrefixExpression>;
    [[nodiscard]] auto parse_infix_expression(std::unique_ptr<Expression> left) -> std::unique_ptr<Expression>;
    [[nodiscard]] auto parse_let_statement() -> std::unique_ptr<LetStatement>;
    [[nodiscard]] auto parse_return_statement() -> std::unique_ptr<ReturnStatement>;
    [[nodiscard]] auto prefix_parse_fn() -> ParsePrefixFn;
    [[nodiscard]] auto infix_parse_fn() -> ParseInfixFn;
    [[nodiscard]] auto current_precedence() const -> Precedence;
    [[nodiscard]] auto peek_precedence() const -> Precedence;
    [[nodiscard]] static auto token_precedence(TokenType type) -> Precedence;

    Lexer lexer_;
    Token current_token_;
    Token peek_token_;
    std::vector<std::string> errors_ {};
};
