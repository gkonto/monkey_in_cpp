#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "monkey/ast.hpp"

TEST_CASE("Program token_literal returns the first statement literal", "[ast]") {
    auto statement = std::make_unique<LetStatement>();
    statement->token = Token {TokenType::Let, "let"};
    statement->name = Token {TokenType::Identifier, "myVar"};

    Program program {};
    program.statements.push_back(std::move(statement));

    REQUIRE(program.token_literal() == "let");
}

TEST_CASE("Program token_literal returns empty string when there are no statements", "[ast]") {
    const Program program {};

    REQUIRE(program.token_literal().empty());
}

TEST_CASE("LetStatement token_literal returns its token literal", "[ast]") {
    LetStatement statement {};
    statement.token = Token {TokenType::Let, "let"};
    statement.name = Token {TokenType::Identifier, "anotherVar"};

    REQUIRE(statement.token_literal() == "let");
}

TEST_CASE("Identifier token_literal returns its token literal", "[ast]") {
    Identifier identifier {};
    identifier.token = Token {TokenType::Identifier, "myVar"};
    identifier.value = "myVar";

    REQUIRE(identifier.token_literal() == "myVar");
    REQUIRE(identifier.value == "myVar");
}

TEST_CASE("Identifier as_string returns its value", "[ast]") {
    Identifier identifier {};
    identifier.token = Token {TokenType::Identifier, "myVar"};
    identifier.value = "myVar";

    REQUIRE(identifier.as_string() == "myVar");
}

TEST_CASE("Program as_string concatenates statement strings", "[ast]") {
    auto value = std::make_unique<Identifier>();
    value->token = Token {TokenType::Identifier, "anotherVar"};
    value->value = "anotherVar";

    auto statement = std::make_unique<LetStatement>();
    statement->token = Token {TokenType::Let, "let"};
    statement->name = Token {TokenType::Identifier, "myVar"};
    statement->value = std::move(value);

    Program program {};
    program.statements.push_back(std::move(statement));

    REQUIRE(program.as_string() == "let myVar = anotherVar;");
}
