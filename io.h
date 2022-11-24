#ifndef _IO_H
#define _IO_H
#include "board.h"
#include "move.h"
#include <iostream>

class IO {
protected:
    std::ostream& out;
public:
    IO(std::ostream& out): out{out} {};
};

class TextDisplay : public IO {
    // Whether pieces are drawn with text [eg. K] or ascii pieces [eg. ♔]
    bool basicPieces = false;
    // Whether to differentiate the dark and light squares
    bool showCheckers = false;
    // Whether to print the board from black's perspective when appropriate
    bool boardPerspective = true;
    // Whether to print the board with wide style
    bool wideBoard = false;

    /**
     * Translate our piece integer into a display character
     */
    const std::array<char, 12> PieceChar{'p', 'P', 'n', 'N', 'b', 'B', 'r', 'R', 'q', 'Q', 'k', 'K'};
    const std::array<std::string, 12> PieceImage{"♟", "♙", "♞", "♘", "♝", "♗", "♜", "♖", "♛", "♕", "♚", "♔"};

public:
    TextDisplay(std::ostream& out): IO{out} {};
    void printBoard(Board& board);

    void setBasicPieces(bool basic);
    void setShowCheckers(bool checkers);
    void setBoardPerspective(bool perspective);
    void setWideBoard(bool wide);
};

#endif
