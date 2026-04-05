#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <new>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "monkey/ast.hpp"

class Environment;
class ScratchArena;
class PersistentStore;

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

enum class ObjectLifetime {
    Standalone,
    Scratch,
    Persistent,
    Static,
};

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

union ValueData {
    std::int64_t integer;
    bool boolean;
    Object* object;

    constexpr ValueData() : object(nullptr) {}
};

struct Value {
    ObjectType object_type {ObjectType::Null};
    ValueData data {};

    constexpr Value() = default;

    [[nodiscard]] static auto make_integer(std::int64_t value) -> Value {
        Value result;
        result.object_type = ObjectType::Integer;
        result.data.integer = value;
        return result;
    }

    [[nodiscard]] static auto make_boolean(bool value) -> Value {
        Value result;
        result.object_type = ObjectType::Boolean;
        result.data.boolean = value;
        return result;
    }

    [[nodiscard]] static auto make_null() -> Value {
        return Value {};
    }

    [[nodiscard]] static auto from_object(Object* object) -> Value;

    [[nodiscard]] auto type() const -> ObjectType {
        return object_type;
    }

    [[nodiscard]] auto is_immediate() const -> bool {
        switch (object_type) {
            case ObjectType::Integer:
            case ObjectType::Boolean:
            case ObjectType::Null:
                return true;
            default:
                return false;
        }
    }

    [[nodiscard]] auto is_heap_object() const -> bool {
        return !is_immediate();
    }

    [[nodiscard]] auto heap_object() const -> const Object* {
        return is_heap_object() ? data.object : nullptr;
    }

    [[nodiscard]] auto heap_object_mut() const -> Object* {
        return is_heap_object() ? data.object : nullptr;
    }

    [[nodiscard]] auto integer_value() const -> std::int64_t {
        return data.integer;
    }

    [[nodiscard]] auto boolean_value() const -> bool {
        return data.boolean;
    }

    [[nodiscard]] auto inspect() const -> std::string;
    [[nodiscard]] auto hash_key() const -> std::optional<HashKey>;
};

struct HashPair {
    Value key;
    Value value;
};

struct StringData {
    std::string value;
};

struct ArrayData {
    std::vector<Value> elements;
};

struct HashData {
    std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;
};

struct ReturnValueData {
    Value value;
};

struct ErrorData {
    std::string message;
};

using BuiltinFunction = Value (*)(ScratchArena&, const std::vector<Value>&);

struct BuiltinData {
    BuiltinFunction function {nullptr};
};

struct FunctionData {
    std::vector<std::unique_ptr<Identifier>> parameters;
    std::unique_ptr<BlockStatement> body;
    Environment* env {nullptr};
};

union ObjectPayload {
    StringData string;
    ArrayData array;
    HashData hash;
    ReturnValueData return_value;
    ErrorData error;
    FunctionData function;
    BuiltinData builtin;

    ObjectPayload() {}
    ~ObjectPayload() {}
};

struct Object {
    ObjectType object_type {ObjectType::Null};
    ObjectLifetime lifetime {ObjectLifetime::Standalone};
    std::int64_t integer {0};
    bool boolean {false};
    ObjectPayload payload;

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

    [[nodiscard]] auto storage() const -> ObjectLifetime {
        return lifetime;
    }

    [[nodiscard]] auto is_persistent() const -> bool {
        return lifetime == ObjectLifetime::Persistent || lifetime == ObjectLifetime::Static;
    }

