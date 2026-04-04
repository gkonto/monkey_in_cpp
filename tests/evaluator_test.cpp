#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>
#include <variant>

#include "monkey/evaluator.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

auto test_eval(std::string_view input) -> std::shared_ptr<Object> {
    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    REQUIRE(parser.errors().empty());
    return eval(&program);
}

void test_integer_object(const Object* object, std::int64_t expected_value) {
    REQUIRE(object != nullptr);

    const auto* integer = dynamic_cast<const IntegerObject*>(object);
    REQUIRE(integer != nullptr);
    REQUIRE(integer->value == expected_value);
    REQUIRE(integer->type() == ObjectType::Integer);
}

void test_boolean_object(const Object* object, bool expected_value) {
    REQUIRE(object != nullptr);

    const auto* boolean = dynamic_cast<const BooleanObject*>(object);
    REQUIRE(boolean != nullptr);
    REQUIRE(boolean->value == expected_value);
    REQUIRE(boolean->type() == ObjectType::Boolean);
}

void test_string_object(const Object* object, std::string_view expected_value) {
    REQUIRE(object != nullptr);

    const auto* string = dynamic_cast<const StringObject*>(object);
    REQUIRE(string != nullptr);
    REQUIRE(string->value == expected_value);
    REQUIRE(string->type() == ObjectType::String);
}

void test_null_object(const Object* object) {
    REQUIRE(object != nullptr);

    const auto* null = dynamic_cast<const NullObject*>(object);
    REQUIRE(null != nullptr);
    REQUIRE(null->type() == ObjectType::Null);
    REQUIRE(null->inspect() == "null");
}

void test_error_object(const Object* object, std::string_view expected_message) {
    REQUIRE(object != nullptr);

    const auto* error = dynamic_cast<const ErrorObject*>(object);
    REQUIRE(error != nullptr);
    REQUIRE(error->message == expected_message);
    REQUIRE(error->type() == ObjectType::Error);
}

}  // namespace

TEST_CASE("TestEvalIntegerExpression", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        std::int64_t expected;
    };

    constexpr TestCase test_cases[] = {
        {"5", 5},
        {"10", 10},
        {"-5", -5},
        {"-10", -10},
        {"5 + 5 + 5 + 5 - 10", 10},
        {"2 * 2 * 2 * 2 * 2", 32},
        {"-50 + 100 + -50", 0},
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"20 + 2 * -10", 0},
        {"50 / 2 * 2 + 10", 60},
        {"2 * (5 + 10)", 30},
        {"3 * 3 * 3 + 10", 37},
        {"3 * (3 * 3) + 10", 37},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_integer_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestEvalBooleanExpression", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        bool expected;
    };

    constexpr TestCase test_cases[] = {
        {"true", true},
        {"false", false},
        {"1 < 2", true},
        {"1 > 2", false},
        {"1 < 1", false},
        {"1 > 1", false},
        {"1 == 1", true},
        {"1 != 1", false},
        {"1 == 2", false},
        {"1 != 2", true},
        {"true == true", true},
        {"false == false", true},
        {"true != false", true},
        {"false != true", true},
        {"(1 < 2) == true", true},
        {"(1 < 2) == false", false},
        {"(1 > 2) == true", false},
        {"(1 > 2) == false", true},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_boolean_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestEvalNullExpression", "[evaluator]") {
    const auto evaluated = test_eval("");
    test_null_object(evaluated.get());
}

TEST_CASE("TestBangOperator", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        bool expected;
    };

    constexpr TestCase test_cases[] = {
        {"!true", false},
        {"!false", true},
        {"!5", false},
        {"!!true", true},
        {"!!false", false},
        {"!!5", true},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_boolean_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestIfElseExpressions", "[evaluator]") {
    using Expected = std::variant<std::int64_t, std::monostate>;

    struct TestCase {
        std::string_view input;
        Expected expected;
    };

    constexpr TestCase test_cases[] = {
        {"if (true) { 10 }", std::int64_t {10}},
        {"if (false) { 10 }", std::monostate {}},
        {"if (1) { 10 }", std::int64_t {10}},
        {"if (1 < 2) { 10 }", std::int64_t {10}},
        {"if (1 > 2) { 10 }", std::monostate {}},
        {"if (1 > 2) { 10 } else { 20 }", std::int64_t {20}},
        {"if (1 < 2) { 10 } else { 20 }", std::int64_t {10}},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);

        std::visit([&](const auto& expected) {
            using ExpectedType = std::decay_t<decltype(expected)>;

            if constexpr (std::is_same_v<ExpectedType, std::int64_t>) {
                test_integer_object(evaluated.get(), expected);
            } else {
                test_null_object(evaluated.get());
            }
        }, test_case.expected);
    }
}

