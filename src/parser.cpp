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

    while (!peek_token_is(TokenType::Semicolon) && precedence < peek_precedence()) {
        auto infix_fn = infix_parse_fn();
        if (!infix_fn) {
            return left_expr;
        }

        next_token();
        left_expr = infix_fn(std::move(left_expr));
    }

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

auto Parser::parse_boolean() -> std::unique_ptr<Boolean> {
    auto boolean = std::make_unique<Boolean>();
    boolean->token = current_token_;
    boolean->value = current_token_is(TokenType::True);
    return boolean;
}

auto Parser::parse_prefix_expression() -> std::unique_ptr<PrefixExpression> {
    auto expression = std::make_unique<PrefixExpression>();
    expression->token = current_token_;
    expression->op = current_token_.literal;

    next_token();
    expression->right = parse_expression(Precedence::Prefix);
    return expression;
}

auto Parser::parse_grouped_expression() -> std::unique_ptr<Expression> {
    next_token();

    auto expression = parse_expression(Precedence::Lowest);

    if (!expect_peek(TokenType::RightParen)) {
        return nullptr;
    }

    return expression;
}

auto Parser::parse_if_expression() -> std::unique_ptr<IfExpression> {
    auto expression = std::make_unique<IfExpression>();
    expression->token = current_token_;

    if (!expect_peek(TokenType::LeftParen)) {
        return nullptr;
    }

    next_token();
    expression->condition = parse_expression(Precedence::Lowest);

    if (!expect_peek(TokenType::RightParen)) {
        return nullptr;
    }

    if (!expect_peek(TokenType::LeftBrace)) {
        return nullptr;
    }

    expression->consequence = parse_block_statement();

    if (peek_token_is(TokenType::Else)) {
        next_token();

        if (!expect_peek(TokenType::LeftBrace)) {
            return nullptr;
        }

        expression->alternative = parse_block_statement();
    }

    return expression;
}

auto Parser::parse_function_literal() -> std::unique_ptr<FunctionLiteral> {
    auto literal = std::make_unique<FunctionLiteral>();
    literal->token = current_token_;

    if (!expect_peek(TokenType::LeftParen)) {
        return nullptr;
    }

    literal->parameters = parse_function_parameters();

    if (!expect_peek(TokenType::LeftBrace)) {
        return nullptr;
    }

    literal->body = parse_block_statement();
    return literal;
}

auto Parser::parse_infix_expression(std::unique_ptr<Expression> left) -> std::unique_ptr<Expression> {
    auto expression = std::make_unique<InfixExpression>();
    expression->token = current_token_;
    expression->op = current_token_.literal;
    expression->left = std::move(left);

    const auto precedence = current_precedence();
    next_token();
    expression->right = parse_expression(precedence);
    return expression;
}

auto Parser::parse_block_statement() -> std::unique_ptr<BlockStatement> {
    auto block = std::make_unique<BlockStatement>();
    block->token = current_token_;

    next_token();

    while (!current_token_is(TokenType::RightBrace) && !current_token_is(TokenType::EndOfFile)) {
        if (auto statement = parse_statement()) {
            block->statements.push_back(std::move(statement));
        }

        next_token();
    }

    return block;
}

auto Parser::parse_function_parameters() -> std::vector<std::unique_ptr<Identifier>> {
    std::vector<std::unique_ptr<Identifier>> identifiers;

    if (peek_token_is(TokenType::RightParen)) {
        next_token();
        return identifiers;
    }

    next_token();

    auto identifier = std::make_unique<Identifier>();
    identifier->token = current_token_;
    identifier->value = current_token_.literal;
    identifiers.push_back(std::move(identifier));

    while (peek_token_is(TokenType::Comma)) {
        next_token();
        next_token();

        auto next_identifier = std::make_unique<Identifier>();
        next_identifier->token = current_token_;
        next_identifier->value = current_token_.literal;
        identifiers.push_back(std::move(next_identifier));
    }

    if (!expect_peek(TokenType::RightParen)) {
        return {};
    }

    return identifiers;
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

    if (current_token_is(TokenType::True) || current_token_is(TokenType::False)) {
        return [this]() {
            return parse_boolean();
        };
    }

    if (current_token_is(TokenType::Bang) || current_token_is(TokenType::Minus)) {
        return [this]() {
            return parse_prefix_expression();
        };
    }

    if (current_token_is(TokenType::If)) {
        return [this]() {
            return parse_if_expression();
        };
    }

    if (current_token_is(TokenType::Function)) {
        return [this]() {
            return parse_function_literal();
        };
    }

    if (current_token_is(TokenType::LeftParen)) {
        return [this]() {
            return parse_grouped_expression();
        };
    }

    return {};
}

auto Parser::infix_parse_fn() -> ParseInfixFn {
    switch (peek_token_.type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Slash:
        case TokenType::Asterisk:
        case TokenType::Equal:
        case TokenType::NotEqual:
        case TokenType::LessThan:
        case TokenType::GreaterThan:
            return [this](std::unique_ptr<Expression> left) {
                return parse_infix_expression(std::move(left));
            };
        default:
            return {};
    }
}

auto Parser::current_precedence() const -> Precedence {
    return token_precedence(current_token_.type);
}

auto Parser::peek_precedence() const -> Precedence {
    return token_precedence(peek_token_.type);
}

auto Parser::token_precedence(TokenType type) -> Precedence {
    switch (type) {
        case TokenType::Equal:
        case TokenType::NotEqual:
            return Precedence::Equals;
        case TokenType::LessThan:
        case TokenType::GreaterThan:
            return Precedence::LessGreater;
        case TokenType::Plus:
        case TokenType::Minus:
            return Precedence::Sum;
        case TokenType::Slash:
        case TokenType::Asterisk:
            return Precedence::Product;
        default:
            return Precedence::Lowest;
    }
}
