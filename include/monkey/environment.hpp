#pragma once

#include <optional>
#include <vector>

#include "monkey/object.hpp"

class Environment {
public:
    explicit Environment(PersistentStore& store, Environment* outer = nullptr)
        : store_(&store), outer_(outer) {}

    void set_at(std::size_t slot, Value value) {
        if (slot >= slots_.size()) {
            slots_.resize(slot + 1);
        }

        slots_[slot] = value;
    }

    [[nodiscard]] auto get_at(std::size_t depth, std::size_t slot) const -> std::optional<Value> {
        auto current = this;
        std::size_t remaining_depth = depth;

        while (remaining_depth > 0) {
            if (current->outer_ == nullptr) {
                return std::nullopt;
            }

            current = current->outer_;
            --remaining_depth;
        }

        if (slot >= current->slots_.size()) {
            return std::nullopt;
        }

        return current->slots_[slot];
    }

    void set_root_at(std::size_t slot, Value value) {
        auto* root = this;
        while (root->outer_ != nullptr) {
            root = root->outer_;
        }

        root->set_at(slot, value);
    }

    [[nodiscard]] auto get_root_at(std::size_t slot) const -> std::optional<Value> {
        auto const* root = this;
        while (root->outer_ != nullptr) {
            root = root->outer_;
        }

        return root->get_at(0, slot);
    }

    [[nodiscard]] auto outer() const -> Environment* {
        return outer_;
    }

    [[nodiscard]] auto store() const -> PersistentStore& {
        return *store_;
    }

private:
    PersistentStore* store_ {nullptr};
    Environment* outer_ {nullptr};
    std::vector<Value> slots_ {};
};

[[nodiscard]] inline auto PersistentStore::create_environment(Environment* outer) -> Environment* {
    return emplace<Environment>(*this, outer);
}

[[nodiscard]] inline auto new_enclosed_environment(PersistentStore& store, Environment* outer)
    -> Environment* {
    return store.create_environment(outer);
}
