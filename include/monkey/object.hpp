#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
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

struct StringData {
    std::string value;
};

struct ArrayData {
    std::vector<std::shared_ptr<Object>> elements;
};

struct HashData {
    std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;
};

struct ReturnValueData {
    std::shared_ptr<Object> value;
};

struct ErrorData {
    std::string message;
};

struct FunctionData {
    std::vector<std::unique_ptr<Identifier>> parameters;
    std::unique_ptr<BlockStatement> body;
    std::shared_ptr<Environment> env;
};

using BuiltinFunction = std::function<std::shared_ptr<Object>(
    const std::vector<std::shared_ptr<Object>>&
)>;

struct BuiltinData {
    BuiltinFunction function;
};

union ObjectData {
    std::int64_t integer;
    bool boolean;
    void* pointer;

    constexpr ObjectData() : pointer(nullptr) {}
};

struct Object {
    ObjectType object_type {ObjectType::Null};
    ObjectData data {};

    Object() = default;

    ~Object() {
        destroy();
    }

    Object(const Object&) = delete;
    auto operator=(const Object&) -> Object& = delete;

    Object(Object&& other) noexcept {
        move_from(std::move(other));
    }

    auto operator=(Object&& other) noexcept -> Object& {
        if (this != &other) {
            destroy();
            move_from(std::move(other));
        }

        return *this;
    }

    [[nodiscard]] auto type() const -> ObjectType {
        return object_type;
    }

