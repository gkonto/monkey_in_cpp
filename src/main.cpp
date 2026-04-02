#include <iostream>

#include "monkey/repl.hpp"

auto main() -> int {
    std::cout << "Monkey REPL" << '\n';
    start_repl(std::cin, std::cout);
    return 0;
}
