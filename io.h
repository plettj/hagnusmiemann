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
    // [Observer Pattern] the subject:
    std::unique_ptr<Input> input;
    // [Observer Pattern] the observers:
    std::vector<std::unique_ptr<Output>> outputs;

    std::ostream& out;

    bool basicPieces = true; // Whether pieces are drawn with text [eg. K] or ascii pieces [eg. ♔]
    bool showCheckers = false; // Whether to differentiate the dark and light squares
    bool boardPerspective = false; // Whether to print the board from black's perspective when appropriate
    bool wideBoard = false; // Whether to print the board with wide style

    // Helper functions for the main `fullDisplay()`:
    void initialize(bool setup, std::string white, std::string black, int game);
    void display(Board& board, GameState state, bool setup = false, bool firstSetup = false);
public:
    IO(std::istream& in, std::ostream& out);
    void makeTextOutput(std::ostream& out);
    void makeGraphicOutput(int size);

    /**
     * Closes the most recently opened graphical output.
     * This is fully functional; however, it is impractical to allow our user to create
     * multiple graphical displays at once, so the input handler implements a cap-at-one system.
     */
    void closeGraphicOutput();
    /** For the input handler's cap-at-one system. */
    bool hasGraphicsOpen();
    
    // [Observer Pattern] notifyObservers():
    void fullDisplay(Board& board, GameState state, int game, std::pair<int, int> players, bool setup = false, bool firstSetup = false);
    
    void toggleSetting(int setting);
    bool getSetting(int setting);

    /**
     * Get this whole big baby ROLLIN'!!!
     */
    void runProgram();
};

// [Observer Pattern] Observer
class Output {
protected:
    Input* toFollow;
public:
    Output(Input* toFollow): toFollow{toFollow} {};
    virtual void initialize(bool setup, std::string white, std::string black, int game) = 0;
    // [Observer Pattern] notify():
    virtual void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) = 0;
    virtual ~Output() = default;
};

// [Observer Pattern] Concrete Observer #1
class TextOutput: public Output {
    std::ostream& out;
public:
    TextOutput(Input* toFollow, std::ostream& out);
    void initialize(bool setup, std::string white, std::string black, int game) override {};
    // [Observer Pattern] notify();
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;
};

// [Observer Pattern] Concrete Observer #2
class GraphicalOutput: public Output {
    std::unique_ptr<Xwindow> window;
    int size;
public:
    GraphicalOutput(Input* toFollow, int size);
    void initialize(bool setup, std::string white, std::string black, int game) override;
    void display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;
};

// [Observer Pattern] Subject
class Input {
protected:
    std::vector<Output*> outputs;
public:
    // [Observer Pattern] attach(), detach(), and notifyObservers():
    virtual void attach(Output* output) = 0;
    virtual void detach(Output* output) = 0;
    virtual void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) = 0;

    /**
     * Get this whole big baby ROLLIN'!!!
     */
    virtual void runProgram(IO& io, std::ostream& out) = 0;
};

// [Observer Pattern] Concrete Subject #1
class TextInput: public Input {
    std::istream& in;
public:
    TextInput(std::istream& in): in{in} {};
    // [Observer Pattern] attach(), detach(), and notifyObservers():
    void attach(Output* output) override;
    void detach(Output* output) override;
    void notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) override;

    /**
     * Get this whole big baby ROLLIN'!!!
     */
    void runProgram(IO& io, std::ostream& out) override;
};

#endif
