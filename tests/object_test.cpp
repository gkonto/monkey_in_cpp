#include <catch2/catch_test_macros.hpp>

#include "monkey/object.hpp"

TEST_CASE("IntegerObject exposes Integer type and inspect value", "[object]") {
    auto object = Object::make_integer(42);

    REQUIRE(object.type() == ObjectType::Integer);
    REQUIRE(to_string(object.type()) == "Integer");
    REQUIRE(object.inspect() == "42");
    REQUIRE(object.integer_value() == 42);
}

TEST_CASE("BooleanObject exposes Boolean type and inspect value", "[object]") {
    auto object = Object::make_boolean(true);

    REQUIRE(object.type() == ObjectType::Boolean);
    REQUIRE(to_string(object.type()) == "Boolean");
    REQUIRE(object.inspect() == "true");
    REQUIRE(object.boolean_value());
}

TEST_CASE("StringObject exposes String type and inspect value", "[object]") {
    auto object = Object::make_string("hello");

    REQUIRE(object.type() == ObjectType::String);
    REQUIRE(to_string(object.type()) == "String");
    REQUIRE(object.inspect() == "hello");
    REQUIRE(object.string_value() == "hello");
}

TEST_CASE("TestStringHashKey", "[object]") {
    auto hello1 = Object::make_string("Hello World");
    auto hello2 = Object::make_string("Hello World");
    auto diff1 = Object::make_string("My name is johnny");
    auto diff2 = Object::make_string("My name is johnny");

    REQUIRE(hello1.hash_key().has_value());
    REQUIRE(hello2.hash_key().has_value());
    REQUIRE(diff1.hash_key().has_value());
    REQUIRE(diff2.hash_key().has_value());

    REQUIRE(*hello1.hash_key() == *hello2.hash_key());
    REQUIRE(*diff1.hash_key() == *diff2.hash_key());
    REQUIRE(*hello1.hash_key() != *diff1.hash_key());
}

TEST_CASE("NullObject exposes Null type and inspect value", "[object]") {
    auto object = Object::make_null();

    REQUIRE(object.type() == ObjectType::Null);
    REQUIRE(to_string(object.type()) == "Null");
    REQUIRE(object.inspect() == "null");
}

TEST_CASE("ReturnValueObject exposes ReturnValue type and inspect value", "[object]") {
    auto value = std::make_shared<Object>(Object::make_integer(10));
    auto object = Object::make_return(value);

    REQUIRE(object.type() == ObjectType::ReturnValue);
    REQUIRE(to_string(object.type()) == "ReturnValue");
    REQUIRE(object.inspect() == "10");
    REQUIRE(object.return_value() == value);
}

TEST_CASE("ErrorObject exposes Error type and inspect value", "[object]") {
    auto object = Object::make_error("type mismatch");

    REQUIRE(object.type() == ObjectType::Error);
    REQUIRE(to_string(object.type()) == "Error");
    REQUIRE(object.inspect() == "ERROR: type mismatch");
    REQUIRE(object.error_message() == "type mismatch");
}
