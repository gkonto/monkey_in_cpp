#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "monkey/object.hpp"

class Environment {
public:
    Environment() = default;

    explicit Environment(std::shared_ptr<Environment> outer) : outer_(std::move(outer)) {}

    void set(const std::string& name, std::shared_ptr<Object> value) {
        store_[name] = std::move(value);
    }

    [[nodiscard]] auto get(const std::string& name) const -> std::shared_ptr<Object> {
        if (const auto it = store_.find(name); it != store_.end()) {
            return it->second;
        }

        if (outer_ != nullptr) {
            return outer_->get(name);
        }

        return nullptr;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Object>> store_ {};
    std::shared_ptr<Environment> outer_ {};
};

[[nodiscard]] inline auto new_enclosed_environment(std::shared_ptr<Environment> outer) -> std::shared_ptr<Environment> {
    return std::make_shared<Environment>(std::move(outer));
}
