#pragma once

#include <cstdint>
#include <memory>
#include <string>

enum class ObjectType {
    Integer,
    Boolean,
    Null,
    ReturnValue,
    Error,
};

[[nodiscard]] inline auto to_string(ObjectType type) -> std::string {
    switch (type) {
        case ObjectType::Integer:
            return "Integer";
        case ObjectType::Boolean:
            return "Boolean";
        case ObjectType::Null:
            return "Null";
        case ObjectType::ReturnValue:
            return "ReturnValue";
        case ObjectType::Error:
            return "Error";
    }

    return "Unknown";
}

struct Object {
    virtual ~Object() = default;

    [[nodiscard]] virtual auto type() const -> ObjectType = 0;
    [[nodiscard]] virtual auto inspect() const -> std::string = 0;
};

struct IntegerObject : Object {
    std::int64_t value {0};

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Integer;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return std::to_string(value);
    }
};

struct BooleanObject : Object {
    bool value {false};

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Boolean;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return value ? "true" : "false";
    }
};

struct NullObject : Object {
    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Null;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return "null";
    }
};

struct ReturnValueObject : Object {
    std::shared_ptr<Object> value;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::ReturnValue;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        if (value == nullptr) {
            return "null";
        }

        return value->inspect();
    }
};

struct ErrorObject : Object {
    std::string message;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Error;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return "ERROR: " + message;
    }
};
