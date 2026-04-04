#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "monkey/ast.hpp"

class Environment;

enum class ObjectType {
    Integer,
    Boolean,
    String,
    Array,
    Hash,
    Null,
    ReturnValue,
    Error,
    Function,
    Builtin,
};

[[nodiscard]] inline auto to_string(ObjectType type) -> std::string {
    switch (type) {
        case ObjectType::Integer:
            return "Integer";
        case ObjectType::Boolean:
            return "Boolean";
        case ObjectType::String:
            return "String";
        case ObjectType::Array:
            return "Array";
        case ObjectType::Hash:
            return "Hash";
        case ObjectType::Null:
            return "Null";
        case ObjectType::ReturnValue:
            return "ReturnValue";
        case ObjectType::Error:
            return "Error";
        case ObjectType::Function:
            return "Function";
        case ObjectType::Builtin:
            return "Builtin";
    }

    return "Unknown";
}

struct HashKey {
    ObjectType type {ObjectType::Null};
    std::size_t value {0};

    [[nodiscard]] auto operator==(const HashKey& other) const -> bool = default;
};

struct HashKeyHasher {
    [[nodiscard]] auto operator()(const HashKey& key) const -> std::size_t {
        return std::hash<int> {}(static_cast<int>(key.type)) ^ (key.value << 1U);
    }
};

struct Object;

struct HashPair {
    std::shared_ptr<Object> key;
    std::shared_ptr<Object> value;
};

struct Object {
    virtual ~Object() = default;

    [[nodiscard]] virtual auto type() const -> ObjectType = 0;
    [[nodiscard]] virtual auto inspect() const -> std::string = 0;
    [[nodiscard]] virtual auto hash_key() const -> std::optional<HashKey> {
        return std::nullopt;
    }
};

struct IntegerObject : Object {
    std::int64_t value {0};

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Integer;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return std::to_string(value);
    }

    [[nodiscard]] auto hash_key() const -> std::optional<HashKey> override {
        return HashKey {ObjectType::Integer, std::hash<std::int64_t> {}(value)};
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

    [[nodiscard]] auto hash_key() const -> std::optional<HashKey> override {
        return HashKey {ObjectType::Boolean, std::hash<bool> {}(value)};
    }
};

struct StringObject : Object {
    std::string value;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::String;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return value;
    }

    [[nodiscard]] auto hash_key() const -> std::optional<HashKey> override {
        return HashKey {ObjectType::String, std::hash<std::string> {}(value)};
    }
};

struct ArrayObject : Object {
    std::vector<std::shared_ptr<Object>> elements;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Array;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        std::string out = "[";

        for (std::size_t index = 0; index < elements.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (elements[index] != nullptr) {
                out += elements[index]->inspect();
            } else {
                out += "null";
            }
        }

        out += "]";
        return out;
    }
};

struct HashObject : Object {
    std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Hash;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        std::string out = "{";
        bool first = true;

        for (const auto& [_, pair] : pairs) {
            if (!first) {
                out += ", ";
            }

            first = false;
            out += pair.key != nullptr ? pair.key->inspect() : "null";
            out += ": ";
            out += pair.value != nullptr ? pair.value->inspect() : "null";
        }

        out += "}";
        return out;
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

struct FunctionObject : Object {
    std::vector<std::unique_ptr<Identifier>> parameters;
    std::unique_ptr<BlockStatement> body;
    std::shared_ptr<Environment> env;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Function;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        std::string out = "fn(";

        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (parameters[index] != nullptr) {
                out += parameters[index]->as_string();
            }
        }

        out += ") {\n";

        if (body != nullptr) {
            out += body->as_string();
        }

        out += "\n}";
        return out;
    }
};

struct BuiltinObject : Object {
    using BuiltinFunction = std::function<std::shared_ptr<Object>(
        const std::vector<std::shared_ptr<Object>>&
    )>;

    BuiltinFunction function;

    [[nodiscard]] auto type() const -> ObjectType override {
        return ObjectType::Builtin;
    }

    [[nodiscard]] auto inspect() const -> std::string override {
        return "builtin function";
    }
};
