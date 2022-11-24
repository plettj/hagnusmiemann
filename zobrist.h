#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <array>
#include <stdlib.h>
#include "board.h"




#include <iostream>  // REMOVE


class ZobristNums {
    uint64_t zobristNums[2][6][64];
    friend class ZobristHash;
public:
    ZobristNums();
    void p() {
        for (int h = 0; h < 2; h++) {
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 64; j++) {
                    std::cout << zobristNums[h][i][j] << " ";
                }
                std::cout << std::endl;
            }
        }
    }
};


class ZobristHash {
private:
    ZobristNums* zn;
public:
    ZobristHash(ZobristNums* zn);
    uint64_t newPosition();
    uint64_t changePiece(uint64_t hash, int pieceType, int pieceLocation, bool pieceIsWhite);
    uint64_t flipPlayerToMove(uint64_t hash);
    uint64_t changeCastleRights(uint64_t hash, bool isWhite, bool isKingside);
    uint64_t changeEnPassent(uint64_t hash, int file);    
};

#endif
