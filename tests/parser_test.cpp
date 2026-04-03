#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

void check_parser_errors(const Parser& parser) {
    const auto& errors = parser.errors();

    REQUIRE(errors.empty());
}

void test_let_statement(const Statement* statement, std::string_view expected_name) {
    REQUIRE(statement != nullptr);

    const auto* let_statement = dynamic_cast<const LetStatement*>(statement);
    REQUIRE(let_statement != nullptr);
    REQUIRE(let_statement->token_literal() == "let");
    REQUIRE(let_statement->name.literal == expected_name);
}

void test_return_statement(const Statement* statement) {
    REQUIRE(statement != nullptr);

    const auto* return_statement = dynamic_cast<const ReturnStatement*>(statement);
    REQUIRE(return_statement != nullptr);
    REQUIRE(return_statement->token_literal() == "return");
}

}  // namespace

TEST_CASE("TestLetStatement", "[parser]") {
    constexpr auto input =
        "let x = 5;\n"
        "let y = 10;\n"
        "let foobar = 838383;";

    auto lexer = Lexer{input};
    auto parser = Parser{lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 3);

    constexpr std::string_view expected_identifiers[] = {"x", "y", "foobar"};

    for (std::size_t index = 0; index < std::size(expected_identifiers); ++index) {
        test_let_statement(program.statements[index].get(), expected_identifiers[index]);
    }
}

TEST_CASE("TestReturnStatements", "[parser]") {
    constexpr auto input =
        "return 5;\n"
        "return 10;\n"
        "return 993322;";

    auto lexer = Lexer{input};
    auto parser = Parser{lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 3);

    for (const auto& statement : program.statements) {
        test_return_statement(statement.get());
    }
}

TEST_CASE("TestIdentifierExpression", "[parser]") {
    constexpr auto input = "foobar;";

    auto lexer = Lexer{input};
    auto parser = Parser{lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* identifier = dynamic_cast<const Identifier*>(statement->expression.get());
    REQUIRE(identifier != nullptr);
    REQUIRE(identifier->value == "foobar");
    REQUIRE(identifier->token_literal() == "foobar");
}

TEST_CASE("TestIntegerLiteralExpression", "[parser]") {
    constexpr auto input = "5;";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(statement->expression.get());
    REQUIRE(integer_literal != nullptr);
    REQUIRE(integer_literal->value == 5);
    REQUIRE(integer_literal->token_literal() == "5");
}

TEST_CASE("TestParsingPrefix", "[parser]") {
    struct TestCase {
        std::string_view input;
        std::string_view op;
        std::int64_t integer_value;
    };

    constexpr TestCase test_cases[] = {
        {"!5;", "!", 5},
        {"-15;", "-", 15},
    };

    for (const auto& test_case : test_cases) {
        auto lexer = Lexer {test_case.input};
        auto parser = Parser {lexer};
        const auto program = parser.parse_program();

        check_parser_errors(parser);
        REQUIRE(program.statements.size() == 1);

        const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
        REQUIRE(statement != nullptr);
        REQUIRE(statement->expression != nullptr);

        const auto* prefix_expression = dynamic_cast<const PrefixExpression*>(statement->expression.get());
        REQUIRE(prefix_expression != nullptr);
        REQUIRE(prefix_expression->op == test_case.op);
        REQUIRE(prefix_expression->right != nullptr);

        const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(prefix_expression->right.get());
        REQUIRE(integer_literal != nullptr);
        REQUIRE(integer_literal->value == test_case.integer_value);
    }
}
