#include "monkey/evaluator.hpp"
#include "monkey/environment.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

[[nodiscard]] auto eval_node(const Node* node, Environment& environment, ScratchArena& scratch) -> Value;

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

[[nodiscard]] auto null_value() -> Value {
    return Value::make_null();
}

[[nodiscard]] auto native_bool_to_value(bool input) -> Value {
    return Value::make_boolean(input);
}

[[nodiscard]] auto is_error(const Value& value) -> bool {
    return value.type() == ObjectType::Error;
}

[[nodiscard]] auto new_error(ScratchArena& scratch, std::string message) -> Value {
    return scratch.allocate_error(std::move(message));
}

[[nodiscard]] auto builtin_len(ScratchArena& scratch, const std::vector<Value>& arguments) -> Value {
    if (arguments.size() != 1) {
        return new_error(
            scratch,
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0].type() == ObjectType::String) {
        return Value::make_integer(
            static_cast<std::int64_t>(arguments[0].heap_object()->string_value().size())
        );
    }

    if (arguments[0].type() == ObjectType::Array) {
        return Value::make_integer(
            static_cast<std::int64_t>(arguments[0].heap_object()->array_elements().size())
        );
    }

    return new_error(
        scratch,
        "argument to `len` not supported, got " + std::string(to_string(arguments[0].type()))
    );
}

[[nodiscard]] auto builtin_first(ScratchArena& scratch, const std::vector<Value>& arguments) -> Value {
    if (arguments.size() != 1) {
        return new_error(
            scratch,
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0].type() != ObjectType::Array) {
        return new_error(
            scratch,
            "argument to `first` must be Array, got " +
                std::string(to_string(arguments[0].type()))
        );
    }

    const auto& elements = arguments[0].heap_object()->array_elements();
    if (elements.empty()) {
        return null_value();
    }

    return elements.front();
}

[[nodiscard]] auto builtin_last(ScratchArena& scratch, const std::vector<Value>& arguments) -> Value {
    if (arguments.size() != 1) {
        return new_error(
            scratch,
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0].type() != ObjectType::Array) {
        return new_error(
            scratch,
            "argument to `last` must be Array, got " +
                std::string(to_string(arguments[0].type()))
        );
    }

    const auto& elements = arguments[0].heap_object()->array_elements();
    if (elements.empty()) {
        return null_value();
    }

    return elements.back();
}

[[nodiscard]] auto builtin_rest(ScratchArena& scratch, const std::vector<Value>& arguments) -> Value {
    if (arguments.size() != 1) {
        return new_error(
            scratch,
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=1"
        );
    }

    if (arguments[0].type() != ObjectType::Array) {
        return new_error(
            scratch,
            "argument to `rest` must be Array, got " +
                std::string(to_string(arguments[0].type()))
        );
    }

    const auto& elements = arguments[0].heap_object()->array_elements();
    if (elements.empty()) {
        return null_value();
    }

    std::vector<Value> rest(elements.begin() + 1, elements.end());
    return scratch.allocate_array(std::move(rest));
}

[[nodiscard]] auto builtin_push(ScratchArena& scratch, const std::vector<Value>& arguments) -> Value {
    if (arguments.size() != 2) {
        return new_error(
            scratch,
            "wrong number of arguments. got=" + std::to_string(arguments.size()) + ", want=2"
        );
    }

    if (arguments[0].type() != ObjectType::Array) {
        return new_error(
            scratch,
            "argument to `push` must be Array, got " +
                std::string(to_string(arguments[0].type()))
        );
    }

    auto elements = arguments[0].heap_object()->array_elements();
    elements.push_back(arguments[1]);
    return scratch.allocate_array(std::move(elements));
}

[[nodiscard]] auto get_builtin_by_index(std::size_t index) -> Value {
    switch (index) {
        case 0: {
            static auto builtin = Object::make_builtin(builtin_len, ObjectLifetime::Static);
            return Value::from_object(&builtin);
        }
        case 1: {
            static auto builtin = Object::make_builtin(builtin_first, ObjectLifetime::Static);
            return Value::from_object(&builtin);
        }
        case 2: {
            static auto builtin = Object::make_builtin(builtin_last, ObjectLifetime::Static);
            return Value::from_object(&builtin);
        }
        case 3: {
            static auto builtin = Object::make_builtin(builtin_rest, ObjectLifetime::Static);
            return Value::from_object(&builtin);
        }
        case 4: {
            static auto builtin = Object::make_builtin(builtin_push, ObjectLifetime::Static);
            return Value::from_object(&builtin);
        }
        default:
            return Value {};
    }
}

