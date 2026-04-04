#include "monkey/evaluator.hpp"

#include <memory>

namespace {

[[nodiscard]] auto nativeBoolToBooleanObject(bool input) -> std::shared_ptr<Object> {
    static auto true_object = [] {
        auto object = std::make_shared<BooleanObject>();
        object->value = true;
        return object;
    }();

    static auto false_object = [] {
        auto object = std::make_shared<BooleanObject>();
        object->value = false;
        return object;
    }();

    return input ? true_object : false_object;
}

[[nodiscard]] auto nullObject() -> std::shared_ptr<Object> {
    static auto null_object = std::make_shared<NullObject>();
    return null_object;
}

[[nodiscard]] auto eval_program(const Program* program) -> std::shared_ptr<Object> {
    auto result = nullObject();

    for (const auto& statement : program->statements) {
        result = eval(statement.get());
    }

    return result;
}

[[nodiscard]] auto eval_bang_operator_expression(const std::shared_ptr<Object>& right) -> std::shared_ptr<Object> {
    if (right == nativeBoolToBooleanObject(true)) {
        return nativeBoolToBooleanObject(false);
    }

    if (right == nativeBoolToBooleanObject(false)) {
        return nativeBoolToBooleanObject(true);
    }

    if (right == nullObject()) {
        return nativeBoolToBooleanObject(true);
    }

    return nativeBoolToBooleanObject(false);
}

[[nodiscard]] auto eval_prefix_expression(
    const std::string& op,
    const std::shared_ptr<Object>& right
) -> std::shared_ptr<Object> {
    if (op == "!") {
        return eval_bang_operator_expression(right);
    }

    return nullObject();
}

}  // namespace

auto eval(const Node* node) -> std::shared_ptr<Object> {
    if (node == nullptr) {
        return nullObject();
    }

    if (const auto* program = dynamic_cast<const Program*>(node); program != nullptr) {
        return eval_program(program);
    }

    if (const auto* statement = dynamic_cast<const ExpressionStatement*>(node); statement != nullptr) {
        return eval(statement->expression.get());
    }

    if (const auto* prefix_expression = dynamic_cast<const PrefixExpression*>(node); prefix_expression != nullptr) {
        const auto right = eval(prefix_expression->right.get());
        return eval_prefix_expression(prefix_expression->op, right);
    }

    if (const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(node); integer_literal != nullptr) {
        auto integer = std::make_shared<IntegerObject>();
        integer->value = integer_literal->value;
        return integer;
    }

    if (const auto* boolean = dynamic_cast<const Boolean*>(node); boolean != nullptr) {
        return nativeBoolToBooleanObject(boolean->value);
    }

    return nullObject();
}
