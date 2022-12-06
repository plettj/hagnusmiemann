#include "io.h"
#include <iostream>

int main() {
    /**
     * Create an Input-Output object,
     * to hold our entire program.
     * 
     * See in io.cc, TextInput::runProgram() for the full input system.
     */
    IO io{std::cin, std::cout};
    
    io.runProgram();

    // No need for special exit codes.
}