[[nodiscard]] auto eval_program(
    const Program* program,
    Environment& environment,
    ScratchArena& scratch
) -> Value {
    auto result = null_value();

    for (const auto& statement : program->statements) {
        result = eval_node(statement.get(), environment, scratch);

        if (is_error(result)) {
            return result;
        }

        if (result.type() == ObjectType::ReturnValue) {
            return result.heap_object()->return_value();
        }
    }

    return result;
}

[[nodiscard]] auto eval_block_statement(
    const BlockStatement* block,
    Environment& environment,
    ScratchArena& scratch
) -> Value {
    auto result = null_value();

    for (const auto& statement : block->statements) {
        result = eval_node(statement.get(), environment, scratch);

        if (result.type() == ObjectType::ReturnValue || result.type() == ObjectType::Error) {
            return result;
        }
    }

    return result;
}

[[nodiscard]] auto is_truthy(const Value& value) -> bool {
    if (value.type() == ObjectType::Null) {
        return false;
    }

    if (value.type() == ObjectType::Boolean) {
        return value.boolean_value();
    }

    return true;
}

[[nodiscard]] auto eval_if_expression(
    const IfExpression* expression,
    Environment& environment,
    ScratchArena& scratch
) -> Value {
    const auto condition = eval_node(expression->condition.get(), environment, scratch);
    if (is_error(condition)) {
        return condition;
    }

    if (is_truthy(condition)) {
        return eval_node(expression->consequence.get(), environment, scratch);
    }

    if (expression->alternative != nullptr) {
        return eval_node(expression->alternative.get(), environment, scratch);
    }

    return null_value();
}

[[nodiscard]] auto eval_bang_operator_expression(const Value& right) -> Value {
    if (right.type() == ObjectType::Boolean) {
        return native_bool_to_value(!right.boolean_value());
    }

    if (right.type() == ObjectType::Null) {
        return native_bool_to_value(true);
    }

    return native_bool_to_value(false);
}

[[nodiscard]] auto eval_minus_prefix_operator_expression(
    ScratchArena& scratch,
    const Value& right
) -> Value {
    if (right.type() != ObjectType::Integer) {
        return new_error(scratch, "unknown operator: -" + std::string(to_string(right.type())));
    }

    return Value::make_integer(-right.integer_value());
}

[[nodiscard]] auto eval_prefix_expression(
    ScratchArena& scratch,
    EvalOperator op,
    const Value& right
) -> Value {
    switch (op) {
        case EvalOperator::Bang:
            return eval_bang_operator_expression(right);
        case EvalOperator::Minus:
            return eval_minus_prefix_operator_expression(scratch, right);
        default:
            return new_error(
                scratch,
                "unknown operator: " + std::string(to_string(op)) +
                    std::string(to_string(right.type()))
            );
    }
}

[[nodiscard]] auto eval_integer_infix_expression(
    ScratchArena& scratch,
    EvalOperator op,
    const Value& left,
    const Value& right
) -> Value {
    switch (op) {
        case EvalOperator::Plus:
            return Value::make_integer(left.integer_value() + right.integer_value());
        case EvalOperator::Minus:
            return Value::make_integer(left.integer_value() - right.integer_value());
        case EvalOperator::Multiply:
            return Value::make_integer(left.integer_value() * right.integer_value());
        case EvalOperator::Divide:
            return Value::make_integer(left.integer_value() / right.integer_value());
        case EvalOperator::LessThan:
            return native_bool_to_value(left.integer_value() < right.integer_value());
        case EvalOperator::GreaterThan:
            return native_bool_to_value(left.integer_value() > right.integer_value());
        case EvalOperator::Equal:
            return native_bool_to_value(left.integer_value() == right.integer_value());
        case EvalOperator::NotEqual:
            return native_bool_to_value(left.integer_value() != right.integer_value());
        default:
            return new_error(
                scratch,
                "unknown operator: " + std::string(to_string(left.type())) + " " +
                    std::string(to_string(op)) + " " +
                    std::string(to_string(right.type()))
            );
    }
}

[[nodiscard]] auto eval_string_infix_expression(
    ScratchArena& scratch,
    EvalOperator op,
    const Value& left,
    const Value& right
) -> Value {
    if (op == EvalOperator::Plus) {
        return scratch.allocate_string(
            left.heap_object()->string_value() + right.heap_object()->string_value()
        );
    }

    return new_error(
        scratch,
        "unknown operator: " + std::string(to_string(left.type())) + " " +
            std::string(to_string(op)) + " " +
            std::string(to_string(right.type()))
    );
}

