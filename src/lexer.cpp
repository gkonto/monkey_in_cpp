#include "monkey/lexer.hpp"

Lexer::Lexer(std::string_view input) : input_(input) {
    read_char();
}

auto Lexer::next_token() -> Token {
    skip_whitespace();

    switch (ch_) {
        case '=':
            if (peek_char() == '=') {
                read_char();
                read_char();
                return {TokenType::Equal, "=="};
            }
            read_char();
            return {TokenType::Assign, "="};
        case '!':
            if (peek_char() == '=') {
                read_char();
                read_char();
                return {TokenType::NotEqual, "!="};
            }
            read_char();
            return {TokenType::Bang, "!"};
        case '+':
            read_char();
            return {TokenType::Plus, "+"};
        case '-':
            read_char();
            return {TokenType::Minus, "-"};
        case '/':
            read_char();
            return {TokenType::Slash, "/"};
        case '*':
            read_char();
            return {TokenType::Asterisk, "*"};
        case '<':
            read_char();
            return {TokenType::LessThan, "<"};
        case '>':
            read_char();
            return {TokenType::GreaterThan, ">"};
        case '(':
            read_char();
            return {TokenType::LeftParen, "("};
        case ')':
            read_char();
            return {TokenType::RightParen, ")"};
        case '{':
            read_char();
            return {TokenType::LeftBrace, "{"};
        case '}':
            read_char();
            return {TokenType::RightBrace, "}"};
        case ',':
            read_char();
            return {TokenType::Comma, ","};
        case ';':
            read_char();
            return {TokenType::Semicolon, ";"};
        case 0:
            return {TokenType::EndOfFile, ""};
    }

    if (is_letter(ch_)) {
        const auto literal = read_identifier();
        return {lookup_identifier(literal), literal};
    }

    if (is_digit(ch_)) {
        return {TokenType::Integer, read_number()};
    }

    const auto token = Token {TokenType::Illegal, std::string(1, ch_)};
    read_char();
    return token;
}

auto Lexer::read_identifier() -> std::string {
    const auto start = position_;

    while (is_letter(ch_)) {
        read_char();
    }

    return std::string(input_.substr(start, position_ - start));
}

auto Lexer::read_number() -> std::string {
    const auto start = position_;

    while (is_digit(ch_)) {
        read_char();
    }

    return std::string(input_.substr(start, position_ - start));
}

auto Lexer::current_char() const -> char {
    return ch_;
}

auto Lexer::peek_char() const -> char {
    if (read_position_ >= input_.size()) {
        return 0;
    }

    return input_[read_position_];
}

auto Lexer::is_letter(char ch) -> bool {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

auto Lexer::is_digit(char ch) -> bool {
    return '0' <= ch && ch <= '9';
}

void Lexer::read_char() {
    if (read_position_ >= input_.size()) {
        ch_ = 0;
    } else {
        ch_ = input_[read_position_];
    }

    position_ = read_position_;
    ++read_position_;
}

void Lexer::skip_whitespace() {
    while (current_char() == ' ' || current_char() == '\t' || current_char() == '\n' || current_char() == '\r') {
        read_char();
    }
}
