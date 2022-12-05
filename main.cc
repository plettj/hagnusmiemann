#include "io.h"
#include <iostream>

int main() {
    IO io{std::cin, std::cout};
    io.runProgram();
}
