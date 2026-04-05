#pragma once

#include "monkey/ast.hpp"
#include "monkey/environment.hpp"
#include "monkey/object.hpp"

[[nodiscard]] auto eval(const Node* node) -> DetachedValue;
[[nodiscard]] auto eval(const Node* node, Environment& environment) -> Value;
