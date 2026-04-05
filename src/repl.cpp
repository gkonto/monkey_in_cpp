#include "monkey/repl.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "monkey/environment.hpp"
#include "monkey/evaluator.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

void print_parser_errors(std::ostream& output, const std::vector<std::string>& errors) {
    for (const auto& error : errors) {
        output << error << '\n';
    }
}

}  // namespace

void start_repl(std::istream& input, std::ostream& output) {
    PersistentStore store;
    auto* environment = store.create_environment();
    std::string line;

    while (true) {
        output << ">> ";
        output.flush();

        if (!std::getline(input, line)) {
            break;
        }

        Lexer lexer {line};
        Parser parser {lexer};
        const auto program = parser.parse_program();

        if (!parser.errors().empty()) {
            print_parser_errors(output, parser.errors());
            continue;
        }

        const auto evaluated = eval(&program, *environment);
        output << evaluated.inspect() << '\n';
    }
}