    [[nodiscard]] auto inspect() const -> std::string {
        switch (object_type) {
            case ObjectType::Integer:
                return std::to_string(integer);
            case ObjectType::Boolean:
                return boolean ? "true" : "false";
            case ObjectType::String:
                return payload.string.value;
            case ObjectType::Array: {
                std::string out = "[";
                for (std::size_t index = 0; index < payload.array.elements.size(); ++index) {
                    if (index > 0) {
                        out += ", ";
                    }
                    out += payload.array.elements[index].inspect();
                }
                out += "]";
                return out;
            }
            case ObjectType::Hash: {
                std::string out = "{";
                bool first = true;
                for (const auto& [_, pair] : payload.hash.pairs) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += pair.key.inspect();
                    out += ": ";
                    out += pair.value.inspect();
                }
                out += "}";
                return out;
            }
            case ObjectType::Null:
                return "null";
            case ObjectType::ReturnValue:
                return payload.return_value.value.inspect();
            case ObjectType::Error:
                return "ERROR: " + payload.error.message;
            case ObjectType::Function: {
                std::string out = "fn(";
                for (std::size_t index = 0; index < payload.function.parameters.size(); ++index) {
                    if (index > 0) {
                        out += ", ";
                    }
                    if (payload.function.parameters[index] != nullptr) {
                        out += payload.function.parameters[index]->as_string();
                    }
                }
                out += ") {\n";
                if (payload.function.body != nullptr) {
                    out += payload.function.body->as_string();
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
                return HashKey {ObjectType::Integer, std::hash<std::int64_t> {}(integer)};
            case ObjectType::Boolean:
                return HashKey {ObjectType::Boolean, std::hash<bool> {}(boolean)};
            case ObjectType::String:
                return HashKey {ObjectType::String, std::hash<std::string> {}(payload.string.value)};
            default:
                return std::nullopt;
        }
    }

    [[nodiscard]] static auto make_integer(
        std::int64_t value,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Integer;
        object.lifetime = lifetime;
        object.integer = value;
        return object;
    }

    [[nodiscard]] static auto make_boolean(
        bool value,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Boolean;
        object.lifetime = lifetime;
        object.boolean = value;
        return object;
    }

    [[nodiscard]] static auto make_string(
        std::string value,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::String;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.string, StringData {std::move(value)});
        return object;
    }

    [[nodiscard]] static auto make_array(
        std::vector<Value> elements,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Array;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.array, ArrayData {std::move(elements)});
        return object;
    }

    [[nodiscard]] static auto make_hash(
        std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Hash;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.hash, HashData {std::move(pairs)});
        return object;
    }

    [[nodiscard]] static auto make_null(
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Null;
        object.lifetime = lifetime;
        return object;
    }

    [[nodiscard]] static auto make_return(
        Value value,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::ReturnValue;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.return_value, ReturnValueData {value});
        return object;
    }

    [[nodiscard]] static auto make_error(
        std::string message,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Error;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.error, ErrorData {std::move(message)});
        return object;
    }

    [[nodiscard]] static auto make_function(
        std::vector<std::unique_ptr<Identifier>> parameters,
        std::unique_ptr<BlockStatement> body,
        Environment* env,
        ObjectLifetime lifetime = ObjectLifetime::Standalone
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Function;
        object.lifetime = lifetime;
        std::construct_at(
            &object.payload.function,
            FunctionData {std::move(parameters), std::move(body), env}
        );
        return object;
    }

    [[nodiscard]] static auto make_builtin(
        BuiltinFunction function,
        ObjectLifetime lifetime = ObjectLifetime::Static
    ) -> Object {
        Object object;
        object.object_type = ObjectType::Builtin;
        object.lifetime = lifetime;
        std::construct_at(&object.payload.builtin, BuiltinData {function});
        return object;
    }

    [[nodiscard]] auto integer_value() const -> std::int64_t {
        return integer;
    }

    [[nodiscard]] auto boolean_value() const -> bool {
        return boolean;
    }

    [[nodiscard]] auto string_value() const -> const std::string& {
        return payload.string.value;
    }

    [[nodiscard]] auto array_elements() const -> const std::vector<Value>& {
        return payload.array.elements;
    }

    [[nodiscard]] auto array_elements_mut() -> std::vector<Value>& {
        return payload.array.elements;
    }

    [[nodiscard]] auto hash_pairs() const
        -> const std::unordered_map<HashKey, HashPair, HashKeyHasher>& {
        return payload.hash.pairs;
    }

    [[nodiscard]] auto hash_pairs_mut()
        -> std::unordered_map<HashKey, HashPair, HashKeyHasher>& {
        return payload.hash.pairs;
    }

    [[nodiscard]] auto return_value() const -> const Value& {
        return payload.return_value.value;
    }

    [[nodiscard]] auto error_message() const -> const std::string& {
        return payload.error.message;
    }

    [[nodiscard]] auto function_parameters() const
        -> const std::vector<std::unique_ptr<Identifier>>& {
        return payload.function.parameters;
    }

    [[nodiscard]] auto function_body() const -> const BlockStatement* {
        return payload.function.body.get();
    }

    [[nodiscard]] auto function_env() const -> Environment* {
        return payload.function.env;
    }

    [[nodiscard]] auto function_env_mut() -> Environment*& {
        return payload.function.env;
    }

    [[nodiscard]] auto builtin_function() const -> BuiltinFunction {
        return payload.builtin.function;
    }

