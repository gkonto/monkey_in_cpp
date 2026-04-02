#pragma once

#include <string_view>

#include "monkey/token.hpp"

class Lexer {
public:
    explicit Lexer(std::string_view input);

    [[nodiscard]] auto next_token() -> Token;

private:
    [[nodiscard]] auto read_identifier() -> std::string;
    [[nodiscard]] auto read_number() -> std::string;
    [[nodiscard]] auto current_char() const -> char;
    [[nodiscard]] static auto is_letter(char ch) -> bool;
    [[nodiscard]] static auto is_digit(char ch) -> bool;
    void read_char();
    void skip_whitespace();

    std::string_view input_;
    std::size_t position_ {0};
    std::size_t read_position_ {0};
    char ch_ {0};
};
