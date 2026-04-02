#include <catch2/catch_test_macros.hpp>

#include "monkey/lexer.hpp"
#include "monkey/token.hpp"

TEST_CASE("TestNextToken", "[lexer]") {
    constexpr auto input =
        "let five = 5;\n"
        "let ten = 10;\n"
        "let add = fn(x, y) {\n"
        "  x + y;\n"
        "};\n"
        "let result = add(five, ten);";

    const Token expected_tokens[] = {
        {TokenType::Let, "let"},
        {TokenType::Identifier, "five"},
        {TokenType::Assign, "="},
        {TokenType::Integer, "5"},
        {TokenType::Semicolon, ";"},
        {TokenType::Let, "let"},
        {TokenType::Identifier, "ten"},
        {TokenType::Assign, "="},
        {TokenType::Integer, "10"},
        {TokenType::Semicolon, ";"},
        {TokenType::Let, "let"},
        {TokenType::Identifier, "add"},
        {TokenType::Assign, "="},
        {TokenType::Function, "fn"},
        {TokenType::LeftParen, "("},
        {TokenType::Identifier, "x"},
        {TokenType::Comma, ","},
        {TokenType::Identifier, "y"},
        {TokenType::RightParen, ")"},
        {TokenType::LeftBrace, "{"},
        {TokenType::Identifier, "x"},
        {TokenType::Plus, "+"},
        {TokenType::Identifier, "y"},
        {TokenType::Semicolon, ";"},
        {TokenType::RightBrace, "}"},
        {TokenType::Semicolon, ";"},
        {TokenType::Let, "let"},
        {TokenType::Identifier, "result"},
        {TokenType::Assign, "="},
        {TokenType::Identifier, "add"},
        {TokenType::LeftParen, "("},
        {TokenType::Identifier, "five"},
        {TokenType::Comma, ","},
        {TokenType::Identifier, "ten"},
        {TokenType::RightParen, ")"},
        {TokenType::Semicolon, ";"},
        {TokenType::EndOfFile, ""},
    };

    auto lexer = Lexer {input};

    for (const auto& expected : expected_tokens) {
        const auto token = lexer.next_token();
        REQUIRE(token.type == expected.type);
        REQUIRE(token.literal == expected.literal);
    }
}
