#pragma once

#include <memory>
#include <vector>

#include "monkey/object.hpp"

class Environment {
public:
    Environment() = default;

    explicit Environment(std::shared_ptr<Environment> outer) : outer_(std::move(outer)) {}

    void set_at(std::size_t slot, std::shared_ptr<Object> value) {
        if (slot >= slots_.size()) {
            slots_.resize(slot + 1);
        }

        slots_[slot] = std::move(value);
    }

    [[nodiscard]] auto get_at(std::size_t depth, std::size_t slot) const -> std::shared_ptr<Object> {
        auto current = this;
        std::size_t remaining_depth = depth;

        while (remaining_depth > 0) {
            if (current->outer_ == nullptr) {
                return nullptr;
            }

            current = current->outer_.get();
            --remaining_depth;
        }

        if (slot >= current->slots_.size()) {
            return nullptr;
        }

        return current->slots_[slot];
    }

    void set_root_at(std::size_t slot, std::shared_ptr<Object> value) {
        auto* root = this;
        while (root->outer_ != nullptr) {
            root = root->outer_.get();
        }

        root->set_at(slot, std::move(value));
    }

    [[nodiscard]] auto get_root_at(std::size_t slot) const -> std::shared_ptr<Object> {
        auto const* root = this;
        while (root->outer_ != nullptr) {
            root = root->outer_.get();
        }

        return root->get_at(0, slot);
    }

private:
    std::vector<std::shared_ptr<Object>> slots_ {};
    std::shared_ptr<Environment> outer_ {};
};

[[nodiscard]] inline auto new_enclosed_environment(std::shared_ptr<Environment> outer) -> std::shared_ptr<Environment> {
    return std::make_shared<Environment>(std::move(outer));
}
