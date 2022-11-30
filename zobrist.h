#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <array>
#include <stdlib.h>
#include "board.h"
#include "constants.h"


class ZobristNums {
    uint64_t zobristNums[NumColors][NumPieces][NumSquares];
    ZobristNums();

public:
    // singleton pattern (exciting!!), where we enforce only one object created, and destroy copy ctors
    static ZobristNums& getZobrist() {
        static ZobristNums instance; 
        return instance;
    }
    ZobristNums(const ZobristNums& other) = delete;
    void operator=(const ZobristNums& other) = delete;

    static uint64_t newPosition();
    static void changePiece(uint64_t& hash, Color pieceColor, Piece pieceType, Square pieceLocation);
    static void flipColor(uint64_t& hash);
    static void changeCastleRights(uint64_t& hash, Color side, bool isKingside);
    static void changeEnPassant(uint64_t& hash, Index file);
};


#endif
