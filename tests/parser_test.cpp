#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <variant>

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

void test_identifier(const Expression* expression, std::string_view expected_value) {
    REQUIRE(expression != nullptr);

    const auto* identifier = dynamic_cast<const Identifier*>(expression);
    REQUIRE(identifier != nullptr);
    REQUIRE(identifier->value == expected_value);
    REQUIRE(identifier->token_literal() == expected_value);
}

void test_integer_literal(const Expression* expression, std::int64_t expected_value) {
    REQUIRE(expression != nullptr);

    const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(expression);
    REQUIRE(integer_literal != nullptr);
    REQUIRE(integer_literal->value == expected_value);
    REQUIRE(integer_literal->token_literal() == std::to_string(expected_value));
}

void test_boolean_literal(const Expression* expression, bool expected_value) {
    REQUIRE(expression != nullptr);

    const auto* boolean = dynamic_cast<const Boolean*>(expression);
    REQUIRE(boolean != nullptr);
    REQUIRE(boolean->value == expected_value);
    REQUIRE(boolean->token_literal() == (expected_value ? "true" : "false"));
}

template <typename T>
void test_literal_expression(const Expression* expression, const T& expected_value) {
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, std::string_view>) {
        test_identifier(expression, expected_value);
    } else if constexpr (std::is_same_v<ValueType, std::int64_t>) {
        test_integer_literal(expression, expected_value);
    } else if constexpr (std::is_same_v<ValueType, bool>) {
        test_boolean_literal(expression, expected_value);
    } else if constexpr (requires { std::visit([](const auto&) {}, expected_value); }) {
        std::visit([&](const auto& value) {
            test_literal_expression(expression, value);
        }, expected_value);
    }
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

    test_literal_expression(statement->expression.get(), std::string_view {"foobar"});
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

    test_literal_expression(statement->expression.get(), std::int64_t {5});
}

TEST_CASE("TestParsingPrefix", "[parser]") {
    using Literal = std::variant<std::int64_t, bool>;

    struct TestCase {
        std::string_view input;
        std::string_view op;
        Literal value;
    };

    constexpr TestCase test_cases[] = {
        {"!5;", "!", std::int64_t {5}},
        {"-15;", "-", std::int64_t {15}},
        {"!true;", "!", true},
        {"!false;", "!", false},
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

        test_literal_expression(prefix_expression->right.get(), test_case.value);
    }
}

TEST_CASE("TestParsingInfixExpressions", "[parser]") {
    using Literal = std::variant<std::int64_t, bool>;

    struct TestCase {
        std::string_view input;
        Literal left_value;
        std::string_view op;
        Literal right_value;
    };

    constexpr TestCase test_cases[] = {
        {"5 + 5;", std::int64_t {5}, "+", std::int64_t {5}},
        {"5 - 5;", std::int64_t {5}, "-", std::int64_t {5}},
        {"5 * 5;", std::int64_t {5}, "*", std::int64_t {5}},
        {"5 / 5;", std::int64_t {5}, "/", std::int64_t {5}},
        {"5 > 5;", std::int64_t {5}, ">", std::int64_t {5}},
        {"5 < 5;", std::int64_t {5}, "<", std::int64_t {5}},
        {"5 == 5;", std::int64_t {5}, "==", std::int64_t {5}},
        {"5 != 5;", std::int64_t {5}, "!=", std::int64_t {5}},
        {"true == true", true, "==", true},
        {"true != false", true, "!=", false},
        {"false == false", false, "==", false},
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

        const auto* infix_expression = dynamic_cast<const InfixExpression*>(statement->expression.get());
        REQUIRE(infix_expression != nullptr);
        REQUIRE(infix_expression->left != nullptr);
        REQUIRE(infix_expression->op == test_case.op);
        REQUIRE(infix_expression->right != nullptr);

        test_literal_expression(infix_expression->left.get(), test_case.left_value);
        test_literal_expression(infix_expression->right.get(), test_case.right_value);
    }
}

TEST_CASE("TestOperatorPrecedenceParsing", "[parser]") {
    struct TestCase {
        std::string_view input;
        std::string_view expected;
    };

    constexpr TestCase test_cases[] = {
        {"-a * b", "((-a) * b)"},
        {"!-a", "(!(-a))"},
        {"a + b + c", "((a + b) + c)"},
        {"a + b - c", "((a + b) - c)"},
        {"a * b * c", "((a * b) * c)"},
        {"a * b / c", "((a * b) / c)"},
        {"a + b / c", "(a + (b / c))"},
        {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
        {"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
        {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
        {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
        {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
        {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
        {"(5 + 5) * 2", "((5 + 5) * 2)"},
        {"2 / (5 + 5)", "(2 / (5 + 5))"},
        {"-(5 + 5)", "(-(5 + 5))"},
        {"!(true == true)", "(!(true == true))"},
        {"true", "true"},
        {"false", "false"},
        {"3 > 5 == false", "((3 > 5) == false)"},
        {"3 < 5 == true", "((3 < 5) == true)"},
    };

    for (const auto& test_case : test_cases) {
        auto lexer = Lexer {test_case.input};
        auto parser = Parser {lexer};
        const auto program = parser.parse_program();

        check_parser_errors(parser);
        REQUIRE(program.as_string() == test_case.expected);
    }
}
