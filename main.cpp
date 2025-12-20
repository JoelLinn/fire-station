#include <iostream>

#include "Format.hpp"

int main() {
    std::cout << fmt::format("Hello {}", "World") << std::endl;
    return 0;
}