    [[nodiscard]] auto inspect() const -> std::string {
        switch (object_type) {
            case ObjectType::Integer:
                return std::to_string(integer_value());
            case ObjectType::Boolean:
                return boolean_value() ? "true" : "false";
            case ObjectType::String:
                return string_value();
            case ObjectType::Array: {
                std::string out = "[";
                const auto& elements = array_elements();

                for (std::size_t index = 0; index < elements.size(); ++index) {
                    if (index > 0) {
                        out += ", ";
                    }

                    out += elements[index] != nullptr ? elements[index]->inspect() : "null";
                }

                out += "]";
                return out;
            }
            case ObjectType::Hash: {
                std::string out = "{";
                bool first = true;

                for (const auto& [_, pair] : hash_pairs()) {
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
            case ObjectType::Null:
                return "null";
            case ObjectType::ReturnValue:
                return return_value() != nullptr ? return_value()->inspect() : "null";
            case ObjectType::Error:
                return "ERROR: " + error_message();
            case ObjectType::Function: {
                std::string out = "fn(";
                const auto& params = function_parameters();

                for (std::size_t index = 0; index < params.size(); ++index) {
                    if (index > 0) {
                        out += ", ";
                    }

                    if (params[index] != nullptr) {
                        out += params[index]->as_string();
                    }
                }

                out += ") {\n";
                if (function_body() != nullptr) {
                    out += function_body()->as_string();
                }
                out += "\n}";
                return out;
            }
            case ObjectType::Builtin:
                return "builtin function";
        }

        return "Unknown";
    }

    [[nodiscard]] auto hash_key() const -> std::optional<HashKey> {
        switch (object_type) {
            case ObjectType::Integer:
                return HashKey {ObjectType::Integer, std::hash<std::int64_t> {}(integer_value())};
            case ObjectType::Boolean:
                return HashKey {ObjectType::Boolean, std::hash<bool> {}(boolean_value())};
            case ObjectType::String:
                return HashKey {ObjectType::String, std::hash<std::string> {}(string_value())};
            default:
                return std::nullopt;
        }
    }

    [[nodiscard]] static auto make_integer(std::int64_t value) -> Object {
        Object object;
        object.object_type = ObjectType::Integer;
        object.data.integer = value;
        return object;
    }

    [[nodiscard]] static auto make_boolean(bool value) -> Object {
        Object object;
        object.object_type = ObjectType::Boolean;
        object.data.boolean = value;
        return object;
    }

    [[nodiscard]] static auto make_string(std::string value) -> Object {
        Object object;
        object.object_type = ObjectType::String;
        object.data.pointer = new StringData {std::move(value)};
        return object;
    }

    [[nodiscard]] static auto make_array(std::vector<std::shared_ptr<Object>> elements) -> Object {
        Object object;
        object.object_type = ObjectType::Array;
        object.data.pointer = new ArrayData {std::move(elements)};
        return object;
    }

    [[nodiscard]] static auto make_hash(
        std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Hash;
        object.data.pointer = new HashData {std::move(pairs)};
        return object;
    }

    [[nodiscard]] static auto make_null() -> Object {
        return Object {};
    }

    [[nodiscard]] static auto make_return(std::shared_ptr<Object> value) -> Object {
        Object object;
        object.object_type = ObjectType::ReturnValue;
        object.data.pointer = new ReturnValueData {std::move(value)};
        return object;
    }

    [[nodiscard]] static auto make_error(std::string message) -> Object {
        Object object;
        object.object_type = ObjectType::Error;
        object.data.pointer = new ErrorData {std::move(message)};
        return object;
    }

    [[nodiscard]] static auto make_function(
        std::vector<std::unique_ptr<Identifier>> parameters,
        std::unique_ptr<BlockStatement> body,
        std::shared_ptr<Environment> env
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Function;
        object.data.pointer = new FunctionData {
            std::move(parameters),
            std::move(body),
            std::move(env),
        };
        return object;
    }

    [[nodiscard]] static auto make_builtin(BuiltinFunction function) -> Object {
        Object object;
        object.object_type = ObjectType::Builtin;
        object.data.pointer = new BuiltinData {std::move(function)};
        return object;
    }

    [[nodiscard]] auto integer_value() const -> std::int64_t {
        return data.integer;
    }

    [[nodiscard]] auto boolean_value() const -> bool {
        return data.boolean;
    }

    [[nodiscard]] auto string_value() const -> const std::string& {
        return payload<StringData>()->value;
    }

    [[nodiscard]] auto array_elements() const -> const std::vector<std::shared_ptr<Object>>& {
        return payload<ArrayData>()->elements;
    }

    [[nodiscard]] auto array_elements_mut() -> std::vector<std::shared_ptr<Object>>& {
        return payload<ArrayData>()->elements;
    }

    [[nodiscard]] auto hash_pairs() const
        -> const std::unordered_map<HashKey, HashPair, HashKeyHasher>& {
        return payload<HashData>()->pairs;
    }

    [[nodiscard]] auto hash_pairs_mut()
        -> std::unordered_map<HashKey, HashPair, HashKeyHasher>& {
        return payload<HashData>()->pairs;
    }

    [[nodiscard]] auto return_value() const -> const std::shared_ptr<Object>& {
        return payload<ReturnValueData>()->value;
    }

    [[nodiscard]] auto error_message() const -> const std::string& {
        return payload<ErrorData>()->message;
    }

    [[nodiscard]] auto function_parameters() const
        -> const std::vector<std::unique_ptr<Identifier>>& {
        return payload<FunctionData>()->parameters;
    }

    [[nodiscard]] auto function_body() const -> const BlockStatement* {
        return payload<FunctionData>()->body.get();
    }

    [[nodiscard]] auto function_env() const -> const std::shared_ptr<Environment>& {
        return payload<FunctionData>()->env;
    }

    [[nodiscard]] auto function_env_mut() -> std::shared_ptr<Environment>& {
        return payload<FunctionData>()->env;
    }

    [[nodiscard]] auto builtin_function() const -> const BuiltinFunction& {
        return payload<BuiltinData>()->function;
    }

private:
    template <typename T>
    [[nodiscard]] auto payload() const -> T* {
        return static_cast<T*>(data.pointer);
    }

    void destroy() {
        switch (object_type) {
            case ObjectType::String:
                delete payload<StringData>();
                break;
            case ObjectType::Array:
                delete payload<ArrayData>();
                break;
            case ObjectType::Hash:
                delete payload<HashData>();
                break;
            case ObjectType::ReturnValue:
                delete payload<ReturnValueData>();
                break;
            case ObjectType::Error:
                delete payload<ErrorData>();
                break;
            case ObjectType::Function:
                delete payload<FunctionData>();
                break;
            case ObjectType::Builtin:
                delete payload<BuiltinData>();
                break;
            case ObjectType::Integer:
            case ObjectType::Boolean:
            case ObjectType::Null:
                break;
        }

        object_type = ObjectType::Null;
        data.pointer = nullptr;
    }

    void move_from(Object&& other) {
        object_type = other.object_type;

        switch (other.object_type) {
            case ObjectType::Integer:
                data.integer = other.data.integer;
                break;
            case ObjectType::Boolean:
                data.boolean = other.data.boolean;
                break;
            default:
                data.pointer = other.data.pointer;
                other.data.pointer = nullptr;
                break;
        }

        other.object_type = ObjectType::Null;
    }
};
