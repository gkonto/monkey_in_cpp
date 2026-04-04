#include "monkey/evaluator.hpp"

#include <memory>

namespace {

[[nodiscard]] auto eval_program(const Program* program) -> std::unique_ptr<Object> {
    std::unique_ptr<Object> result {};

    for (const auto& statement : program->statements) {
        result = eval(statement.get());
    }

    return result;
}

}  // namespace

auto eval(const Node* node) -> std::unique_ptr<Object> {
    if (node == nullptr) {
        return nullptr;
    }

    if (const auto* program = dynamic_cast<const Program*>(node); program != nullptr) {
        return eval_program(program);
    }

    if (const auto* statement = dynamic_cast<const ExpressionStatement*>(node); statement != nullptr) {
        return eval(statement->expression.get());
    }

    if (const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(node); integer_literal != nullptr) {
        auto integer = std::make_unique<IntegerObject>();
        integer->value = integer_literal->value;
        return integer;
    }

    return nullptr;
}
