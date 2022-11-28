#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <array>
#include <stdlib.h>
#include "board.h"


class ZobristNums {
    uint64_t zobristNums[Board::NumColors][Board::NumPieces][Board::NumSquares];
    ZobristNums();

public:
    // singleton pattern (exciting!!), where we enforce only one object created, and destroy copy ctors
    static ZobristNums& getZobrist() {
        static ZobristNums instance; 
        return instance;
    }
    ZobristNums(const ZobristNums& other) = delete;
    void operator=(const ZobristNums& other) = delete;

    uint64_t getSquare(Board::Color pieceColor, Board::Piece pieceType, Board::Square pieceLocation);
};

#endif
