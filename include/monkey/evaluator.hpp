#pragma once

#include <memory>

#include "monkey/ast.hpp"
#include "monkey/object.hpp"

[[nodiscard]] auto eval(const Node* node) -> std::shared_ptr<Object>;
