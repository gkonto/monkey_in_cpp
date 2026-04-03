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
