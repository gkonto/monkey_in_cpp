#pragma once

#include <string>
#include <string_view>

enum class TokenType {
    Illegal,
    EndOfFile,

    Identifier,
    Integer,
    
    Assign,
    Bang,
    Plus,
    Minus,
    Slash,
    Asterisk,
    LessThan,
    GreaterThan,
    
    Comma,
    Semicolon,

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,

    Function,
    Let,
};

struct Token {
    TokenType type {TokenType::Illegal};
    std::string literal;
};

[[nodiscard]] constexpr auto lookup_identifier(std::string_view literal) -> TokenType {
    if (literal == "fn") {
        return TokenType::Function;
    }

    if (literal == "let") {
        return TokenType::Let;
    }

    return TokenType::Identifier;
}

[[nodiscard]] constexpr auto to_string(TokenType type) -> std::string_view {
    switch (type) {
        case TokenType::Illegal:
            return "Illegal";
        case TokenType::EndOfFile:
            return "EndOfFile";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::Integer:
            return "Integer";
        case TokenType::Assign:
            return "Assign";
        case TokenType::Bang:
            return "Bang";
        case TokenType::Plus:
            return "Plus";
        case TokenType::Minus:
            return "Minus";
        case TokenType::Slash:
            return "Slash";
        case TokenType::Asterisk:
            return "Asterisk";
        case TokenType::LessThan:
            return "LessThan";
        case TokenType::GreaterThan:
            return "GreaterThan";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::LeftParen:
            return "LeftParen";
        case TokenType::RightParen:
            return "RightParen";
        case TokenType::LeftBrace:
            return "LeftBrace";
        case TokenType::RightBrace:
            return "RightBrace";
        case TokenType::Function:
            return "Function";
        case TokenType::Let:
            return "Let";
    }

    return "Unknown";
}
