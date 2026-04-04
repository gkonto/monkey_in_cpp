#pragma once

#include <memory>

#include "monkey/ast.hpp"
#include "monkey/environment.hpp"
#include "monkey/object.hpp"

[[nodiscard]] auto eval(const Node* node) -> std::shared_ptr<Object>;
[[nodiscard]] auto eval(const Node* node, Environment& environment) -> std::shared_ptr<Object>;
