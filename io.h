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
    bool basicPieces = true;
    // Whether to differentiate the dark and light squares
    bool showCheckers = true;
    // Whether to print the board from black's perspective when appropriate
    bool boardPerspective = true;

    /**
     * Translate our piece integer into a display character
     */
    const std::array<wchar_t, 12> PieceChar{'p', 'P', 'n', 'N', 'b', 'B', 'r', 'R', 'q', 'Q', 'k', 'K'};
    const std::array<wchar_t, 12> PieceImage{L'♟', L'♙', L'♞', L'♘', L'♝', L'♗', L'♜', L'♖', L'♛', L'♕', L'♚', L'♔'};

public:
    TextDisplay(std::ostream& out): IO{out} {};
    void printBoard(Board& board);
};

#endif
