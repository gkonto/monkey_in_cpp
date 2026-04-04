#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
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

void test_integer_array_object(
    const Object* object,
    const std::vector<std::int64_t>& expected_elements
) {
    REQUIRE(object != nullptr);

    const auto* array = dynamic_cast<const ArrayObject*>(object);
    REQUIRE(array != nullptr);
    REQUIRE(array->type() == ObjectType::Array);
    REQUIRE(array->elements.size() == expected_elements.size());

    for (std::size_t index = 0; index < expected_elements.size(); ++index) {
        test_integer_object(array->elements[index].get(), expected_elements[index]);
    }
}

auto hash_key_for_test(const Object* object) -> std::optional<HashKey> {
    if (const auto* integer = dynamic_cast<const IntegerObject*>(object); integer != nullptr) {
        return HashKey {ObjectType::Integer, std::hash<std::int64_t> {}(integer->value)};
    }

    if (const auto* boolean = dynamic_cast<const BooleanObject*>(object); boolean != nullptr) {
        return HashKey {ObjectType::Boolean, std::hash<bool> {}(boolean->value)};
    }

    if (const auto* string = dynamic_cast<const StringObject*>(object); string != nullptr) {
        return HashKey {ObjectType::String, std::hash<std::string> {}(string->value)};
    }

    return std::nullopt;
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

    const TestCase test_cases[] = {
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

    const TestCase test_cases[] = {
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

    const TestCase test_cases[] = {
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
        {"{\"name\": \"Monkey\"}[fn(x) { x }];", "unusable as hash key: Function"},
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

TEST_CASE("TestBuiltinFunctions", "[evaluator]") {
    using Expected = std::variant<
        std::int64_t,
        std::string_view,
        std::monostate,
        std::vector<std::int64_t>
    >;

    struct TestCase {
        std::string_view input;
        Expected expected;
    };

    const TestCase test_cases[] = {
        {"len(\"\")", std::int64_t {0}},
        {"len(\"four\")", std::int64_t {4}},
        {"len(\"hello world\")", std::int64_t {11}},
        {"len([1, 2, 3])", std::int64_t {3}},
        {"len(1)", std::string_view {"argument to `len` not supported, got Integer"}},
        {
            "len(\"one\", \"two\")",
            std::string_view {"wrong number of arguments. got=2, want=1"},
        },
        {"first([1, 2, 3])", std::int64_t {1}},
        {"first([])", std::monostate {}},
        {"first(1)", std::string_view {"argument to `first` must be Array, got Integer"}},
        {"last([1, 2, 3])", std::int64_t {3}},
        {"last([])", std::monostate {}},
        {"last(1)", std::string_view {"argument to `last` must be Array, got Integer"}},
        {"rest([1, 2, 3])", std::vector<std::int64_t> {2, 3}},
        {"rest([])", std::monostate {}},
        {"rest(1)", std::string_view {"argument to `rest` must be Array, got Integer"}},
        {"push([], 1)", std::vector<std::int64_t> {1}},
        {"push([1, 2], 3)", std::vector<std::int64_t> {1, 2, 3}},
        {"push(1, 1)", std::string_view {"argument to `push` must be Array, got Integer"}},
        {
            "push([1])",
            std::string_view {"wrong number of arguments. got=1, want=2"},
        },
    };

    for (const auto& test_case : test_cases) {
        const auto evaluated = test_eval(test_case.input);

        std::visit([&](const auto& expected) {
            using ExpectedType = std::decay_t<decltype(expected)>;

            if constexpr (std::is_same_v<ExpectedType, std::int64_t>) {
                test_integer_object(evaluated.get(), expected);
            } else if constexpr (std::is_same_v<ExpectedType, std::monostate>) {
                test_null_object(evaluated.get());
            } else if constexpr (std::is_same_v<ExpectedType, std::vector<std::int64_t>>) {
                test_integer_array_object(evaluated.get(), expected);
            } else {
                test_error_object(evaluated.get(), expected);
            }
        }, test_case.expected);
    }
}

TEST_CASE("TestArrayLiterals", "[evaluator]") {
    constexpr auto input = "[1, 2 * 2, 3 + 3]";

    const auto evaluated = test_eval(input);
    REQUIRE(evaluated != nullptr);

    const auto* array = dynamic_cast<const ArrayObject*>(evaluated.get());
    REQUIRE(array != nullptr);
    REQUIRE(array->type() == ObjectType::Array);
    REQUIRE(array->elements.size() == 3);

    test_integer_object(array->elements[0].get(), 1);
    test_integer_object(array->elements[1].get(), 4);
    test_integer_object(array->elements[2].get(), 6);
}

TEST_CASE("TestArrayIndexExpressions", "[evaluator]") {
    using Expected = std::variant<std::int64_t, std::monostate>;

    struct TestCase {
        std::string_view input;
        Expected expected;
    };

    constexpr TestCase test_cases[] = {
        {"[1, 2, 3][0]", std::int64_t {1}},
        {"[1, 2, 3][1]", std::int64_t {2}},
        {"[1, 2, 3][2]", std::int64_t {3}},
        {"let i = 0; [1][i];", std::int64_t {1}},
        {"[1, 2, 3][1+1];", std::int64_t {3}},
        {"let myArray = [1, 2, 3]; myArray[2];", std::int64_t {3}},
        {"let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];", std::int64_t {6}},
        {"let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i];", std::int64_t {2}},
        {"[1, 2, 3][3]", std::monostate {}},
        {"[1, 2, 3][-1]", std::monostate {}},
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

TEST_CASE("TestHashLiterals", "[evaluator]") {
    constexpr auto input = "{\"one\": 10 - 9, \"two\": 1 + 1, \"three\": 6 / 2, 4: 4, true: 5, false: 6}";

    const auto evaluated = test_eval(input);
    REQUIRE(evaluated != nullptr);

    const auto* hash = dynamic_cast<const HashObject*>(evaluated.get());
    REQUIRE(hash != nullptr);
    REQUIRE(hash->type() == ObjectType::Hash);
    REQUIRE(hash->pairs.size() == 6);

    auto one = std::make_shared<StringObject>();
    one->value = "one";
    auto two = std::make_shared<StringObject>();
    two->value = "two";
    auto three = std::make_shared<StringObject>();
    three->value = "three";
    auto four = std::make_shared<IntegerObject>();
    four->value = 4;
    auto true_value = std::make_shared<BooleanObject>();
    true_value->value = true;
    auto false_value = std::make_shared<BooleanObject>();
    false_value->value = false;

    const auto one_iterator = hash->pairs.find(*hash_key_for_test(one.get()));
    const auto two_iterator = hash->pairs.find(*hash_key_for_test(two.get()));
    const auto three_iterator = hash->pairs.find(*hash_key_for_test(three.get()));
    const auto four_iterator = hash->pairs.find(*hash_key_for_test(four.get()));
    const auto true_iterator = hash->pairs.find(*hash_key_for_test(true_value.get()));
    const auto false_iterator = hash->pairs.find(*hash_key_for_test(false_value.get()));

    REQUIRE(one_iterator != hash->pairs.end());
    REQUIRE(two_iterator != hash->pairs.end());
    REQUIRE(three_iterator != hash->pairs.end());
    REQUIRE(four_iterator != hash->pairs.end());
    REQUIRE(true_iterator != hash->pairs.end());
    REQUIRE(false_iterator != hash->pairs.end());

    test_integer_object(one_iterator->second.value.get(), 1);
    test_integer_object(two_iterator->second.value.get(), 2);
    test_integer_object(three_iterator->second.value.get(), 3);
    test_integer_object(four_iterator->second.value.get(), 4);
    test_integer_object(true_iterator->second.value.get(), 5);
    test_integer_object(false_iterator->second.value.get(), 6);
}

TEST_CASE("TestHashIndexExpressions", "[evaluator]") {
    using Expected = std::variant<std::int64_t, std::monostate>;

    struct TestCase {
        std::string_view input;
        Expected expected;
    };

    const TestCase test_cases[] = {
        {"{\"foo\": 5}[\"foo\"]", std::int64_t {5}},
        {"{\"foo\": 5}[\"bar\"]", std::monostate {}},
        {"let key = \"foo\"; {\"foo\": 5}[key]", std::int64_t {5}},
        {"{}[\"foo\"]", std::monostate {}},
        {"{5: 5}[5]", std::int64_t {5}},
        {"{true: 5}[true]", std::int64_t {5}},
        {"{false: 5}[false]", std::int64_t {5}},
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
