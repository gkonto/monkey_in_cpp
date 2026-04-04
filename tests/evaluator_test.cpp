#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

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

void test_null_object(const Object* object) {
    REQUIRE(object != nullptr);

    const auto* null = dynamic_cast<const NullObject*>(object);
    REQUIRE(null != nullptr);
    REQUIRE(null->type() == ObjectType::Null);
    REQUIRE(null->inspect() == "null");
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
