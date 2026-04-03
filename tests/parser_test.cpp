#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "monkey/ast.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

void test_let_statement(const Statement* statement, std::string_view expected_name) {
    REQUIRE(statement != nullptr);

    const auto* let_statement = dynamic_cast<const LetStatement*>(statement);
    REQUIRE(let_statement != nullptr);
    REQUIRE(let_statement->token_literal() == "let");
    REQUIRE(let_statement->name.literal == expected_name);
}

}  // namespace

TEST_CASE("TestLetStatement", "[parser]") {
    constexpr auto input =
        "let x = 5;\n"
        "let y = 10;\n"
        "let foobar = 838383;";

    auto lexer = Lexer{input};
    auto parser = Parser{lexer};
    const auto program = parser.parse_program();

    REQUIRE(program.statements.size() == 3);

    constexpr std::string_view expected_identifiers[] = {"x", "y", "foobar"};

    for (std::size_t index = 0; index < std::size(expected_identifiers); ++index) {
        test_let_statement(program.statements[index].get(), expected_identifiers[index]);
    }
}
