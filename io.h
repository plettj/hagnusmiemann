#ifndef _IO_H
#define _IO_H
#include "board.h"
#include "move.h"
#include "window.h"
#include "difficultylevel.h"
#include <iostream>
#include <memory>

class Input;
class Output;
class TextInput;

/**
 * IO is a meta-object, to hold the Outputs [observers] and
 * the singular Input [subject], that follow the Observer pattern
 */
class IO {
    std::unique_ptr<Input> input;
    std::vector<std::unique_ptr<Output>> outputs;
    std::ostream& out; // for the Input object to use.

    bool basicPieces = true; // Whether pieces are drawn with text [eg. K] or ascii pieces [eg. ♔]
    bool showCheckers = false; // Whether to differentiate the dark and light squares
    bool boardPerspective = false; // Whether to print the board from black's perspective when appropriate
    bool wideBoard = false; // Whether to print the board with wide style

    void initialize(bool setup, std::string white, std::string black, int game);
    void display(Board& board, GameState state, bool setup = false, bool firstSetup = false);
public:
    IO(std::istream& in, std::ostream& out);
    void makeTextOutput(std::ostream& out);
    void makeGraphicOutput(int size = 600);
    void closeGraphicOutput(); // Closes the most recently opened output.
    bool hasGraphicsOpen();
    void fullDisplay(Board& board, GameState state, int game, std::pair<int, int> players, bool setup = false, bool firstSetup = false);
    void toggleSetting(int setting);
    bool getSetting(int setting);
    void runProgram();
};

// Observer
class Output {
protected:
    Input* toFollow;
public:
    Output(Input* toFollow): toFollow{toFollow} {};
    virtual void initialize(bool setup, std::string white, std::string black, int game) = 0;
    virtual void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) = 0; // notify()
    virtual ~Output() = default;
};

// Concrete Observer #1
class TextOutput: public Output {
    std::ostream& out;
public:
    TextOutput(Input* toFollow, std::ostream& out);
    void initialize(bool setup, std::string white, std::string black, int game) override {};
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;
};

// Concrete Observer #2
class GraphicalOutput: public Output {
    std::unique_ptr<Xwindow> window;
    int size;
public:
    GraphicalOutput(Input* toFollow, int size);
    void initialize(bool setup, std::string white, std::string black, int game) override;
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;
};

// Subject
class Input {
protected:
    std::vector<Output*> outputs;
public:
    virtual void attach(Output* output) = 0;
    virtual void detach(Output* output) = 0;
    virtual void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) = 0;
    virtual void runProgram(IO& io, std::ostream& out) = 0;
};

// Concrete Subject #1
class TextInput: public Input {
    std::istream& in;
public:
    TextInput(std::istream& in): in{in} {};
    void attach(Output* output) override;
    void detach(Output* output) override;
    void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;
    void runProgram(IO& io, std::ostream& out) override;
};

#endif
