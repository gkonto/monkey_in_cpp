#include "monkey/repl.hpp"

#include <iostream>
#include <string>

#include "monkey/lexer.hpp"
#include "monkey/token.hpp"

void start_repl(std::istream& input, std::ostream& output) {
    std::string line {};

    while (true) {
        output << ">> ";
        output.flush();

        if (!std::getline(input, line)) {
            break;
        }

        Lexer lexer {line};

        while (true) {
            const auto token = lexer.next_token();
            output << to_string(token.type) << ' ' << '"' << token.literal << '"' << '\n';

            if (token.type == TokenType::EndOfFile) {
                break;
            }
        }
    }
}
