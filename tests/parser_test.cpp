#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <variant>

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

template <typename T>
void test_literal_expression(const Expression* expression, const T& expected_value);

void check_parser_errors(const Parser& parser) {
    const auto& errors = parser.errors();

    REQUIRE(errors.empty());
}

template <typename T>
void test_let_statement(const Statement* statement, std::string_view expected_name, const T& expected_value) {
    REQUIRE(statement != nullptr);

    const auto* let_statement = dynamic_cast<const LetStatement*>(statement);
    REQUIRE(let_statement != nullptr);
    REQUIRE(let_statement->token_literal() == "let");
    REQUIRE(let_statement->name.literal == expected_name);
    REQUIRE(let_statement->value != nullptr);
    test_literal_expression(let_statement->value.get(), expected_value);
}

template <typename T>
void test_return_statement(const Statement* statement, const T& expected_value) {
    REQUIRE(statement != nullptr);

    const auto* return_statement = dynamic_cast<const ReturnStatement*>(statement);
    REQUIRE(return_statement != nullptr);
    REQUIRE(return_statement->token_literal() == "return");
    REQUIRE(return_statement->return_value != nullptr);
    test_literal_expression(return_statement->return_value.get(), expected_value);
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

void test_string_literal(const Expression* expression, std::string_view expected_value) {
    REQUIRE(expression != nullptr);

    const auto* string_literal = dynamic_cast<const StringLiteral*>(expression);
    REQUIRE(string_literal != nullptr);
    REQUIRE(string_literal->value == expected_value);
    REQUIRE(string_literal->token_literal() == expected_value);
}

template <typename T>
void test_literal_expression(const Expression* expression, const T& expected_value) {
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, std::string_view>) {
        if (dynamic_cast<const Identifier*>(expression) != nullptr) {
            test_identifier(expression, expected_value);
        } else {
            test_string_literal(expression, expected_value);
        }
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

    struct TestCase {
        std::string_view name;
        std::int64_t value;
    };

    constexpr TestCase test_cases[] = {
        {"x", 5},
        {"y", 10},
        {"foobar", 838383},
    };

    for (std::size_t index = 0; index < std::size(test_cases); ++index) {
        test_let_statement(program.statements[index].get(), test_cases[index].name, test_cases[index].value);
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

    constexpr std::int64_t expected_values[] = {5, 10, 993322};

    for (std::size_t index = 0; index < std::size(expected_values); ++index) {
        test_return_statement(program.statements[index].get(), expected_values[index]);
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

TEST_CASE("TestStringLiteralExpression", "[parser]") {
    constexpr auto input = "\"hello world\";";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    test_string_literal(statement->expression.get(), "hello world");
}

TEST_CASE("TestParsingArrayLiterals", "[parser]") {
    constexpr auto input = "[1, 2 * 2, 3 + 3]";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* array = dynamic_cast<const ArrayLiteral*>(statement->expression.get());
    REQUIRE(array != nullptr);
    REQUIRE(array->elements.size() == 3);

    test_integer_literal(array->elements[0].get(), 1);

    const auto* second = dynamic_cast<const InfixExpression*>(array->elements[1].get());
    REQUIRE(second != nullptr);
    test_literal_expression(second->left.get(), std::int64_t {2});
    REQUIRE(second->op == "*");
    test_literal_expression(second->right.get(), std::int64_t {2});

    const auto* third = dynamic_cast<const InfixExpression*>(array->elements[2].get());
    REQUIRE(third != nullptr);
    test_literal_expression(third->left.get(), std::int64_t {3});
    REQUIRE(third->op == "+");
    test_literal_expression(third->right.get(), std::int64_t {3});
}

TEST_CASE("TestParsingHashLiterals", "[parser]") {
    constexpr auto input = "{\"one\": 1, \"two\": 2, \"three\": 3}";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* hash = dynamic_cast<const HashLiteral*>(statement->expression.get());
    REQUIRE(hash != nullptr);
    REQUIRE(hash->pairs.size() == 3);

    test_string_literal(hash->pairs[0].first.get(), "one");
    test_integer_literal(hash->pairs[0].second.get(), 1);
    test_string_literal(hash->pairs[1].first.get(), "two");
    test_integer_literal(hash->pairs[1].second.get(), 2);
    test_string_literal(hash->pairs[2].first.get(), "three");
    test_integer_literal(hash->pairs[2].second.get(), 3);
}

TEST_CASE("TestParsingEmptyHashLiteral", "[parser]") {
    constexpr auto input = "{}";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* hash = dynamic_cast<const HashLiteral*>(statement->expression.get());
    REQUIRE(hash != nullptr);
    REQUIRE(hash->pairs.empty());
}

TEST_CASE("TestParsingHashLiteralsWithExpressions", "[parser]") {
    constexpr auto input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* hash = dynamic_cast<const HashLiteral*>(statement->expression.get());
    REQUIRE(hash != nullptr);
    REQUIRE(hash->pairs.size() == 3);

    test_string_literal(hash->pairs[0].first.get(), "one");
    const auto* one_value = dynamic_cast<const InfixExpression*>(hash->pairs[0].second.get());
    REQUIRE(one_value != nullptr);
    test_integer_literal(one_value->left.get(), 0);
    REQUIRE(one_value->op == "+");
    test_integer_literal(one_value->right.get(), 1);

    test_string_literal(hash->pairs[1].first.get(), "two");
    const auto* two_value = dynamic_cast<const InfixExpression*>(hash->pairs[1].second.get());
    REQUIRE(two_value != nullptr);
    test_integer_literal(two_value->left.get(), 10);
    REQUIRE(two_value->op == "-");
    test_integer_literal(two_value->right.get(), 8);

    test_string_literal(hash->pairs[2].first.get(), "three");
    const auto* three_value = dynamic_cast<const InfixExpression*>(hash->pairs[2].second.get());
    REQUIRE(three_value != nullptr);
    test_integer_literal(three_value->left.get(), 15);
    REQUIRE(three_value->op == "/");
    test_integer_literal(three_value->right.get(), 5);
}

TEST_CASE("TestParsingIndexExpressions", "[parser]") {
    constexpr auto input = "myArray[1 + 1]";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* index_expression = dynamic_cast<const IndexExpression*>(statement->expression.get());
    REQUIRE(index_expression != nullptr);
    REQUIRE(index_expression->left != nullptr);
    REQUIRE(index_expression->index != nullptr);

    test_literal_expression(index_expression->left.get(), std::string_view {"myArray"});

    const auto* index = dynamic_cast<const InfixExpression*>(index_expression->index.get());
    REQUIRE(index != nullptr);
    REQUIRE(index->op == "+");
    test_literal_expression(index->left.get(), std::int64_t {1});
    test_literal_expression(index->right.get(), std::int64_t {1});
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
        {"a + add(b * c) + d", "((a + add((b * c))) + d)"},
        {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))", "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
        {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"},
        {"a * [1, 2, 3, 4][b * c] * d", "((a * ([1, 2, 3, 4][(b * c)])) * d)"},
    };

    for (const auto& test_case : test_cases) {
        auto lexer = Lexer {test_case.input};
        auto parser = Parser {lexer};
        const auto program = parser.parse_program();

        check_parser_errors(parser);
        REQUIRE(program.as_string() == test_case.expected);
    }
}

TEST_CASE("TestIfExpression", "[parser]") {
    constexpr auto input = "if (x < y) { x }";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* if_expression = dynamic_cast<const IfExpression*>(statement->expression.get());
    REQUIRE(if_expression != nullptr);
    REQUIRE(if_expression->condition != nullptr);

    const auto* condition = dynamic_cast<const InfixExpression*>(if_expression->condition.get());
    REQUIRE(condition != nullptr);
    REQUIRE(condition->op == "<");
    REQUIRE(condition->left != nullptr);
    REQUIRE(condition->right != nullptr);
    test_literal_expression(condition->left.get(), std::string_view {"x"});
    test_literal_expression(condition->right.get(), std::string_view {"y"});

    REQUIRE(if_expression->consequence != nullptr);
    REQUIRE(if_expression->consequence->statements.size() == 1);

    const auto* consequence_statement =
        dynamic_cast<const ExpressionStatement*>(if_expression->consequence->statements[0].get());
    REQUIRE(consequence_statement != nullptr);
    REQUIRE(consequence_statement->expression != nullptr);
    test_literal_expression(consequence_statement->expression.get(), std::string_view {"x"});

    REQUIRE(if_expression->alternative == nullptr);
}

TEST_CASE("TestFunctionLiteral", "[parser]") {
    constexpr auto input = "fn(x, y) { x + y; }";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* function = dynamic_cast<const FunctionLiteral*>(statement->expression.get());
    REQUIRE(function != nullptr);
    REQUIRE(function->parameters.size() == 2);
    test_literal_expression(function->parameters[0].get(), std::string_view {"x"});
    test_literal_expression(function->parameters[1].get(), std::string_view {"y"});

    REQUIRE(function->body != nullptr);
    REQUIRE(function->body->statements.size() == 1);

    const auto* body_statement =
        dynamic_cast<const ExpressionStatement*>(function->body->statements[0].get());
    REQUIRE(body_statement != nullptr);
    REQUIRE(body_statement->expression != nullptr);

    const auto* body_expression = dynamic_cast<const InfixExpression*>(body_statement->expression.get());
    REQUIRE(body_expression != nullptr);
    REQUIRE(body_expression->op == "+");
    test_literal_expression(body_expression->left.get(), std::string_view {"x"});
    test_literal_expression(body_expression->right.get(), std::string_view {"y"});
}

TEST_CASE("TestFunctionParameterParsing", "[parser]") {
    struct TestCase {
        std::string_view input;
        std::vector<std::string_view> expected_parameters;
    };

    const TestCase test_cases[] = {
        {"fn() {};", {}},
        {"fn(x) {};", {"x"}},
        {"fn(x, y, z) {};", {"x", "y", "z"}},
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

        const auto* function = dynamic_cast<const FunctionLiteral*>(statement->expression.get());
        REQUIRE(function != nullptr);
        REQUIRE(function->parameters.size() == test_case.expected_parameters.size());

        for (std::size_t index = 0; index < test_case.expected_parameters.size(); ++index) {
            test_literal_expression(function->parameters[index].get(), test_case.expected_parameters[index]);
        }
    }
}

TEST_CASE("TestCallExpressionParsing", "[parser]") {
    constexpr auto input = "add(1, 2 * 3, 4 + 5);";

    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    check_parser_errors(parser);
    REQUIRE(program.statements.size() == 1);

    const auto* statement = dynamic_cast<const ExpressionStatement*>(program.statements[0].get());
    REQUIRE(statement != nullptr);
    REQUIRE(statement->expression != nullptr);

    const auto* call = dynamic_cast<const CallExpression*>(statement->expression.get());
    REQUIRE(call != nullptr);
    REQUIRE(call->function != nullptr);
    test_literal_expression(call->function.get(), std::string_view {"add"});

    REQUIRE(call->arguments.size() == 3);
    test_literal_expression(call->arguments[0].get(), std::int64_t {1});

    const auto* second_argument = dynamic_cast<const InfixExpression*>(call->arguments[1].get());
    REQUIRE(second_argument != nullptr);
    REQUIRE(second_argument->op == "*");
    test_literal_expression(second_argument->left.get(), std::int64_t {2});
    test_literal_expression(second_argument->right.get(), std::int64_t {3});

    const auto* third_argument = dynamic_cast<const InfixExpression*>(call->arguments[2].get());
    REQUIRE(third_argument != nullptr);
    REQUIRE(third_argument->op == "+");
    test_literal_expression(third_argument->left.get(), std::int64_t {4});
    test_literal_expression(third_argument->right.get(), std::int64_t {5});
}
