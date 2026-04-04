#include "monkey/evaluator.hpp"
#include "monkey/environment.hpp"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

[[nodiscard]] auto eval_node(const Node* node, Environment& environment) -> std::shared_ptr<Object>;

namespace {

constexpr std::array<std::string_view, 5> builtin_names {
    "len",
    "first",
    "last",
    "rest",
    "push",
};

struct SymbolBinding {
    SymbolRef symbol {};
    bool ready {false};
};

struct SymbolTable {
    SymbolTable* outer {nullptr};
    std::unordered_map<std::string, SymbolBinding> bindings {};
    std::size_t next_index {0};
};

void resolve_statement(const Statement* statement, SymbolTable& table);
void resolve_expression(const Expression* expression, SymbolTable& table);

[[nodiscard]] auto define_symbol(SymbolTable& table, std::string_view name) -> SymbolRef {
    if (const auto existing = table.bindings.find(std::string(name));
        existing != table.bindings.end() && existing->second.symbol.scope != SymbolScope::Builtin) {
        return existing->second.symbol;
    }

    const auto scope = table.outer == nullptr ? SymbolScope::Global : SymbolScope::Local;
    const auto symbol = SymbolRef {.scope = scope, .depth = 0, .index = table.next_index++};
    table.bindings[std::string(name)] = SymbolBinding {.symbol = symbol, .ready = false};
    return symbol;
}

void define_builtin(SymbolTable& table, std::string_view name, std::size_t index) {
    table.bindings[std::string(name)] = SymbolBinding {
        .symbol = SymbolRef {.scope = SymbolScope::Builtin, .depth = 0, .index = index},
        .ready = true,
    };
}

void mark_symbol_ready(SymbolTable& table, std::string_view name) {
    if (const auto binding = table.bindings.find(std::string(name)); binding != table.bindings.end()) {
        binding->second.ready = true;
    }
}

[[nodiscard]] auto resolve_symbol(
    const SymbolTable& table,
    std::string_view name,
    std::size_t depth = 0
) -> std::optional<SymbolRef> {
    if (const auto binding = table.bindings.find(std::string(name)); binding != table.bindings.end()) {
        if (!binding->second.ready &&
            binding->second.symbol.scope != SymbolScope::Builtin &&
            depth == 0) {
            if (table.outer != nullptr) {
                return resolve_symbol(*table.outer, name, depth + 1);
            }
            return std::nullopt;
        }

        auto symbol = binding->second.symbol;
        if (symbol.scope == SymbolScope::Local) {
            symbol.depth = depth;
        }
        return symbol;
    }

    if (table.outer != nullptr) {
        return resolve_symbol(*table.outer, name, depth + 1);
    }

    return std::nullopt;
}

void resolve_program(const Program* program) {
    if (program == nullptr || program->symbols_resolved) {
        return;
    }

    SymbolTable table {};
    for (std::size_t index = 0; index < builtin_names.size(); ++index) {
        define_builtin(table, builtin_names[index], index);
    }

    for (const auto& statement : program->statements) {
        if (statement != nullptr) {
            resolve_statement(statement.get(), table);
        }
    }

    program->symbols_resolved = true;
}

void resolve_block_statement(const BlockStatement* block, SymbolTable& table) {
    if (block == nullptr) {
        return;
    }

    for (const auto& statement : block->statements) {
        if (statement != nullptr) {
            resolve_statement(statement.get(), table);
        }
    }
}

void resolve_statement(const Statement* statement, SymbolTable& table) {
    if (statement == nullptr) {
        return;
    }

    switch (statement->kind()) {
        case NodeType::ExpressionStatement: {
            const auto* expression_statement = static_cast<const ExpressionStatement*>(statement);
            resolve_expression(expression_statement->expression.get(), table);
            return;
        }
        case NodeType::LetStatement: {
            const auto* let_statement = static_cast<const LetStatement*>(statement);
            let_statement->symbol = define_symbol(table, let_statement->name.literal);
            resolve_expression(let_statement->value.get(), table);
            mark_symbol_ready(table, let_statement->name.literal);
            return;
        }
        case NodeType::ReturnStatement: {
            const auto* return_statement = static_cast<const ReturnStatement*>(statement);
            resolve_expression(return_statement->return_value.get(), table);
            return;
        }
        case NodeType::BlockStatement:
            resolve_block_statement(static_cast<const BlockStatement*>(statement), table);
            return;
        default:
            return;
    }
}

void resolve_expression(const Expression* expression, SymbolTable& table) {
    if (expression == nullptr) {
        return;
    }

    switch (expression->kind()) {
        case NodeType::Identifier: {
            const auto* identifier = static_cast<const Identifier*>(expression);
            identifier->symbol = resolve_symbol(table, identifier->value).value_or(SymbolRef {});
            return;
        }
        case NodeType::ArrayLiteral: {
            const auto* array = static_cast<const ArrayLiteral*>(expression);
            for (const auto& element : array->elements) {
                if (element != nullptr) {
                    resolve_expression(element.get(), table);
                }
            }
            return;
        }
        case NodeType::HashLiteral: {
            const auto* hash = static_cast<const HashLiteral*>(expression);
            for (const auto& [key, value] : hash->pairs) {
                resolve_expression(key.get(), table);
                resolve_expression(value.get(), table);
            }
            return;
        }
        case NodeType::PrefixExpression: {
            const auto* prefix = static_cast<const PrefixExpression*>(expression);
            resolve_expression(prefix->right.get(), table);
            return;
        }
        case NodeType::InfixExpression: {
            const auto* infix = static_cast<const InfixExpression*>(expression);
            resolve_expression(infix->left.get(), table);
            resolve_expression(infix->right.get(), table);
            return;
        }
        case NodeType::IfExpression: {
            const auto* if_expression = static_cast<const IfExpression*>(expression);
            resolve_expression(if_expression->condition.get(), table);
            resolve_block_statement(if_expression->consequence.get(), table);
            resolve_block_statement(if_expression->alternative.get(), table);
            return;
        }
        case NodeType::FunctionLiteral: {
            const auto* function = static_cast<const FunctionLiteral*>(expression);
            SymbolTable function_table {.outer = &table};

            for (const auto& parameter : function->parameters) {
                if (parameter != nullptr) {
                    parameter->symbol = define_symbol(function_table, parameter->value);
                    mark_symbol_ready(function_table, parameter->value);
                }
            }

            resolve_block_statement(function->body.get(), function_table);
            return;
        }
        case NodeType::CallExpression: {
            const auto* call = static_cast<const CallExpression*>(expression);
            resolve_expression(call->function.get(), table);
            for (const auto& argument : call->arguments) {
                if (argument != nullptr) {
                    resolve_expression(argument.get(), table);
                }
            }
            return;
        }
        case NodeType::IndexExpression: {
            const auto* index = static_cast<const IndexExpression*>(expression);
            resolve_expression(index->left.get(), table);
            resolve_expression(index->index.get(), table);
            return;
        }
        default:
            return;
    }
}

[[nodiscard]] auto eval_hash_index_expression(
    const Object* hash,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object>;

[[nodiscard]] auto nativeBoolToBooleanObject(bool input) -> std::shared_ptr<Object> {
    static auto true_object = [] {
        return std::make_shared<Object>(Object::make_boolean(true));
    }();

    static auto false_object = [] {
        return std::make_shared<Object>(Object::make_boolean(false));
    }();

    return input ? true_object : false_object;
}

[[nodiscard]] auto nullObject() -> std::shared_ptr<Object> {
    static auto null_object = std::make_shared<Object>(Object::make_null());
    return null_object;
}

[[nodiscard]] auto new_error(std::string message) -> std::shared_ptr<Object> {
    return std::make_shared<Object>(Object::make_error(std::move(message)));
}

[[nodiscard]] auto new_integer_object(std::int64_t value) -> std::shared_ptr<Object> {
    return std::make_shared<Object>(Object::make_integer(value));
}

[[nodiscard]] auto builtin_len(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0]->type() == ObjectType::String) {
        return new_integer_object(static_cast<std::int64_t>(arguments[0]->string_value().size()));
    }