private:
    void destroy() {
        switch (object_type) {
            case ObjectType::String:
                std::destroy_at(&payload.string);
                break;
            case ObjectType::Array:
                std::destroy_at(&payload.array);
                break;
            case ObjectType::Hash:
                std::destroy_at(&payload.hash);
                break;
            case ObjectType::ReturnValue:
                std::destroy_at(&payload.return_value);
                break;
            case ObjectType::Error:
                std::destroy_at(&payload.error);
                break;
            case ObjectType::Function:
                std::destroy_at(&payload.function);
                break;
            case ObjectType::Builtin:
                std::destroy_at(&payload.builtin);
                break;
            case ObjectType::Integer:
            case ObjectType::Boolean:
            case ObjectType::Null:
                break;
        }

        object_type = ObjectType::Null;
        lifetime = ObjectLifetime::Standalone;
        integer = 0;
        boolean = false;
    }

    void move_from(Object&& other) {
        object_type = other.object_type;
        lifetime = other.lifetime;
        integer = other.integer;
        boolean = other.boolean;

        switch (other.object_type) {
            case ObjectType::String:
                std::construct_at(&payload.string, std::move(other.payload.string));
                break;
            case ObjectType::Array:
                std::construct_at(&payload.array, std::move(other.payload.array));
                break;
            case ObjectType::Hash:
                std::construct_at(&payload.hash, std::move(other.payload.hash));
                break;
            case ObjectType::ReturnValue:
                std::construct_at(&payload.return_value, std::move(other.payload.return_value));
                break;
            case ObjectType::Error:
                std::construct_at(&payload.error, std::move(other.payload.error));
                break;
            case ObjectType::Function:
                std::construct_at(&payload.function, std::move(other.payload.function));
                break;
            case ObjectType::Builtin:
                std::construct_at(&payload.builtin, std::move(other.payload.builtin));
                break;
            case ObjectType::Integer:
            case ObjectType::Boolean:
            case ObjectType::Null:
                break;
        }

        other.destroy();
    }
};

inline auto Value::from_object(Object* object) -> Value {
    if (object == nullptr) {
        return make_null();
    }

    switch (object->type()) {
        case ObjectType::Integer:
            return make_integer(object->integer_value());
        case ObjectType::Boolean:
            return make_boolean(object->boolean_value());
        case ObjectType::Null:
            return make_null();
        default: {
            Value result;
            result.object_type = object->type();
            result.data.object = object;
            return result;
        }
    }
}

inline auto Value::inspect() const -> std::string {
    switch (object_type) {
        case ObjectType::Integer:
            return std::to_string(integer_value());
        case ObjectType::Boolean:
            return boolean_value() ? "true" : "false";
        case ObjectType::Null:
            return "null";
        default:
            return data.object != nullptr ? data.object->inspect() : "null";
    }
}

