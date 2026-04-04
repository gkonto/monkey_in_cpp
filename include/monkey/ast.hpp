#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "monkey/token.hpp"

struct Node {
    virtual ~Node() = default;

    [[nodiscard]] virtual auto token_literal() const -> std::string = 0;
    [[nodiscard]] virtual auto as_string() const -> std::string = 0;
};

struct Statement : Node {
    ~Statement() override = default;
};

struct Expression : Node {
    ~Expression() override = default;
};

struct Identifier : Expression {
    Token token;
    std::string value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        return value;
    }
};

struct IntegerLiteral : Expression {
    Token token;
    std::int64_t value {0};

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        return token.literal;
    }
};

struct Boolean : Expression {
    Token token;
    bool value {false};

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        return token.literal;
    }
};

struct StringLiteral : Expression {
    Token token;
    std::string value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        return token.literal;
    }
};

struct ArrayLiteral : Expression {
    Token token;
    std::vector<std::unique_ptr<Expression>> elements;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "[";

        for (std::size_t index = 0; index < elements.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (elements[index] != nullptr) {
                out += elements[index]->as_string();
            }
        }

        out += "]";
        return out;
    }
};

struct HashLiteral : Expression {
    Token token;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "{";

        for (std::size_t index = 0; index < pairs.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (pairs[index].first != nullptr) {
                out += pairs[index].first->as_string();
            }

            out += ": ";

            if (pairs[index].second != nullptr) {
                out += pairs[index].second->as_string();
            }
        }

        out += "}";
        return out;
    }
};

struct PrefixExpression : Expression {
    Token token;
    std::string op;
    std::unique_ptr<Expression> right;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "(" + op;

        if (right != nullptr) {
            out += right->as_string();
        }

        out += ")";
        return out;
    }
};

struct InfixExpression : Expression {
    Token token;
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "(";

        if (left != nullptr) {
            out += left->as_string();
        }

        out += " " + op + " ";

        if (right != nullptr) {
            out += right->as_string();
        }

        out += ")";
        return out;
    }
};

struct BlockStatement : Statement {
    Token token;
    std::vector<std::unique_ptr<Statement>> statements;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out {};

        for (const auto& statement : statements) {
            if (statement != nullptr) {
                out += statement->as_string();
            }
        }

        return out;
    }
};

struct IfExpression : Expression {
    Token token;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> consequence;
    std::unique_ptr<BlockStatement> alternative;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "if";

        if (condition != nullptr) {
            out += condition->as_string();
        }

        out += " ";

        if (consequence != nullptr) {
            out += consequence->as_string();
        }

        if (alternative != nullptr) {
            out += "else ";
            out += alternative->as_string();
        }

        return out;
    }
};

struct FunctionLiteral : Expression {
    Token token;
    std::vector<std::unique_ptr<Identifier>> parameters;
    std::unique_ptr<BlockStatement> body;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = token_literal();
        out += "(";

        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (parameters[index] != nullptr) {
                out += parameters[index]->as_string();
            }
        }

        out += ")";

        if (body != nullptr) {
            out += body->as_string();
        }

        return out;
    }
};

struct CallExpression : Expression {
    Token token;
    std::unique_ptr<Expression> function;
    std::vector<std::unique_ptr<Expression>> arguments;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out {};

        if (function != nullptr) {
            out += function->as_string();
        }

        out += "(";

        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index > 0) {
                out += ", ";
            }

            if (arguments[index] != nullptr) {
                out += arguments[index]->as_string();
            }
        }

        out += ")";
        return out;
    }
};

struct IndexExpression : Expression {
    Token token;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> index;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = "(";

        if (left != nullptr) {
            out += left->as_string();
        }

        out += "[";

        if (index != nullptr) {
            out += index->as_string();
        }

        out += "])";
        return out;
    }
};

struct ExpressionStatement : Statement {
    Token token;
    std::unique_ptr<Expression> expression;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        if (expression == nullptr) {
            return "";
        }