[[nodiscard]] auto eval_infix_expression(
    ScratchArena& scratch,
    EvalOperator op,
    const Value& left,
    const Value& right
) -> Value {
    if (left.type() == ObjectType::Integer && right.type() == ObjectType::Integer) {
        return eval_integer_infix_expression(scratch, op, left, right);
    }

    if (left.type() == ObjectType::String && right.type() == ObjectType::String) {
        return eval_string_infix_expression(scratch, op, left, right);
    }

    if (op == EvalOperator::Equal) {
        if (left.is_immediate() || right.is_immediate()) {
            if (left.type() != right.type()) {
                return native_bool_to_value(false);
            }

            switch (left.type()) {
                case ObjectType::Null:
                    return native_bool_to_value(true);
                case ObjectType::Boolean:
                    return native_bool_to_value(left.boolean_value() == right.boolean_value());
                default:
                    return native_bool_to_value(false);
            }
        }

        return native_bool_to_value(left.heap_object() == right.heap_object());
    }

    if (op == EvalOperator::NotEqual) {
        if (left.is_immediate() || right.is_immediate()) {
            if (left.type() != right.type()) {
                return native_bool_to_value(true);
            }

            switch (left.type()) {
                case ObjectType::Null:
                    return native_bool_to_value(false);
                case ObjectType::Boolean:
                    return native_bool_to_value(left.boolean_value() != right.boolean_value());
                default:
                    return native_bool_to_value(true);
            }
        }

        return native_bool_to_value(left.heap_object() != right.heap_object());
    }

    if (left.type() != right.type()) {
        return new_error(
            scratch,
            "type mismatch: " + std::string(to_string(left.type())) + " " +
                std::string(to_string(op)) + " " +
                std::string(to_string(right.type()))
        );
    }

    return new_error(
        scratch,
        "unknown operator: " + std::string(to_string(left.type())) + " " +
            std::string(to_string(op)) + " " +
            std::string(to_string(right.type()))
    );
}

[[nodiscard]] auto eval_array_index_expression(const Object* array, const Value& index) -> Value {
    if (index.integer_value() < 0) {
        return null_value();
    }

    const auto max_index = static_cast<std::int64_t>(array->array_elements().size()) - 1;
    if (index.integer_value() > max_index) {
        return null_value();
    }

    return array->array_elements()[static_cast<std::size_t>(index.integer_value())];
}

[[nodiscard]] auto hash_key_for_value(const Value& value) -> std::optional<HashKey> {
    return value.hash_key();
}

[[nodiscard]] auto eval_hash_index_expression(
    ScratchArena& scratch,
    const Object* hash,
    const Value& index
) -> Value {
    const auto hash_key = hash_key_for_value(index);
    if (!hash_key.has_value()) {
        return new_error(
            scratch,
            "unusable as hash key: " + std::string(to_string(index.type()))
        );
    }

    const auto iterator = hash->hash_pairs().find(*hash_key);
    if (iterator == hash->hash_pairs().end()) {
        return null_value();
    }

    return iterator->second.value;
}

[[nodiscard]] auto eval_index_expression(
    ScratchArena& scratch,
    const Value& left,
    const Value& index
) -> Value {
    if (left.type() == ObjectType::Array && index.type() == ObjectType::Integer) {
        return eval_array_index_expression(left.heap_object(), index);
    }

    if (left.type() == ObjectType::Hash) {
        return eval_hash_index_expression(scratch, left.heap_object(), index);
    }

    return new_error(
        scratch,
        "index operator not supported: " + std::string(to_string(left.type()))
    );
}

[[nodiscard]] auto eval_hash_literal(
    const HashLiteral* hash_literal,
    Environment& environment,
    ScratchArena& scratch
) -> Value {
    std::unordered_map<HashKey, HashPair, HashKeyHasher> pairs;

    for (const auto& [key_node, value_node] : hash_literal->pairs) {
        const auto key = eval_node(key_node.get(), environment, scratch);
        if (is_error(key)) {
            return key;
        }

        const auto hash_key = hash_key_for_value(key);
        if (!hash_key.has_value()) {
            return new_error(
                scratch,
                "unusable as hash key: " + std::string(to_string(key.type()))
            );
        }

        const auto value = eval_node(value_node.get(), environment, scratch);
        if (is_error(value)) {
            return value;
        }

        pairs[*hash_key] = HashPair {key, value};
    }

    return scratch.allocate_hash(std::move(pairs));
}