TEST_CASE("TestReturnStatements", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        std::int64_t expected;
    };

    constexpr TestCase test_cases[] = {
        {"return 10;", 10},
        {"return 10; 9;", 10},
        {"return 2 * 5; 9;", 10},
        {"9; return 2 * 5; 9;", 10},
        {"if (10 > 1) { if (10 > 1) { return 10; } return 1; }", 10},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_integer_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestErrorHandling", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        std::string_view expected_message;
    };

    constexpr TestCase test_cases[] = {
        {"5 + true;", "type mismatch: Integer + Boolean"},
        {"5 + true; 5;", "type mismatch: Integer + Boolean"},
        {"-true", "unknown operator: -Boolean"},
        {"true + false;", "unknown operator: Boolean + Boolean"},
        {"\"Hello\" - \"World\"", "unknown operator: String - String"},
        {"5; true + false; 5", "unknown operator: Boolean + Boolean"},
        {"if (10 > 1) { true + false; }", "unknown operator: Boolean + Boolean"},
        {
            "if (10 > 1) { if (10 > 1) { return true + false; } return 1; }",
            "unknown operator: Boolean + Boolean",
        },
        {"foobar", "identifier not found: foobar"},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_error_object(evaluated.get(), test_case.expected_message);
    }
}

TEST_CASE("TestLetStatements", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        std::int64_t expected;
    };

    constexpr TestCase test_cases[] = {
        {"let a = 5; a;", 5},
        {"let a = 5 * 5; a;", 25},
        {"let a = 5; let b = a; b;", 5},
        {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_integer_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestFunctionObject", "[evaluator]") {
    constexpr auto input = "fn(x) { x + 2; };";

    const auto evaluated = test_eval(input);
    REQUIRE(evaluated != nullptr);

    const auto* function = dynamic_cast<const FunctionObject*>(evaluated.get());
    REQUIRE(function != nullptr);
    REQUIRE(function->parameters.size() == 1);
    REQUIRE(function->parameters[0] != nullptr);
    REQUIRE(function->parameters[0]->as_string() == "x");
    REQUIRE(function->body != nullptr);
    REQUIRE(function->body->as_string() == "(x + 2)");
}

TEST_CASE("TestFunctionApplication", "[evaluator]") {
    struct TestCase {
        std::string_view input;
        std::int64_t expected;
    };

    constexpr TestCase test_cases[] = {
        {"let identity = fn(x) { x; }; identity(5);", 5},
        {"let identity = fn(x) { return x; }; identity(5);", 5},
        {"let doubleFn = fn(x) { x * 2; }; doubleFn(5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        {"fn(x) { x; }(5)", 5},
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);
        test_integer_object(evaluated.get(), test_case.expected);
    }
}

TEST_CASE("TestClosures", "[evaluator]") {
    constexpr auto input =
        "let newAdder = fn(x) { fn(y) { x + y; }; };\n"
        "let addTwo = newAdder(2);\n"
        "addTwo(2);";

    const auto evaluated = test_eval(input);
    test_integer_object(evaluated.get(), 4);
}

TEST_CASE("TestStringLiteral", "[evaluator]") {
    constexpr auto input = "\"hello world\"";

    const auto evaluated = test_eval(input);
    test_string_object(evaluated.get(), "hello world");
}

TEST_CASE("TestStringConcatenation", "[evaluator]") {
    constexpr auto input = "\"Hello\" + \" \" + \"World!\"";

    const auto evaluated = test_eval(input);
    test_string_object(evaluated.get(), "Hello World!");
}