        return expression->as_string();
    }
};

struct Program : Node {
    std::vector<std::unique_ptr<Statement>> statements {};

    [[nodiscard]] auto token_literal() const -> std::string override {
        if (statements.empty() || statements.front() == nullptr) {
            return "";
        }

        return statements.front()->token_literal();
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out {};

        for (const auto& statement : statements) {
            if (statement != nullptr) {
                out += statement->as_string();
            }
        }

        return out;
    }
};

struct LetStatement : Statement {
    Token token;
    Token name;
    std::unique_ptr<Expression> value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = token_literal() + " " + name.literal + " = ";

        if (value != nullptr) {
            out += value->as_string();
        }

        out += ";";
        return out;
    }
};

struct ReturnStatement : Statement {
    Token token;
    std::unique_ptr<Expression> return_value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }

    [[nodiscard]] auto as_string() const -> std::string override {
        std::string out = token_literal() + " ";

        if (return_value != nullptr) {
            out += return_value->as_string();
        }

        out += ";";
        return out;
    }
};

[[nodiscard]] inline auto clone_expression(const Expression& expression) -> std::unique_ptr<Expression>;
[[nodiscard]] inline auto clone_statement(const Statement& statement) -> std::unique_ptr<Statement>;

[[nodiscard]] inline auto clone_block_statement(const BlockStatement& block) -> std::unique_ptr<BlockStatement> {
    auto clone = std::make_unique<BlockStatement>();
    clone->token = block.token;

    for (const auto& statement : block.statements) {
        if (statement != nullptr) {
            clone->statements.push_back(clone_statement(*statement));
        }
    }

    return clone;
}