[[nodiscard]] auto apply_function(
    const Value& function,
    const std::vector<Value>& arguments,
    ScratchArena& scratch
) -> Value {
    if (function.type() == ObjectType::Builtin) {
        return function.heap_object()->builtin_function()(scratch, arguments);
    }

    if (function.type() != ObjectType::Function) {
        return new_error(
            scratch,
            "not a function: " + std::string(to_string(function.type()))
        );
    }

    auto* function_environment = function.heap_object()->function_env();
    Environment extended_environment {
        function_environment->store(),
        function_environment,
        EnvironmentLifetime::Temporary,
    };

    for (std::size_t index = 0; index < function.heap_object()->function_parameters().size(); ++index) {
        if (index < arguments.size()) {
            extended_environment.set_at(index, arguments[index]);
        }
    }

    auto evaluated = eval_node(function.heap_object()->function_body(), extended_environment, scratch);
    if (evaluated.type() == ObjectType::ReturnValue) {
        evaluated = evaluated.heap_object()->return_value();
    }

    return promote(evaluated, function_environment->store());
}

[[nodiscard]] auto eval_expressions(
    const std::vector<std::unique_ptr<Expression>>& expressions,
    Environment& environment,
    ScratchArena& scratch
) -> std::vector<Value> {
    std::vector<Value> result;
    result.reserve(expressions.size());

    for (const auto& expression : expressions) {
        auto evaluated = eval_node(expression.get(), environment, scratch);
        if (is_error(evaluated)) {
            return {evaluated};
        }

        result.push_back(evaluated);
    }

    return result;
}

}  // namespace