inline auto Value::hash_key() const -> std::optional<HashKey> {
    switch (object_type) {
        case ObjectType::Integer:
            return HashKey {ObjectType::Integer, std::hash<std::int64_t> {}(integer_value())};
        case ObjectType::Boolean:
            return HashKey {ObjectType::Boolean, std::hash<bool> {}(boolean_value())};
        case ObjectType::Null:
            return std::nullopt;
        default:
            return data.object != nullptr ? data.object->hash_key() : std::nullopt;
    }
}

struct DestructionEntry {
    void* pointer {nullptr};
    void (*destroy)(void*) {nullptr};
};

template <typename T>
inline void destroy_allocated(void* pointer) {
    std::destroy_at(static_cast<T*>(pointer));
}

class ScratchArena {
public:
    ScratchArena() : resource_(initial_buffer_.data(), initial_buffer_.size()) {}

    ScratchArena(const ScratchArena&) = delete;
    auto operator=(const ScratchArena&) -> ScratchArena& = delete;

    ~ScratchArena() {
        destroy_all();
    }

    template <typename T, typename... Args>
    [[nodiscard]] auto emplace(Args&&... args) -> T* {
        void* memory = resource_.allocate(sizeof(T), alignof(T));
        auto* pointer = ::new (memory) T(std::forward<Args>(args)...);
        destructors_.push_back({pointer, &destroy_allocated<T>});
        return pointer;
    }

    [[nodiscard]] auto allocate_string(std::string value) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_string(std::move(value), ObjectLifetime::Scratch))
        );
    }

    [[nodiscard]] auto allocate_array(std::vector<Value> elements) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_array(std::move(elements), ObjectLifetime::Scratch))
        );
    }

    [[nodiscard]] auto allocate_hash(
        std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs
    ) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_hash(std::move(pairs), ObjectLifetime::Scratch))
        );
    }

    [[nodiscard]] auto allocate_return(Value value) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_return(value, ObjectLifetime::Scratch))
        );
    }

    [[nodiscard]] auto allocate_error(std::string message) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_error(std::move(message), ObjectLifetime::Scratch))
        );
    }

    [[nodiscard]] auto allocate_function(
        std::vector<std::unique_ptr<Identifier>> parameters,
        std::unique_ptr<BlockStatement> body,
        Environment* env
    ) -> Value {
        return Value::from_object(
            emplace<Object>(
                Object::make_function(
                    std::move(parameters),
                    std::move(body),
                    env,
                    ObjectLifetime::Scratch
                )
            )
        );
    }

    void reset() {
        destroy_all();
        resource_.release();
    }

private:
    std::array<std::byte, 64 * 1024> initial_buffer_ {};
    std::pmr::monotonic_buffer_resource resource_;
    std::vector<DestructionEntry> destructors_ {};

    void destroy_all() {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); ++it) {
            it->destroy(it->pointer);
        }
        destructors_.clear();
    }
};

class PersistentStore {
public:
    PersistentStore() = default;
    PersistentStore(const PersistentStore&) = delete;
    auto operator=(const PersistentStore&) -> PersistentStore& = delete;

    ~PersistentStore() {
        destroy_all();
    }

    template <typename T, typename... Args>
    [[nodiscard]] auto emplace(Args&&... args) -> T* {
        void* memory = resource_.allocate(sizeof(T), alignof(T));
        auto* pointer = ::new (memory) T(std::forward<Args>(args)...);
        destructors_.push_back({pointer, &destroy_allocated<T>});
        return pointer;
    }