[[nodiscard]] inline auto clone_expression(const Expression& expression) -> std::unique_ptr<Expression> {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expression); identifier != nullptr) {
        auto clone = std::make_unique<Identifier>();
        clone->token = identifier->token;
        clone->value = identifier->value;
        return clone;
    }

    if (const auto* literal = dynamic_cast<const IntegerLiteral*>(&expression); literal != nullptr) {
        auto clone = std::make_unique<IntegerLiteral>();
        clone->token = literal->token;
        clone->value = literal->value;
        return clone;
    }

    if (const auto* boolean = dynamic_cast<const Boolean*>(&expression); boolean != nullptr) {
        auto clone = std::make_unique<Boolean>();
        clone->token = boolean->token;
        clone->value = boolean->value;
        return clone;
    }

    if (const auto* literal = dynamic_cast<const StringLiteral*>(&expression); literal != nullptr) {
        auto clone = std::make_unique<StringLiteral>();
        clone->token = literal->token;
        clone->value = literal->value;
        return clone;
    }

    if (const auto* array = dynamic_cast<const ArrayLiteral*>(&expression); array != nullptr) {
        auto clone = std::make_unique<ArrayLiteral>();
        clone->token = array->token;
        for (const auto& element : array->elements) {
            if (element != nullptr) {
                clone->elements.push_back(clone_expression(*element));
            }
        }
        return clone;
    }

    if (const auto* hash = dynamic_cast<const HashLiteral*>(&expression); hash != nullptr) {
        auto clone = std::make_unique<HashLiteral>();
        clone->token = hash->token;
        for (const auto& [key, value] : hash->pairs) {
            std::unique_ptr<Expression> key_clone;
            std::unique_ptr<Expression> value_clone;

            if (key != nullptr) {
                key_clone = clone_expression(*key);
            }

            if (value != nullptr) {
                value_clone = clone_expression(*value);
            }

            clone->pairs.push_back({std::move(key_clone), std::move(value_clone)});
        }
        return clone;
    }

    if (const auto* prefix = dynamic_cast<const PrefixExpression*>(&expression); prefix != nullptr) {
        auto clone = std::make_unique<PrefixExpression>();
        clone->token = prefix->token;
        clone->op = prefix->op;
        if (prefix->right != nullptr) {
            clone->right = clone_expression(*prefix->right);
        }
        return clone;
    }

    if (const auto* infix = dynamic_cast<const InfixExpression*>(&expression); infix != nullptr) {
        auto clone = std::make_unique<InfixExpression>();
        clone->token = infix->token;
        clone->op = infix->op;
        if (infix->left != nullptr) {
            clone->left = clone_expression(*infix->left);
        }
        if (infix->right != nullptr) {
            clone->right = clone_expression(*infix->right);
        }
        return clone;
    }

    if (const auto* if_expression = dynamic_cast<const IfExpression*>(&expression); if_expression != nullptr) {
        auto clone = std::make_unique<IfExpression>();
        clone->token = if_expression->token;
        if (if_expression->condition != nullptr) {
            clone->condition = clone_expression(*if_expression->condition);
        }
        if (if_expression->consequence != nullptr) {
            clone->consequence = clone_block_statement(*if_expression->consequence);
        }
        if (if_expression->alternative != nullptr) {
            clone->alternative = clone_block_statement(*if_expression->alternative);
        }
        return clone;
    }

    if (const auto* function = dynamic_cast<const FunctionLiteral*>(&expression); function != nullptr) {
        auto clone = std::make_unique<FunctionLiteral>();
        clone->token = function->token;
        for (const auto& parameter : function->parameters) {
            if (parameter != nullptr) {
                auto parameter_clone = std::make_unique<Identifier>();
                parameter_clone->token = parameter->token;
                parameter_clone->value = parameter->value;
                clone->parameters.push_back(std::move(parameter_clone));
            }
        }
        if (function->body != nullptr) {
            clone->body = clone_block_statement(*function->body);
        }
        return clone;
    }

    if (const auto* call = dynamic_cast<const CallExpression*>(&expression); call != nullptr) {
        auto clone = std::make_unique<CallExpression>();
        clone->token = call->token;
        if (call->function != nullptr) {
            clone->function = clone_expression(*call->function);
        }
        for (const auto& argument : call->arguments) {
            if (argument != nullptr) {
                clone->arguments.push_back(clone_expression(*argument));
            }
        }
        return clone;
    }

    if (const auto* index = dynamic_cast<const IndexExpression*>(&expression); index != nullptr) {
        auto clone = std::make_unique<IndexExpression>();
        clone->token = index->token;
        if (index->left != nullptr) {
            clone->left = clone_expression(*index->left);
        }
        if (index->index != nullptr) {
            clone->index = clone_expression(*index->index);
        }
        return clone;
    }

    return nullptr;
}

[[nodiscard]] inline auto clone_statement(const Statement& statement) -> std::unique_ptr<Statement> {
    if (const auto* expression_statement = dynamic_cast<const ExpressionStatement*>(&statement); expression_statement != nullptr) {
        auto clone = std::make_unique<ExpressionStatement>();
        clone->token = expression_statement->token;
        if (expression_statement->expression != nullptr) {
            clone->expression = clone_expression(*expression_statement->expression);
        }
        return clone;
    }

    if (const auto* let_statement = dynamic_cast<const LetStatement*>(&statement); let_statement != nullptr) {
        auto clone = std::make_unique<LetStatement>();
        clone->token = let_statement->token;
        clone->name = let_statement->name;
        if (let_statement->value != nullptr) {
            clone->value = clone_expression(*let_statement->value);
        }
        return clone;
    }

    if (const auto* return_statement = dynamic_cast<const ReturnStatement*>(&statement); return_statement != nullptr) {
        auto clone = std::make_unique<ReturnStatement>();
        clone->token = return_statement->token;
        if (return_statement->return_value != nullptr) {
            clone->return_value = clone_expression(*return_statement->return_value);
        }
        return clone;
    }

    if (const auto* block_statement = dynamic_cast<const BlockStatement*>(&statement); block_statement != nullptr) {
        return clone_block_statement(*block_statement);
    }

    return nullptr;
}