auto eval_node(const Node* node, Environment& environment, ScratchArena& scratch) -> Value {
    if (node == nullptr) {
        return null_value();
    }

    switch (node->kind()) {
        case NodeType::Program:
            return eval_program(static_cast<const Program*>(node), environment, scratch);
        case NodeType::ExpressionStatement: {
            const auto* statement = static_cast<const ExpressionStatement*>(node);
            return eval_node(statement->expression.get(), environment, scratch);
        }
        case NodeType::LetStatement: {
            const auto* statement = static_cast<const LetStatement*>(node);
            const auto value = eval_node(statement->value.get(), environment, scratch);
            if (is_error(value)) {
                return value;
            }

            if (!statement->symbol.resolved()) {
                return new_error(scratch, "internal error: unresolved let binding");
            }

            auto stored_value = value;

            switch (statement->symbol.scope) {
                case SymbolScope::Global:
                    stored_value = promote(value, environment.store());
                    environment.set_root_at(statement->symbol.index, stored_value);
                    break;
                case SymbolScope::Local:
                    if (environment.is_persistent()) {
                        stored_value = promote(value, environment.store());
                    }
                    environment.set_at(statement->symbol.index, stored_value);
                    break;
                default:
                    return new_error(scratch, "internal error: invalid let symbol scope");
            }

            if (stored_value.type() == ObjectType::Function) {
                switch (statement->symbol.scope) {
                    case SymbolScope::Global:
                        stored_value.heap_object_mut()->function_env_mut()->set_root_at(
                            statement->symbol.index,
                            stored_value
                        );
                        break;
                    case SymbolScope::Local:
                        stored_value.heap_object_mut()->function_env_mut()->set_at(
                            statement->symbol.index,
                            stored_value
                        );
                        break;
                    default:
                        return new_error(scratch, "internal error: invalid function symbol scope");
                }
            }

            return null_value();
        }
        case NodeType::ReturnStatement: {
            const auto* statement = static_cast<const ReturnStatement*>(node);
            const auto value = eval_node(statement->return_value.get(), environment, scratch);
            if (is_error(value)) {
                return value;
            }
            return scratch.allocate_return(value);
        }
        case NodeType::BlockStatement:
            return eval_block_statement(static_cast<const BlockStatement*>(node), environment, scratch);
        case NodeType::PrefixExpression: {
            const auto* prefix_expression = static_cast<const PrefixExpression*>(node);
            const auto right = eval_node(prefix_expression->right.get(), environment, scratch);
            if (is_error(right)) {
                return right;
            }
            return eval_prefix_expression(scratch, prefix_expression->op, right);
        }
        case NodeType::InfixExpression: {
            const auto* infix_expression = static_cast<const InfixExpression*>(node);
            const auto left = eval_node(infix_expression->left.get(), environment, scratch);
            if (is_error(left)) {
                return left;
            }
            const auto right = eval_node(infix_expression->right.get(), environment, scratch);
            if (is_error(right)) {
                return right;
            }
            return eval_infix_expression(scratch, infix_expression->op, left, right);
        }
        case NodeType::CallExpression: {
            const auto* call_expression = static_cast<const CallExpression*>(node);
            const auto function = eval_node(call_expression->function.get(), environment, scratch);
            if (is_error(function)) {
                return function;
            }

            const auto arguments = eval_expressions(call_expression->arguments, environment, scratch);
            if (arguments.size() == 1 && is_error(arguments[0])) {
                return arguments[0];
            }

            return apply_function(function, arguments, scratch);
        }
        case NodeType::IndexExpression: {
            const auto* index_expression = static_cast<const IndexExpression*>(node);
            const auto left = eval_node(index_expression->left.get(), environment, scratch);
            if (is_error(left)) {
                return left;
            }

            const auto index = eval_node(index_expression->index.get(), environment, scratch);
            if (is_error(index)) {
                return index;
            }

            return eval_index_expression(scratch, left, index);
        }
        case NodeType::Identifier: {
            const auto* identifier = static_cast<const Identifier*>(node);
            if (identifier->symbol.resolved()) {
                switch (identifier->symbol.scope) {
                    case SymbolScope::Global: {
                        const auto value = environment.get_root_at(identifier->symbol.index);
                        if (value.has_value()) {
                            return *value;
                        }
                        break;
                    }
                    case SymbolScope::Local: {
                        const auto value = environment.get_at(
                            identifier->symbol.depth,
                            identifier->symbol.index
                        );
                        if (value.has_value()) {
                            return *value;
                        }
                        break;
                    }
                    case SymbolScope::Builtin: {
                        const auto builtin = get_builtin_by_index(identifier->symbol.index);
                        if (builtin.type() != ObjectType::Null) {
                            return builtin;
                        }
                        break;
                    }
                    case SymbolScope::Unresolved:
                        break;
                }
            }

            return new_error(scratch, "identifier not found: " + identifier->value);
        }
        case NodeType::IntegerLiteral: {
            const auto* integer_literal = static_cast<const IntegerLiteral*>(node);
            return Value::make_integer(integer_literal->value);
        }
        case NodeType::Boolean: {
            const auto* boolean = static_cast<const Boolean*>(node);
            return Value::make_boolean(boolean->value);
        }
        case NodeType::StringLiteral: {
            const auto* string_literal = static_cast<const StringLiteral*>(node);
            return scratch.allocate_string(string_literal->value);
        }
        case NodeType::ArrayLiteral: {
            const auto* array_literal = static_cast<const ArrayLiteral*>(node);
            const auto elements = eval_expressions(array_literal->elements, environment, scratch);
            if (elements.size() == 1 && is_error(elements[0])) {
                return elements[0];
            }
            return scratch.allocate_array(elements);
        }
        case NodeType::HashLiteral:
            return eval_hash_literal(static_cast<const HashLiteral*>(node), environment, scratch);
        case NodeType::IfExpression:
            return eval_if_expression(static_cast<const IfExpression*>(node), environment, scratch);
        case NodeType::FunctionLiteral: {
            const auto* function_literal = static_cast<const FunctionLiteral*>(node);
            std::vector<std::unique_ptr<Identifier>> parameters;
            for (const auto& parameter : function_literal->parameters) {
                if (parameter != nullptr) {
                    auto parameter_clone = std::make_unique<Identifier>();
                    parameter_clone->token = parameter->token;
                    parameter_clone->value = parameter->value;
                    parameter_clone->symbol = parameter->symbol;
                    parameters.push_back(std::move(parameter_clone));
                }
            }

            auto body = function_literal->body != nullptr
                ? clone_block_statement(*function_literal->body)
                : nullptr;

            return scratch.allocate_function(std::move(parameters), std::move(body), &environment);
        }
        default:
            return null_value();
    }
}

auto eval(const Node* node, Environment& environment) -> Value {
    if (node != nullptr && node->kind() == NodeType::Program) {
        resolve_program(static_cast<const Program*>(node));
    }

    ScratchArena scratch;
    const auto evaluated = eval_node(node, environment, scratch);
    return promote(evaluated, environment.store());
}

auto eval(const Node* node) -> DetachedValue {
    auto store = std::make_unique<PersistentStore>();
    auto* environment = store->create_environment();

    if (node != nullptr && node->kind() == NodeType::Program) {
        resolve_program(static_cast<const Program*>(node));
    }

    ScratchArena scratch;
    const auto evaluated = eval_node(node, *environment, scratch);
    return DetachedValue {std::move(store), promote(evaluated, environment->store())};
}
