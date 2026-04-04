#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string_view>

#include "monkey/evaluator.hpp"
#include "monkey/lexer.hpp"
#include "monkey/parser.hpp"

namespace {

auto test_eval(std::string_view input) -> std::shared_ptr<Object> {
    auto lexer = Lexer {input};
    auto parser = Parser {lexer};
    const auto program = parser.parse_program();

    REQUIRE(parser.errors().empty());
    return eval(&program);
}

auto format_elapsed(std::chrono::milliseconds elapsed) -> std::string {
    auto remaining = elapsed;

    const auto hours = std::chrono::duration_cast<std::chrono::hours>(remaining);
    remaining -= std::chrono::duration_cast<std::chrono::milliseconds>(hours);

    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(remaining);
    remaining -= std::chrono::duration_cast<std::chrono::milliseconds>(minutes);

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(remaining);
    remaining -= std::chrono::duration_cast<std::chrono::milliseconds>(seconds);

    std::ostringstream output;
    bool wrote = false;

    if (hours.count() > 0) {
        output << hours.count() << "h";
        wrote = true;
    }

    if (minutes.count() > 0 || wrote) {
        if (wrote) {
            output << " ";
        }
        output << minutes.count() << "m";
        wrote = true;
    }

    if (seconds.count() > 0 || wrote) {
        if (wrote) {
            output << " ";
        }
        output << seconds.count() << "s";
        wrote = true;
    }

    if (wrote) {
        output << " ";
    }
    output << remaining.count() << "ms";

    return output.str();
}

}  // namespace

TEST_CASE("Performance fibonacci(33)", "[performance]") {
    constexpr auto input =
        "let fibonacci = fn(x) {\n"
        "  if (x == 0) {\n"
        "    0\n"
        "  } else {\n"
        "    if (x == 1) {\n"
        "      1\n"
        "    } else {\n"
        "      fibonacci(x - 1) + fibonacci(x - 2);\n"
        "    }\n"
        "  }\n"
        "};\n"
        "fibonacci(33);";

    const auto start = std::chrono::steady_clock::now();
    const auto evaluated = test_eval(input);
    const auto end = std::chrono::steady_clock::now();

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "fibonacci(33) took " << format_elapsed(elapsed) << "\n";

    REQUIRE(evaluated != nullptr);

    const auto* integer = dynamic_cast<const IntegerObject*>(evaluated.get());
    REQUIRE(integer != nullptr);
    REQUIRE(integer->value == 3524578);
}
