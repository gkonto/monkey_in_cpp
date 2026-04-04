#pragma once

#include <string>
#include <string_view>

enum class TokenType {
    Illegal,
    EndOfFile,

    Identifier,
    Integer,
    String,
    
    Assign,
    Bang,
    Equal,
    NotEqual,
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
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,

    Function,
    Let,
    True,
    False,
    If,
    Else,
    Return,
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

    if (literal == "true") {
        return TokenType::True;
    }

    if (literal == "false") {
        return TokenType::False;
    }

    if (literal == "if") {
        return TokenType::If;
    }

    if (literal == "else") {
        return TokenType::Else;
    }

    if (literal == "return") {
        return TokenType::Return;
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
        case TokenType::String:
            return "String";
        case TokenType::Assign:
            return "Assign";
        case TokenType::Bang:
            return "Bang";
        case TokenType::Equal:
            return "Equal";
        case TokenType::NotEqual:
            return "NotEqual";
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
        case TokenType::LeftBracket:
            return "LeftBracket";
        case TokenType::RightBracket:
            return "RightBracket";
        case TokenType::LeftBrace:
            return "LeftBrace";
        case TokenType::RightBrace:
            return "RightBrace";
        case TokenType::Function:
            return "Function";
        case TokenType::Let:
            return "Let";
        case TokenType::True:
            return "True";
        case TokenType::False:
            return "False";
        case TokenType::If:
            return "If";
        case TokenType::Else:
            return "Else";
        case TokenType::Return:
            return "Return";
    }

    return "Unknown";
}
