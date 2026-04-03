#include "monkey/parser.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>

Parser::Parser(Lexer lexer) : lexer_(std::move(lexer)) {
    next_token();
    next_token();
}

void Parser::next_token() {
    current_token_ = peek_token_;
    peek_token_ = lexer_.next_token();
}

auto Parser::parse_program() -> Program {
    Program program;

    while (!current_token_is(TokenType::EndOfFile)) {
        if (auto statement = parse_statement()) {
            program.statements.push_back(std::move(statement));
        }

        next_token();
    }

    return program;
}

auto Parser::errors() const -> const std::vector<std::string>& {
    return errors_;
}

auto Parser::current_token_is(TokenType type) const -> bool {
    return current_token_.type == type;
}

auto Parser::peek_token_is(TokenType type) const -> bool {
    return peek_token_.type == type;
}

auto Parser::expect_peek(TokenType type) -> bool {
    if (!peek_token_is(type)) {
        peek_error(type);
        return false;
    }

    next_token();
    return true;
}

void Parser::peek_error(TokenType type) {
    errors_.push_back(
        "expected next token to be " + std::string(to_string(type)) +
        ", got " + std::string(to_string(peek_token_.type)) + " instead"
    );
}

auto Parser::parse_statement() -> std::unique_ptr<Statement> {
    if (current_token_is(TokenType::Let)) {
        return parse_let_statement();
    }

    if (current_token_is(TokenType::Return)) {
        return parse_return_statement();
    }

    return parse_expression_statement();
}

auto Parser::parse_expression_statement() -> std::unique_ptr<ExpressionStatement> {
    auto statement = std::make_unique<ExpressionStatement>();
    statement->token = current_token_;
    statement->expression = parse_expression(Precedence::Lowest);

    if (peek_token_is(TokenType::Semicolon)) {
        next_token();
    }

    return statement;
}

auto Parser::parse_expression(Precedence precedence) -> std::unique_ptr<Expression> {
    auto prefix_fn = prefix_parse_fn();
    if (!prefix_fn) {
        no_prefix_parse_fn_error(current_token_.type);
        return nullptr;
    }

    auto left_expr = prefix_fn();
    return left_expr;
}

void Parser::no_prefix_parse_fn_error(TokenType type) {
    errors_.push_back(
        "no prefix parse function for " + std::string(to_string(type)) + " found"
    );
}

auto Parser::parse_identifier() -> std::unique_ptr<Identifier> {
    auto identifier = std::make_unique<Identifier>();
    identifier->token = current_token_;
    identifier->value = current_token_.literal;
    return identifier;
}

auto Parser::parse_integer_literal() -> std::unique_ptr<IntegerLiteral> {
    auto literal = std::make_unique<IntegerLiteral>();
    literal->token = current_token_;
    literal->value = static_cast<std::int64_t>(std::stoll(current_token_.literal));
    return literal;
}

auto Parser::parse_prefix_expression() -> std::unique_ptr<PrefixExpression> {
    auto expression = std::make_unique<PrefixExpression>();
    expression->token = current_token_;
    expression->op = current_token_.literal;

    next_token();
    expression->right = parse_expression(Precedence::Prefix);
    return expression;
}

auto Parser::parse_let_statement() -> std::unique_ptr<LetStatement> {
    auto statement = std::make_unique<LetStatement>();
    statement->token = current_token_;

    if (!expect_peek(TokenType::Identifier)) {
        return nullptr;
    }

    statement->name = current_token_;

    if (!expect_peek(TokenType::Assign)) {
        return nullptr;
    }

    while (!current_token_is(TokenType::Semicolon)) {
        next_token();
    }

    return statement;
}

auto Parser::parse_return_statement() -> std::unique_ptr<ReturnStatement> {
    auto statement = std::make_unique<ReturnStatement>();
    statement->token = current_token_;

    next_token();

    while (!current_token_is(TokenType::Semicolon)) {
        next_token();
    }

    return statement;
}

auto Parser::prefix_parse_fn() -> ParsePrefixFn {
    if (current_token_is(TokenType::Identifier)) {
        return [this]() {
            return parse_identifier();
        };
    }

    if (current_token_is(TokenType::Integer)) {
        return [this]() {
            return parse_integer_literal();
        };
    }

    if (current_token_is(TokenType::Bang) || current_token_is(TokenType::Minus)) {
        return [this]() {
            return parse_prefix_expression();
        };
    }

    return {};
}
