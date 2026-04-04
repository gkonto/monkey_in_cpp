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

        if (const auto* return_value = dynamic_cast<const ReturnValueObject*>(result.get()); return_value != nullptr) {
            return return_value->value;
        }
    }

    return result;
}

[[nodiscard]] auto eval_block_statement(const BlockStatement* block) -> std::shared_ptr<Object> {
    auto result = nullObject();

    for (const auto& statement : block->statements) {
        result = eval(statement.get());

        if (result->type() == ObjectType::ReturnValue) {
            return result;
        }
    }

    return result;
}

[[nodiscard]] auto is_truthy(const std::shared_ptr<Object>& object) -> bool {
    if (object == nullObject()) {
        return false;
    }

    if (object == nativeBoolToBooleanObject(true)) {
        return true;
    }

    if (object == nativeBoolToBooleanObject(false)) {
        return false;
    }

    return true;
}

[[nodiscard]] auto eval_if_expression(const IfExpression* expression) -> std::shared_ptr<Object> {
    const auto condition = eval(expression->condition.get());

    if (is_truthy(condition)) {
        return eval(expression->consequence.get());
    }

    if (expression->alternative != nullptr) {
        return eval(expression->alternative.get());
    }

    return nullObject();
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

[[nodiscard]] auto eval_minus_prefix_operator_expression(const std::shared_ptr<Object>& right) -> std::shared_ptr<Object> {
    const auto* integer = dynamic_cast<const IntegerObject*>(right.get());
    if (integer == nullptr) {
        return nullObject();
    }

    auto result = std::make_shared<IntegerObject>();
    result->value = -integer->value;
    return result;
}

[[nodiscard]] auto eval_prefix_expression(
    const std::string& op,
    const std::shared_ptr<Object>& right
) -> std::shared_ptr<Object> {
    if (op == "!") {
        return eval_bang_operator_expression(right);
    }

    if (op == "-") {
        return eval_minus_prefix_operator_expression(right);
    }

    return nullObject();
}

[[nodiscard]] auto eval_integer_infix_expression(
    const std::string& op,
    const IntegerObject* left,
    const IntegerObject* right
) -> std::shared_ptr<Object> {
    if (op == "+") {
        auto result = std::make_shared<IntegerObject>();
        result->value = left->value + right->value;
        return result;
    }

    if (op == "-") {
        auto result = std::make_shared<IntegerObject>();
        result->value = left->value - right->value;
        return result;
    }

    if (op == "*") {
        auto result = std::make_shared<IntegerObject>();
        result->value = left->value * right->value;
        return result;
    }

    if (op == "/") {
        auto result = std::make_shared<IntegerObject>();
        result->value = left->value / right->value;
        return result;
    }

    if (op == "<") {
        return nativeBoolToBooleanObject(left->value < right->value);
    }

    if (op == ">") {
        return nativeBoolToBooleanObject(left->value > right->value);
    }

    if (op == "==") {
        return nativeBoolToBooleanObject(left->value == right->value);
    }

    if (op == "!=") {
        return nativeBoolToBooleanObject(left->value != right->value);
    }

    return nullObject();
}

[[nodiscard]] auto eval_infix_expression(
    const std::string& op,
    const std::shared_ptr<Object>& left,
    const std::shared_ptr<Object>& right
) -> std::shared_ptr<Object> {
    const auto* left_integer = dynamic_cast<const IntegerObject*>(left.get());
    const auto* right_integer = dynamic_cast<const IntegerObject*>(right.get());

    if (left_integer != nullptr && right_integer != nullptr) {
        return eval_integer_infix_expression(op, left_integer, right_integer);
    }

    if (op == "==") {
        return nativeBoolToBooleanObject(left == right);
    }

    if (op == "!=") {
        return nativeBoolToBooleanObject(left != right);
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

    if (const auto* statement = dynamic_cast<const ReturnStatement*>(node); statement != nullptr) {
        auto result = std::make_shared<ReturnValueObject>();
        result->value = eval(statement->return_value.get());
        return result;
    }

    if (const auto* block = dynamic_cast<const BlockStatement*>(node); block != nullptr) {
        return eval_block_statement(block);
    }

    if (const auto* prefix_expression = dynamic_cast<const PrefixExpression*>(node); prefix_expression != nullptr) {
        const auto right = eval(prefix_expression->right.get());
        return eval_prefix_expression(prefix_expression->op, right);
    }

    if (const auto* infix_expression = dynamic_cast<const InfixExpression*>(node); infix_expression != nullptr) {
        const auto left = eval(infix_expression->left.get());
        const auto right = eval(infix_expression->right.get());
        return eval_infix_expression(infix_expression->op, left, right);
    }

    if (const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(node); integer_literal != nullptr) {
        auto integer = std::make_shared<IntegerObject>();
        integer->value = integer_literal->value;
        return integer;
    }

    if (const auto* boolean = dynamic_cast<const Boolean*>(node); boolean != nullptr) {
        return nativeBoolToBooleanObject(boolean->value);
    }

    if (const auto* if_expression = dynamic_cast<const IfExpression*>(node); if_expression != nullptr) {
        return eval_if_expression(if_expression);
    }

    return nullObject();
}
