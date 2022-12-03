#ifndef _IO_H
#define _IO_H
#include "board.h"
#include "move.h"
#include <iostream>
#include <memory>

class Input;
class Output;

/**
 * IO is a meta-object, to hold the Displays [observers] and
 * the singular Input [subject], that follow the Observer pattern
 */
class IO {
    Input* input;
    std::vector<Output*> outputs;

    bool basicPieces = true; // Whether pieces are drawn with text [eg. K] or ascii pieces [eg. ♔]
    bool showCheckers = false; // Whether to differentiate the dark and light squares
    bool boardPerspective = false; // Whether to print the board from black's perspective when appropriate
    bool wideBoard = false; // Whether to print the board with wide style
    bool autoMove = false; // Whether to 
public:
    IO(std::istream& in);
    void makeTextOutput(std::ostream& out);
    void makeGraphicOutput();
    void display(Board& board, GameState state, bool setup = false);
    void toggleSetting(int setting);
    bool getSetting(int setting);
};

// Observer
class Output {
protected:
    Input* toFollow;
public:
    Output(Input* toFollow): toFollow{toFollow} {};
    virtual void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup) = 0; // notify()
};

// Concrete Observer #1
class TextOutput: public Output {
    std::ostream& out;
    // Translate our piece integers into display characters:
    const std::array<char, 12> PieceChar{'p', 'P', 'n', 'N', 'b', 'B', 'r', 'R', 'q', 'Q', 'k', 'K'};
    const std::array<std::string, 12> PieceImage{"♟", "♙", "♞", "♘", "♝", "♗", "♜", "♖", "♛", "♕", "♚", "♔"};
public:
    TextOutput(Input* toFollow, std::ostream& out);
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup) override;
};

// Concrete Observer #2
class GraphicalOutput: public Output {

public:
    GraphicalOutput(Input* toFollow);
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup) override;
};

// Subject
class Input {
protected:
    std::vector<Output*> outputs;
public:
    virtual void attach(Output* output) = 0;
    virtual void detach(Output* output) = 0;
    virtual void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup) = 0;
};

// Concrete Subject #1
class TextInput: public Input {
    std::istream& in;
public:
    TextInput(std::istream& in): in{in} {};
    void attach(Output* output) override;
    void detach(Output* output) override;
    void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup) override;
};

#endif
