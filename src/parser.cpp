#include "monkey/parser.hpp"

#include <memory>

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

auto Parser::current_token_is(TokenType type) const -> bool {
    return current_token_.type == type;
}

auto Parser::peek_token_is(TokenType type) const -> bool {
    return peek_token_.type == type;
}

auto Parser::expect_peek(TokenType type) -> bool {
    if (!peek_token_is(type)) {
        return false;
    }

    next_token();
    return true;
}

auto Parser::parse_statement() -> std::unique_ptr<Statement> {
    if (current_token_is(TokenType::Let)) {
        return parse_let_statement();
    }

    return nullptr;
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