    if (arguments[0]->type() == ObjectType::Array) {
        return new_integer_object(static_cast<std::int64_t>(arguments[0]->array_elements().size()));
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

    if (arguments[0]->type() != ObjectType::Array) {
        return new_error(
            "argument to `first` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    const auto& elements = arguments[0]->array_elements();
    if (elements.empty()) {
        return nullObject();
    }

    return elements.front();
}

[[nodiscard]] auto builtin_last(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0]->type() != ObjectType::Array) {
        return new_error(
            "argument to `last` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    const auto& elements = arguments[0]->array_elements();
    if (elements.empty()) {
        return nullObject();
    }

    return elements.back();
}

[[nodiscard]] auto builtin_rest(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 1) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0]->type() != ObjectType::Array) {
        return new_error(
            "argument to `rest` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    const auto& elements = arguments[0]->array_elements();
    if (elements.empty()) {
        return nullObject();
    }

    std::vector<std::shared_ptr<Object>> rest(elements.begin() + 1, elements.end());
    return std::make_shared<Object>(Object::make_array(std::move(rest)));
}

[[nodiscard]] auto builtin_push(const std::vector<std::shared_ptr<Object>>& arguments)
    -> std::shared_ptr<Object> {
    if (arguments.size() != 2) {
        return new_error(
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=2"
        );
    }

    if (arguments[0]->type() != ObjectType::Array) {
        return new_error(
            "argument to `push` must be Array, got " +
            std::string(to_string(arguments[0]->type()))
        );
    }

    auto elements = arguments[0]->array_elements();
    elements.push_back(arguments[1]);
    return std::make_shared<Object>(Object::make_array(std::move(elements)));
}

[[nodiscard]] auto get_builtin_by_index(std::size_t index) -> std::shared_ptr<Object> {
    static const std::array<std::shared_ptr<Object>, builtin_names.size()> builtins {
        std::make_shared<Object>(Object::make_builtin(builtin_len)),
        std::make_shared<Object>(Object::make_builtin(builtin_first)),
        std::make_shared<Object>(Object::make_builtin(builtin_last)),
        std::make_shared<Object>(Object::make_builtin(builtin_rest)),
        std::make_shared<Object>(Object::make_builtin(builtin_push)),
    };

    if (index >= builtins.size()) {
        return nullptr;
    }

    return builtins[index];
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

        if (result->type() == ObjectType::ReturnValue) {
            return result->return_value();
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
    if (right->type() != ObjectType::Integer) {
        return new_error("unknown operator: -" + std::string(to_string(right->type())));
    }

    return std::make_shared<Object>(Object::make_integer(-right->integer_value()));
}

[[nodiscard]] auto eval_prefix_expression(
    EvalOperator op,
    const std::shared_ptr<Object>& right
) -> std::shared_ptr<Object> {
    switch (op) {
        case EvalOperator::Bang:
            return eval_bang_operator_expression(right);
        case EvalOperator::Minus:
            return eval_minus_prefix_operator_expression(right);
        default:
            return new_error(
                "unknown operator: " + std::string(to_string(op)) + std::string(to_string(right->type()))
            );
    }
}

[[nodiscard]] auto eval_integer_infix_expression(
    EvalOperator op,
    const Object* left,
    const Object* right
) -> std::shared_ptr<Object> {
    switch (op) {
        case EvalOperator::Plus:
            return new_integer_object(left->integer_value() + right->integer_value());
        case EvalOperator::Minus:
            return new_integer_object(left->integer_value() - right->integer_value());
        case EvalOperator::Multiply:
            return new_integer_object(left->integer_value() * right->integer_value());
        case EvalOperator::Divide:
            return new_integer_object(left->integer_value() / right->integer_value());
        case EvalOperator::LessThan:
            return nativeBoolToBooleanObject(left->integer_value() < right->integer_value());
        case EvalOperator::GreaterThan:
            return nativeBoolToBooleanObject(left->integer_value() > right->integer_value());
        case EvalOperator::Equal:
            return nativeBoolToBooleanObject(left->integer_value() == right->integer_value());
        case EvalOperator::NotEqual:
            return nativeBoolToBooleanObject(left->integer_value() != right->integer_value());
        default:
            return new_error(
                "unknown operator: " +
                std::string(to_string(left->type())) + " " +
                std::string(to_string(op)) + " " +
                std::string(to_string(right->type()))
            );
    }
}

[[nodiscard]] auto eval_string_infix_expression(
    EvalOperator op,
    const Object* left,
    const Object* right
) -> std::shared_ptr<Object> {
    if (op == EvalOperator::Plus) {
        return std::make_shared<Object>(Object::make_string(left->string_value() + right->string_value()));
    }

    return new_error(
        "unknown operator: " +
        std::string(to_string(left->type())) + " " +
        std::string(to_string(op)) + " " +
        std::string(to_string(right->type()))
    );
}

[[nodiscard]] auto eval_infix_expression(
    EvalOperator op,
    const std::shared_ptr<Object>& left,
    const std::shared_ptr<Object>& right
) -> std::shared_ptr<Object> {
    if (left->type() == ObjectType::Integer && right->type() == ObjectType::Integer) {
        return eval_integer_infix_expression(op, left.get(), right.get());
    }

    if (left->type() == ObjectType::String && right->type() == ObjectType::String) {
        return eval_string_infix_expression(op, left.get(), right.get());
    }

    if (op == EvalOperator::Equal) {
        return nativeBoolToBooleanObject(left == right);
    }

    if (op == EvalOperator::NotEqual) {
        return nativeBoolToBooleanObject(left != right);
    }

    if (left->type() != right->type()) {
        return new_error(
            "type mismatch: " +
            std::string(to_string(left->type())) + " " +
            std::string(to_string(op)) + " " +
            std::string(to_string(right->type()))
        );
    }

    return new_error(
        "unknown operator: " +
        std::string(to_string(left->type())) + " " +
        std::string(to_string(op)) + " " +
        std::string(to_string(right->type()))
    );
}

[[nodiscard]] auto eval_array_index_expression(
    const Object* array,
    const Object* index
) -> std::shared_ptr<Object> {
    if (index->integer_value() < 0) {
        return nullObject();
    }

    const auto max_index = static_cast<std::int64_t>(array->array_elements().size()) - 1;
    if (index->integer_value() > max_index) {
        return nullObject();
    }

    return array->array_elements()[static_cast<std::size_t>(index->integer_value())];
}

[[nodiscard]] auto eval_index_expression(
    const std::shared_ptr<Object>& left,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object> {
    if (left->type() == ObjectType::Array && index->type() == ObjectType::Integer) {
        return eval_array_index_expression(left.get(), index.get());
    }

    if (left->type() == ObjectType::Hash) {
        return eval_hash_index_expression(left.get(), index);
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
    std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;

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

        pairs[*hash_key] = HashPair {key, value};
    }

    return std::make_shared<Object>(Object::make_hash(std::move(pairs)));
}

[[nodiscard]] auto eval_hash_index_expression(
    const Object* hash,
    const std::shared_ptr<Object>& index
) -> std::shared_ptr<Object> {
    const auto hash_key = hash_key_for_object(index);
    if (!hash_key.has_value()) {
        return new_error("unusable as hash key: " + std::string(to_string(index->type())));
    }

    const auto iterator = hash->hash_pairs().find(*hash_key);
    if (iterator == hash->hash_pairs().end()) {
        return nullObject();
    }

    return iterator->second.value;
}

[[nodiscard]] auto apply_function(
    const std::shared_ptr<Object>& function,
    const std::vector<std::shared_ptr<Object>>& arguments
) -> std::shared_ptr<Object> {
    if (function->type() == ObjectType::Builtin) {
        return function->builtin_function()(arguments);
    }

    if (function->type() != ObjectType::Function) {
        return new_error("not a function: " + std::string(to_string(function->type())));
    }

    auto extended_environment = new_enclosed_environment(function->function_env());
    for (std::size_t index = 0; index < function->function_parameters().size(); ++index) {
        if (function->function_parameters()[index] != nullptr && index < arguments.size()) {
            extended_environment->set_at(index, arguments[index]);
        }
    }

    auto evaluated = eval_node(function->function_body(), *extended_environment);
    if (evaluated->type() == ObjectType::ReturnValue) {
        return evaluated->return_value();
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

    switch (node->kind()) {
        case NodeType::Program:
            return eval_program(static_cast<const Program*>(node), environment);
        case NodeType::ExpressionStatement: {
            const auto* statement = static_cast<const ExpressionStatement*>(node);
            return eval_node(statement->expression.get(), environment);
        }
        case NodeType::LetStatement: {
            const auto* statement = static_cast<const LetStatement*>(node);
            const auto value = eval_node(statement->value.get(), environment);
            if (is_error(value)) {
                return value;
            }

            if (!statement->symbol.resolved()) {
                return new_error("internal error: unresolved let binding");
            }

            switch (statement->symbol.scope) {
                case SymbolScope::Global:
                    environment.set_root_at(statement->symbol.index, value);
                    break;
                case SymbolScope::Local:
                    environment.set_at(statement->symbol.index, value);
                    break;
                default:
                    return new_error("internal error: invalid let symbol scope");
            }

            if (value->type() == ObjectType::Function) {
                switch (statement->symbol.scope) {
                    case SymbolScope::Global:
                        value->function_env_mut()->set_root_at(statement->symbol.index, value);
                        break;
                    case SymbolScope::Local:
                        value->function_env_mut()->set_at(statement->symbol.index, value);
                        break;
                    default:
                        return new_error("internal error: invalid function symbol scope");
                }
            }

            return nullObject();
        }
        case NodeType::ReturnStatement: {
            const auto* statement = static_cast<const ReturnStatement*>(node);
            const auto value = eval_node(statement->return_value.get(), environment);
            if (is_error(value)) {
                return value;
            }
            return std::make_shared<Object>(Object::make_return(value));
        }
        case NodeType::BlockStatement:
            return eval_block_statement(static_cast<const BlockStatement*>(node), environment);
        case NodeType::PrefixExpression: {
            const auto* prefix_expression = static_cast<const PrefixExpression*>(node);
            const auto right = eval_node(prefix_expression->right.get(), environment);
            if (is_error(right)) {
                return right;
            }
            return eval_prefix_expression(prefix_expression->op, right);
        }
        case NodeType::InfixExpression: {
            const auto* infix_expression = static_cast<const InfixExpression*>(node);
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
        case NodeType::CallExpression: {
            const auto* call_expression = static_cast<const CallExpression*>(node);
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
        case NodeType::IndexExpression: {
            const auto* index_expression = static_cast<const IndexExpression*>(node);
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
        case NodeType::Identifier: {
            const auto* identifier = static_cast<const Identifier*>(node);
            if (identifier->symbol.resolved()) {
                switch (identifier->symbol.scope) {
                    case SymbolScope::Global: {
                        const auto value = environment.get_root_at(identifier->symbol.index);
                        if (value != nullptr) {
                            return value;
                        }
                        break;
                    }
                    case SymbolScope::Local: {
                        const auto value = environment.get_at(identifier->symbol.depth, identifier->symbol.index);
                        if (value != nullptr) {
                            return value;
                        }
                        break;
                    }
                    case SymbolScope::Builtin: {
                        const auto builtin = get_builtin_by_index(identifier->symbol.index);
                        if (builtin != nullptr) {
                            return builtin;
                        }
                        break;
                    }
                    case SymbolScope::Unresolved:
                        break;
                }
            }

            return new_error("identifier not found: " + identifier->value);
        }
        case NodeType::IntegerLiteral: {
            const auto* integer_literal = static_cast<const IntegerLiteral*>(node);
            return std::make_shared<Object>(Object::make_integer(integer_literal->value));
        }
        case NodeType::Boolean: {
            const auto* boolean = static_cast<const Boolean*>(node);
            return nativeBoolToBooleanObject(boolean->value);
        }
        case NodeType::StringLiteral: {
            const auto* string_literal = static_cast<const StringLiteral*>(node);
            return std::make_shared<Object>(Object::make_string(string_literal->value));
        }
        case NodeType::ArrayLiteral: {
            const auto* array_literal = static_cast<const ArrayLiteral*>(node);
            const auto elements = eval_expressions(array_literal->elements, environment);
            if (elements.size() == 1 && is_error(elements[0])) {
                return elements[0];
            }

            return std::make_shared<Object>(Object::make_array(elements));
        }
        case NodeType::HashLiteral:
            return eval_hash_literal(static_cast<const HashLiteral*>(node), environment);
        case NodeType::IfExpression:
            return eval_if_expression(static_cast<const IfExpression*>(node), environment);
        case NodeType::FunctionLiteral: {
            const auto* function_literal = static_cast<const FunctionLiteral*>(node);
            std::vector<std::unique_ptr<Identifier>> parameters;
            for (const auto& parameter : function_literal->parameters) {
                if (parameter != nullptr) {
                    auto parameter_clone = std::make_unique<Identifier>();
                    parameter_clone->token = parameter->token;
                    parameter_clone->value = parameter->value;
                    parameters.push_back(std::move(parameter_clone));
                }
            }

            auto body = function_literal->body != nullptr
                ? clone_block_statement(*function_literal->body)
                : nullptr;

            return std::make_shared<Object>(Object::make_function(
                std::move(parameters),
                std::move(body),
                std::make_shared<Environment>(environment)
            ));
        }
        default:
            return nullObject();
    }
}

auto eval(const Node* node, Environment& environment) -> std::shared_ptr<Object> {
    if (node != nullptr && node->kind() == NodeType::Program) {
        resolve_program(static_cast<const Program*>(node));
    }

    return eval_node(node, environment);
}

auto eval(const Node* node) -> std::shared_ptr<Object> {
    Environment environment;

    if (node != nullptr && node->kind() == NodeType::Program) {
        resolve_program(static_cast<const Program*>(node));
    }

    return eval_node(node, environment);
}
