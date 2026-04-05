#include "monkey/environment.hpp"
#include "monkey/object.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] auto promote_environment(Environment* environment, PersistentStore& store)
    -> Environment* {
    if (environment == nullptr) {
        return nullptr;
    }

    if (environment->is_persistent()) {
        return environment;
    }

    if (environment->promoted_copy() != nullptr) {
        return environment->promoted_copy();
    }

    auto* promoted_outer = promote_environment(environment->outer(), store);
    auto* promoted_environment = store.create_environment(promoted_outer);
    environment->set_promoted_copy(promoted_environment);

    for (std::size_t index = 0; index < environment->slot_count(); ++index) {
        promoted_environment->set_at(index, promote(environment->slot_value(index), store));
    }

    return promoted_environment;
}

[[nodiscard]] auto promote_object(Object* object, PersistentStore& store) -> Object* {
    if (object == nullptr) {
        return nullptr;
    }

    if (object->is_persistent()) {
        return object;
    }

    if (object->promoted_copy_cached() != nullptr) {
        return object->promoted_copy_cached();
    }

    switch (object->type()) {
        case ObjectType::String: {
            auto* promoted = store.emplace<Object>(
                Object::make_string(object->string_value(), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::Array: {
            std::vector<Value> elements;
            elements.reserve(object->array_elements().size());
            for (const auto& element : object->array_elements()) {
                elements.push_back(promote(element, store));
            }

            auto* promoted = store.emplace<Object>(
                Object::make_array(std::move(elements), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::Hash: {
            std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;
            for (const auto& [key, pair] : object->hash_pairs()) {
                pairs.emplace(key, HashPair {
                    promote(pair.key, store),
                    promote(pair.value, store),
                });
            }

            auto* promoted = store.emplace<Object>(
                Object::make_hash(std::move(pairs), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::ReturnValue: {
            auto* promoted = store.emplace<Object>(
                Object::make_return(promote(object->return_value(), store), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::Error: {
            auto* promoted = store.emplace<Object>(
                Object::make_error(object->error_message(), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::Function: {
            auto* promoted = store.emplace<Object>(
                Object::make_function(
                    clone_function_parameters(*object),
                    object->function_body() != nullptr
                        ? clone_block_statement(*object->function_body())
                        : nullptr,
                    nullptr,
                    ObjectLifetime::Persistent
                )
            );
            object->set_promoted_copy(promoted);
            promoted->function_env_mut() = promote_environment(object->function_env(), store);
            return promoted;
        }
        case ObjectType::Builtin: {
            auto* promoted = store.emplace<Object>(
                Object::make_builtin(object->builtin_function(), ObjectLifetime::Persistent)
            );
            object->set_promoted_copy(promoted);
            return promoted;
        }
        case ObjectType::Integer:
        case ObjectType::Boolean:
        case ObjectType::Null:
            return nullptr;
    }

    return nullptr;
}

}  // namespace

auto promote(Value value, PersistentStore& store) -> Value {
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

    return Value::from_object(promote_object(object, store));
}

auto detach(Value value) -> DetachedValue {
    auto store = std::make_unique<PersistentStore>();
    auto detached = promote(value, *store);
    return DetachedValue {std::move(store), detached};
}
