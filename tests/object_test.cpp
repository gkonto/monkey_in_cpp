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
    const auto value = Value::make_integer(10);
    auto object = Object::make_return(value);

    REQUIRE(object.type() == ObjectType::ReturnValue);
    REQUIRE(to_string(object.type()) == "ReturnValue");
    REQUIRE(object.inspect() == "10");
    REQUIRE(object.return_value().type() == ObjectType::Integer);
    REQUIRE(object.return_value().integer_value() == 10);
}

TEST_CASE("ErrorObject exposes Error type and inspect value", "[object]") {
    auto object = Object::make_error("type mismatch");

    REQUIRE(object.type() == ObjectType::Error);
    REQUIRE(to_string(object.type()) == "Error");
    REQUIRE(object.inspect() == "ERROR: type mismatch");
    REQUIRE(object.error_message() == "type mismatch");
}

TEST_CASE("Value stores integers booleans and null inline", "[object]") {
    const auto integer = Value::make_integer(42);
    const auto boolean = Value::make_boolean(true);
    const auto null = Value::make_null();

    REQUIRE(integer.type() == ObjectType::Integer);
    REQUIRE(integer.is_immediate());
    REQUIRE_FALSE(integer.is_heap_object());
    REQUIRE(integer.integer_value() == 42);
    REQUIRE(integer.inspect() == "42");
    REQUIRE(integer.hash_key().has_value());

    REQUIRE(boolean.type() == ObjectType::Boolean);
    REQUIRE(boolean.is_immediate());
    REQUIRE_FALSE(boolean.is_heap_object());
    REQUIRE(boolean.boolean_value());
    REQUIRE(boolean.inspect() == "true");
    REQUIRE(boolean.hash_key().has_value());

    REQUIRE(null.type() == ObjectType::Null);
    REQUIRE(null.is_immediate());
    REQUIRE_FALSE(null.is_heap_object());
    REQUIRE(null.inspect() == "null");
    REQUIRE_FALSE(null.hash_key().has_value());
}

TEST_CASE("Value wraps heap-backed objects without copying", "[object]") {
    auto object = Object::make_string("hello");
    const auto value = Value::from_object(&object);

    REQUIRE(value.type() == ObjectType::String);
    REQUIRE_FALSE(value.is_immediate());
    REQUIRE(value.is_heap_object());
    REQUIRE(value.heap_object() == &object);
    REQUIRE(value.inspect() == "hello");
    REQUIRE(value.hash_key().has_value());
}

TEST_CASE("Value promotes scalar objects to immediates", "[object]") {
    auto integer_object = Object::make_integer(7);
    auto boolean_object = Object::make_boolean(false);
    auto null_object = Object::make_null();

    const auto integer = Value::from_object(&integer_object);
    const auto boolean = Value::from_object(&boolean_object);
    const auto null = Value::from_object(&null_object);

    REQUIRE(integer.is_immediate());
    REQUIRE(integer.integer_value() == 7);
    REQUIRE(integer.heap_object() == nullptr);

    REQUIRE(boolean.is_immediate());
    REQUIRE_FALSE(boolean.boolean_value());
    REQUIRE(boolean.heap_object() == nullptr);

    REQUIRE(null.is_immediate());
    REQUIRE(null.type() == ObjectType::Null);
    REQUIRE(null.heap_object() == nullptr);
}

TEST_CASE("DetachedValue keeps composite results alive after scratch arena reset", "[object]") {
    ScratchArena scratch;
    auto array = scratch.allocate_array(
        std::vector<Value> {
            Value::make_integer(1),
            scratch.allocate_string("two"),
        }
    );

    auto detached = detach(array);
    scratch.reset();

    REQUIRE(detached.type() == ObjectType::Array);
    REQUIRE(detached.value.heap_object() != nullptr);
    REQUIRE(detached.value.heap_object()->array_elements().size() == 2);
    REQUIRE(detached.value.heap_object()->array_elements()[0].type() == ObjectType::Integer);
    REQUIRE(detached.value.heap_object()->array_elements()[0].integer_value() == 1);
    REQUIRE(detached.value.heap_object()->array_elements()[1].type() == ObjectType::String);
    REQUIRE(detached.value.heap_object()->array_elements()[1].heap_object() != nullptr);
    REQUIRE(
        detached.value.heap_object()->array_elements()[1].heap_object()->string_value() == "two"
    );
}
