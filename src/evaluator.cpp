#include "monkey/evaluator.hpp"
#include "monkey/environment.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

[[nodiscard]] auto eval_node(const Node* node, Environment& environment) -> std::shared_ptr<Object>;

namespace {

[[nodiscard]] auto eval_hash_index_expression(
    const HashObject* hash,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object>;

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

[[nodiscard]] auto new_error(std::string message) -> std::shared_ptr<Object> {
    auto error = std::make_shared<ErrorObject>();
    error->message = std::move(message);
    return error;
}

[[nodiscard]] auto new_integer_object(std::int64_t value) -> std::shared_ptr<Object> {
    auto result = std::make_shared<IntegerObject>();
    result->value = value;
    return result;
}

[[nodiscard]] auto builtin_len(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (const auto* string = dynamic_cast<const StringObject*>(arguments[0].get()); string != nullptr) {
        return new_integer_object(static_cast<std::int64_t>(string->value.size()));
    }

    if (const auto* array = dynamic_cast<const ArrayObject*>(arguments[0].get()); array != nullptr) {
        return new_integer_object(static_cast<std::int64_t>(array->elements.size()));
    }

    return new_error(
        "argument to `len` not supported, got " +
        std::string(to_string(arguments[0]->type()))
    );
}

[[nodiscard]] auto builtin_first(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    const auto* array = dynamic_cast<const ArrayObject*>(arguments[0].get());
    if (array == nullptr) {
        return new_error(
            "argument to `first` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    if (array->elements.empty()) {
        return nullObject();
    }

    return array->elements.front();
}

[[nodiscard]] auto builtin_last(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    const auto* array = dynamic_cast<const ArrayObject*>(arguments[0].get());
    if (array == nullptr) {
        return new_error(
            "argument to `last` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    if (array->elements.empty()) {
        return nullObject();
    }

    return array->elements.back();
}

[[nodiscard]] auto builtin_rest(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    const auto* array = dynamic_cast<const ArrayObject*>(arguments[0].get());
    if (array == nullptr) {
        return new_error(
            "argument to `rest` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    if (array->elements.empty()) {
        return nullObject();
    }

    auto result = std::make_shared<ArrayObject>();
    result->elements.assign(array->elements.begin() + 1, array->elements.end());
    return result;
}

[[nodiscard]] auto builtin_push(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 2) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=2"
        );
    }

    const auto* array = dynamic_cast<const ArrayObject*>(arguments[0].get());
    if (array == nullptr) {
        return new_error(
            "argument to `push` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    auto result = std::make_shared<ArrayObject>();
    result->elements = array->elements;
    result->elements.push_back(arguments[1]);
    return result;
}

[[nodiscard]] auto get_builtin_by_name(std::string_view name) -> std::shared_ptr<Object> {
    if (name == "len") {
        static auto builtin = [] {
            auto object = std::make_shared<BuiltinObject>();
            object->function = builtin_len;
            return object;
        }();

        return builtin;
    }

    if (name == "first") {
        static auto builtin = [] {
            auto object = std::make_shared<BuiltinObject>();
            object->function = builtin_first;
            return object;
        }();

        return builtin;
    }

    if (name == "last") {
        static auto builtin = [] {
            auto object = std::make_shared<BuiltinObject>();
            object->function = builtin_last;
            return object;
        }();

        return builtin;
    }

    if (name == "rest") {
        static auto builtin = [] {
            auto object = std::make_shared<BuiltinObject>();
            object->function = builtin_rest;
            return object;
        }();

        return builtin;
    }

    if (name == "push") {
        static auto builtin = [] {
            auto object = std::make_shared<BuiltinObject>();
            object->function = builtin_push;
            return object;
        }();

        return builtin;
    }

    return nullptr;
}

[[nodiscard]] auto is_error(const std::shared_ptr<Object>& object) -> bool {
    return object != nullptr && object->type() == ObjectType::Error;
}

[[nodiscard]] auto eval_program(const Program* program, Environment& environment) -> std::shared_ptr<Object> {
    auto result = nullObject();

    for (const auto& statement : program->statements) {
        result = eval_node(statement.get(), environment);

        if (is_error(result)) {
            return result;
        }

        if (const auto* return_value = dynamic_cast<const ReturnValueObject*>(result.get()); return_value != nullptr) {
            return return_value->value;
        }
    }

    return result;
}

[[nodiscard]] auto eval_block_statement(const BlockStatement* block, Environment& environment) -> std::shared_ptr<Object> {
    auto result = nullObject();

    for (const auto& statement : block->statements) {
        result = eval_node(statement.get(), environment);

        if (result->type() == ObjectType::ReturnValue || result->type() == ObjectType::Error) {
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

[[nodiscard]] auto eval_if_expression(const IfExpression* expression, Environment& environment) -> std::shared_ptr<Object> {
    const auto condition = eval_node(expression->condition.get(), environment);
    if (is_error(condition)) {
        return condition;
    }

    if (is_truthy(condition)) {
        return eval_node(expression->consequence.get(), environment);
    }

    if (expression->alternative != nullptr) {
        return eval_node(expression->alternative.get(), environment);
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
        return new_error("unknown operator: -" + std::string(to_string(right->type())));
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

    return new_error("unknown operator: " + op + std::string(to_string(right->type())));
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

    return new_error(
        "unknown operator: " +
        std::string(to_string(left->type())) + " " +
        op + " " +
        std::string(to_string(right->type()))
    );
}

[[nodiscard]] auto eval_string_infix_expression(
    const std::string& op,
    const StringObject* left,
    const StringObject* right
) -> std::shared_ptr<Object> {
    if (op == "+") {
        auto result = std::make_shared<StringObject>();
        result->value = left->value + right->value;
        return result;
    }

    return new_error(
        "unknown operator: " +
        std::string(to_string(left->type())) + " " +
        op + " " +
        std::string(to_string(right->type()))
    );
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

    const auto* left_string = dynamic_cast<const StringObject*>(left.get());
    const auto* right_string = dynamic_cast<const StringObject*>(right.get());

    if (left_string != nullptr && right_string != nullptr) {
        return eval_string_infix_expression(op, left_string, right_string);
    }

    if (op == "==") {
        return nativeBoolToBooleanObject(left == right);
    }

    if (op == "!=") {
        return nativeBoolToBooleanObject(left != right);
    }

    if (left->type() != right->type()) {
        return new_error(
            "type mismatch: " +
            std::string(to_string(left->type())) + " " +
            op + " " +
            std::string(to_string(right->type()))
        );
    }

    return new_error(
        "unknown operator: " +
        std::string(to_string(left->type())) + " " +
        op + " " +
        std::string(to_string(right->type()))
    );
}

[[nodiscard]] auto eval_array_index_expression(
    const ArrayObject* array,
    const IntegerObject* index
) -> std::shared_ptr<Object> {
    if (index->value < 0) {
        return nullObject();
    }

    const auto max_index = static_cast<std::int64_t>(array->elements.size()) - 1;
    if (index->value > max_index) {
        return nullObject();
    }

    return array->elements[static_cast<std::size_t>(index->value)];
}

[[nodiscard]] auto eval_index_expression(
    const std::shared_ptr<Object>& left,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object> {
    const auto* array = dynamic_cast<const ArrayObject*>(left.get());
    const auto* integer_index = dynamic_cast<const IntegerObject*>(index.get());

    if (array != nullptr && integer_index != nullptr) {
        return eval_array_index_expression(array, integer_index);
    }

    if (const auto* hash = dynamic_cast<const HashObject*>(left.get()); hash != nullptr) {
        return eval_hash_index_expression(hash, index);
    }

    return new_error(
        "index operator not supported: " + std::string(to_string(left->type()))
    );
}

[[nodiscard]] auto hash_key_for_object(const std::shared_ptr<Object>& object)
    -> std::optional<HashKey> {
    if (object == nullptr) {
        return std::nullopt;
    }

    return object->hash_key();
}

[[nodiscard]] auto eval_hash_literal(const HashLiteral* hash_literal, Environment& environment)
    -> std::shared_ptr<Object> {
    auto hash = std::make_shared<HashObject>();

    for (const auto& [key_node, value_node] : hash_literal->pairs) {
        const auto key = eval_node(key_node.get(), environment);
        if (is_error(key)) {
            return key;
        }

        const auto hash_key = hash_key_for_object(key);
        if (!hash_key.has_value()) {
            return new_error("unusable as hash key: " + std::string(to_string(key->type())));
        }

        const auto value = eval_node(value_node.get(), environment);
        if (is_error(value)) {
            return value;
        }

        hash->pairs[*hash_key] = HashPair {key, value};
    }

    return hash;
}

[[nodiscard]] auto eval_hash_index_expression(
    const HashObject* hash,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object> {
    const auto hash_key = hash_key_for_object(index);
    if (!hash_key.has_value()) {
        return new_error("unusable as hash key: " + std::string(to_string(index->type())));
    }

    const auto iterator = hash->pairs.find(*hash_key);
    if (iterator == hash->pairs.end()) {
        return nullObject();
    }

    return iterator->second.value;
}

[[nodiscard]] auto apply_function(
    const std::shared_ptr<Object>& function,
    const std::vector<std::shared_ptr<Object>>& arguments
) -> std::shared_ptr<Object> {
    if (const auto* builtin = dynamic_cast<const BuiltinObject*>(function.get()); builtin != nullptr) {
        return builtin->function(arguments);
    }

    const auto* function_object = dynamic_cast<const FunctionObject*>(function.get());
    if (function_object == nullptr) {
        return new_error("not a function: " + std::string(to_string(function->type())));
    }

    auto extended_environment = new_enclosed_environment(function_object->env);
    for (std::size_t index = 0; index < function_object->parameters.size(); ++index) {
        if (function_object->parameters[index] != nullptr && index < arguments.size()) {
            extended_environment->set(function_object->parameters[index]->value, arguments[index]);
        }
    }

    auto evaluated = eval_node(function_object->body.get(), *extended_environment);
    if (const auto* return_value = dynamic_cast<const ReturnValueObject*>(evaluated.get()); return_value != nullptr) {
        return return_value->value;
    }

    return evaluated;
}

[[nodiscard]] auto eval_expressions(
    const std::vector<std::unique_ptr<Expression>>& expressions,
    Environment& environment
) -> std::vector<std::shared_ptr<Object>> {
    std::vector<std::shared_ptr<Object>> result;
    result.reserve(expressions.size());

    for (const auto& expression : expressions) {
        auto evaluated = eval_node(expression.get(), environment);
        if (is_error(evaluated)) {
            return {evaluated};
        }

        result.push_back(std::move(evaluated));
    }

    return result;
}

}  // namespace

auto eval_node(const Node* node, Environment& environment) -> std::shared_ptr<Object> {
    if (node == nullptr) {
        return nullObject();
    }

    if (const auto* program = dynamic_cast<const Program*>(node); program != nullptr) {
        return eval_program(program, environment);
    }

    if (const auto* statement = dynamic_cast<const ExpressionStatement*>(node); statement != nullptr) {
        return eval_node(statement->expression.get(), environment);
    }

    if (const auto* statement = dynamic_cast<const LetStatement*>(node); statement != nullptr) {
        const auto value = eval_node(statement->value.get(), environment);
        if (is_error(value)) {
            return value;
        }

        environment.set(statement->name.literal, value);

        if (const auto* function = dynamic_cast<const FunctionObject*>(value.get()); function != nullptr) {
            function->env->set(statement->name.literal, value);
        }

        return nullObject();
    }

    if (const auto* statement = dynamic_cast<const ReturnStatement*>(node); statement != nullptr) {
        auto result = std::make_shared<ReturnValueObject>();
        result->value = eval_node(statement->return_value.get(), environment);
        if (is_error(result->value)) {
            return result->value;
        }
        return result;
    }

    if (const auto* block = dynamic_cast<const BlockStatement*>(node); block != nullptr) {
        return eval_block_statement(block, environment);
    }

    if (const auto* prefix_expression = dynamic_cast<const PrefixExpression*>(node); prefix_expression != nullptr) {
        const auto right = eval_node(prefix_expression->right.get(), environment);
        if (is_error(right)) {
            return right;
        }
        return eval_prefix_expression(prefix_expression->op, right);
    }

    if (const auto* infix_expression = dynamic_cast<const InfixExpression*>(node); infix_expression != nullptr) {
        const auto left = eval_node(infix_expression->left.get(), environment);
        if (is_error(left)) {
            return left;
        }
        const auto right = eval_node(infix_expression->right.get(), environment);
        if (is_error(right)) {
            return right;
        }
        return eval_infix_expression(infix_expression->op, left, right);
    }

    if (const auto* call_expression = dynamic_cast<const CallExpression*>(node); call_expression != nullptr) {
        const auto function = eval_node(call_expression->function.get(), environment);
        if (is_error(function)) {
            return function;
        }

        const auto arguments = eval_expressions(call_expression->arguments, environment);
        if (arguments.size() == 1 && is_error(arguments[0])) {
            return arguments[0];
        }

        return apply_function(function, arguments);
    }

    if (const auto* index_expression = dynamic_cast<const IndexExpression*>(node); index_expression != nullptr) {
        const auto left = eval_node(index_expression->left.get(), environment);
        if (is_error(left)) {
            return left;
        }

        const auto index = eval_node(index_expression->index.get(), environment);
        if (is_error(index)) {
            return index;
        }

        return eval_index_expression(left, index);
    }

    if (const auto* identifier = dynamic_cast<const Identifier*>(node); identifier != nullptr) {
        const auto value = environment.get(identifier->value);
        if (value != nullptr) {
            return value;
        }

        const auto builtin = get_builtin_by_name(identifier->value);
        if (builtin != nullptr) {
            return builtin;
        }

        return new_error("identifier not found: " + identifier->value);
    }

    if (const auto* integer_literal = dynamic_cast<const IntegerLiteral*>(node); integer_literal != nullptr) {
        auto integer = std::make_shared<IntegerObject>();
        integer->value = integer_literal->value;
        return integer;
    }

    if (const auto* boolean = dynamic_cast<const Boolean*>(node); boolean != nullptr) {
        return nativeBoolToBooleanObject(boolean->value);
    }

    if (const auto* string_literal = dynamic_cast<const StringLiteral*>(node); string_literal != nullptr) {
        auto string = std::make_shared<StringObject>();
        string->value = string_literal->value;
        return string;
    }

    if (const auto* array_literal = dynamic_cast<const ArrayLiteral*>(node); array_literal != nullptr) {
        const auto elements = eval_expressions(array_literal->elements, environment);
        if (elements.size() == 1 && is_error(elements[0])) {
            return elements[0];
        }

        auto array = std::make_shared<ArrayObject>();
        array->elements = elements;
        return array;
    }

    if (const auto* hash_literal = dynamic_cast<const HashLiteral*>(node); hash_literal != nullptr) {
        return eval_hash_literal(hash_literal, environment);
    }

    if (const auto* if_expression = dynamic_cast<const IfExpression*>(node); if_expression != nullptr) {
        return eval_if_expression(if_expression, environment);
    }

    if (const auto* function_literal = dynamic_cast<const FunctionLiteral*>(node); function_literal != nullptr) {
        auto function = std::make_shared<FunctionObject>();
        function->env = std::make_shared<Environment>(environment);

        for (const auto& parameter : function_literal->parameters) {
            if (parameter != nullptr) {
                auto parameter_clone = std::make_unique<Identifier>();
                parameter_clone->token = parameter->token;
                parameter_clone->value = parameter->value;
                function->parameters.push_back(std::move(parameter_clone));
            }
        }

        if (function_literal->body != nullptr) {
            function->body = clone_block_statement(*function_literal->body);
        }

        return function;
    }

    return nullObject();
}

auto eval(const Node* node, Environment& environment) -> std::shared_ptr<Object> {
    return eval_node(node, environment);
}

auto eval(const Node* node) -> std::shared_ptr<Object> {
    Environment environment;
    return eval_node(node, environment);
}