    [[nodiscard]] auto allocate_string(std::string value) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_string(std::move(value), ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto allocate_array(std::vector<Value> elements) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_array(std::move(elements), ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto allocate_hash(
        std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs
    ) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_hash(std::move(pairs), ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto allocate_return(Value value) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_return(value, ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto allocate_error(std::string message) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_error(std::move(message), ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto allocate_function(
        std::vector<std::unique_ptr<Identifier>> parameters,
        std::unique_ptr<BlockStatement> body,
        Environment* env
    ) -> Value {
        return Value::from_object(
            emplace<Object>(
                Object::make_function(
                    std::move(parameters),
                    std::move(body),
                    env,
                    ObjectLifetime::Persistent
                )
            )
        );
    }

    [[nodiscard]] auto allocate_builtin(BuiltinFunction function) -> Value {
        return Value::from_object(
            emplace<Object>(Object::make_builtin(function, ObjectLifetime::Persistent))
        );
    }

    [[nodiscard]] auto create_environment(Environment* outer = nullptr) -> Environment*;

private:
    std::pmr::unsynchronized_pool_resource resource_ {};
    std::vector<DestructionEntry> destructors_ {};

    void destroy_all() {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); ++it) {
            it->destroy(it->pointer);
        }
        destructors_.clear();
    }
};

struct DetachedValue {
    std::unique_ptr<PersistentStore> store;
    Value value;

    DetachedValue() = default;
    DetachedValue(std::unique_ptr<PersistentStore> store_value, Value detached_value)
        : store(std::move(store_value)), value(detached_value) {}

    [[nodiscard]] auto type() const -> ObjectType {
        return value.type();
    }

    [[nodiscard]] auto inspect() const -> std::string {
        return value.inspect();
    }
};

[[nodiscard]] inline auto clone_function_parameters(const Object& object)
    -> std::vector<std::unique_ptr<Identifier>> {
    std::vector<std::unique_ptr<Identifier>> parameters;
    for (const auto& parameter : object.function_parameters()) {
        if (parameter != nullptr) {
            auto parameter_clone = std::make_unique<Identifier>();
            parameter_clone->token = parameter->token;
            parameter_clone->value = parameter->value;
            parameter_clone->symbol = parameter->symbol;
            parameters.push_back(std::move(parameter_clone));
        }
    }
    return parameters;
}

[[nodiscard]] inline auto promote(Value value, PersistentStore& store) -> Value {
    if (value.is_immediate()) {
        return value;
    }

    auto* object = value.heap_object_mut();
    if (object == nullptr) {
        return Value::make_null();
    }

    if (object->is_persistent()) {
        return value;
    }

    switch (object->type()) {
        case ObjectType::String:
            return store.allocate_string(object->string_value());
        case ObjectType::Array: {
            std::vector<Value> elements;
            elements.reserve(object->array_elements().size());
            for (const auto& element : object->array_elements()) {
                elements.push_back(promote(element, store));
            }
            return store.allocate_array(std::move(elements));
        }
        case ObjectType::Hash: {
            std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;
            for (const auto& [key, pair] : object->hash_pairs()) {
                pairs.emplace(key, HashPair {
                    promote(pair.key, store),
                    promote(pair.value, store),
                });
            }
            return store.allocate_hash(std::move(pairs));
        }
        case ObjectType::ReturnValue:
            return store.allocate_return(promote(object->return_value(), store));
        case ObjectType::Error:
            return store.allocate_error(object->error_message());
        case ObjectType::Function:
            return store.allocate_function(
                clone_function_parameters(*object),
                object->function_body() != nullptr
                    ? clone_block_statement(*object->function_body())
                    : nullptr,
                object->function_env()
            );
        case ObjectType::Builtin:
            return store.allocate_builtin(object->builtin_function());
        case ObjectType::Integer:
            return Value::make_integer(object->integer_value());
        case ObjectType::Boolean:
            return Value::make_boolean(object->boolean_value());
        case ObjectType::Null:
            return Value::make_null();
    }

    return Value::make_null();
}

[[nodiscard]] inline auto detach(Value value) -> DetachedValue {
    auto store = std::make_unique<PersistentStore>();
    auto detached = promote(value, *store);
    return DetachedValue {std::move(store), detached};
}
