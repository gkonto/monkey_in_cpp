#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "monkey/object.hpp"

class Environment {
public:
    void set(const std::string& name, std::shared_ptr<Object> value) {
        store_[name] = std::move(value);
    }

    [[nodiscard]] auto get(const std::string& name) const -> std::shared_ptr<Object> {
        if (const auto it = store_.find(name); it != store_.end()) {
            return it->second;
        }

        return nullptr;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Object>> store_ {};
};
