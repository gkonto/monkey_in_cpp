#include <catch2/catch_test_macros.hpp>

#include "monkey/object.hpp"

TEST_CASE("IntegerObject exposes Integer type and inspect value", "[object]") {
    IntegerObject object {};
    object.value = 42;

    REQUIRE(object.type() == ObjectType::Integer);
    REQUIRE(to_string(object.type()) == "Integer");
    REQUIRE(object.inspect() == "42");
}

TEST_CASE("BooleanObject exposes Boolean type and inspect value", "[object]") {
    BooleanObject object {};
    object.value = true;

    REQUIRE(object.type() == ObjectType::Boolean);
    REQUIRE(to_string(object.type()) == "Boolean");
    REQUIRE(object.inspect() == "true");
}

TEST_CASE("NullObject exposes Null type and inspect value", "[object]") {
    NullObject object {};

    REQUIRE(object.type() == ObjectType::Null);
    REQUIRE(to_string(object.type()) == "Null");
    REQUIRE(object.inspect() == "null");
}

TEST_CASE("ReturnValueObject exposes ReturnValue type and inspect value", "[object]") {
    auto value = std::make_shared<IntegerObject>();
    value->value = 10;

    ReturnValueObject object {};
    object.value = value;

    REQUIRE(object.type() == ObjectType::ReturnValue);
    REQUIRE(to_string(object.type()) == "ReturnValue");
    REQUIRE(object.inspect() == "10");
}

TEST_CASE("ErrorObject exposes Error type and inspect value", "[object]") {
    ErrorObject object {};
    object.message = "type mismatch";

    REQUIRE(object.type() == ObjectType::Error);
    REQUIRE(to_string(object.type()) == "Error");
    REQUIRE(object.inspect() == "ERROR: type mismatch");
}
