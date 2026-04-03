#pragma once

#include <memory>
#include <string>
#include <vector>

#include "monkey/token.hpp"

struct Node {
    virtual ~Node() = default;

    [[nodiscard]] virtual auto token_literal() const -> std::string = 0;
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
};

struct Program {
    std::vector<std::unique_ptr<Statement>> statements {};

    [[nodiscard]] auto token_literal() const -> std::string {
        if (statements.empty() || statements.front() == nullptr) {
            return "";
        }

        return statements.front()->token_literal();
    }
};

struct LetStatement : Statement {
    Token token;
    Token name;
    std::unique_ptr<Expression> value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }
};

struct ReturnStatement : Statement {
    Token token;
    std::unique_ptr<Expression> return_value;

    [[nodiscard]] auto token_literal() const -> std::string override {
        return token.literal;
    }
};
