#include "zobrist.h"
#include <stdlib.h>
#include <random>


ZobristNums::ZobristNums() {
    // date of the Sinquefield Cup match between Carlsen and Niemann
    std::mt19937 gen(20220904);
    
    std::uniform_int_distribution<unsigned long long> dist(std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());

    // iterate over every nested square, give each their own random Bitboard
    for(int h = 0; h < 2; ++h) {
        for (int i = 0; i < 6; ++i)  {
            for (int j = 0; j < 64; ++j) {
                zobristNums[h][i][j] = dist(gen);    
            }
        }
    }
    

    // First index is for color of piece
    // Second index is for each type of piece (ordered as in board)
    // second nested array is for each square (0 ... 63)
    // i.e. zorbistNums[0][1][9] is a white knight on a2 
    // we have leftover sqares in the randomNums array, as pawns are unable to go on the first or final rank.
    // white to move at index [0][0][0]
    // castling rights at index [0][0][1-4]
    // En Passent rights at index [1][0][0-7]
}


uint64_t ZobristNums::getSquare(Color pieceColor, Piece pieceType, Square pieceLocation) {
    return zobristNums[pieceColor][pieceType][pieceLocation];
}


// create singleton that our zobrist methods requires access to
ZobristNums& zn = ZobristNums::getZobrist();


uint64_t zobristNewPosition() {
    return 0;
}


void zobristChangePiece(uint64_t& hash, Color pieceColor, Piece pieceType, Square pieceLocation) {
   hash ^= zn.getSquare(pieceColor, pieceType, pieceLocation);
}


void zobristFlipColor(uint64_t& hash) {
    hash ^= zn.getSquare(White, Pawn, a1);
}


void zobristChangeCastleRights(uint64_t& hash, Color side, bool isKingside) {
    if (side == White && isKingside) {
        hash ^= zn.getSquare(White, Pawn, b1);
    }
    if (side == White && !isKingside) {
        hash ^= zn.getSquare(White, Pawn, c1);
    }
    if (side != White && isKingside) {
        hash ^= zn.getSquare(White, Pawn, d1);
    }
    hash ^= zn.getSquare(White, Pawn, e1);
}


void zobristChangeEnPassant(uint64_t& hash, Index file) {
    hash ^= zn.getSquare(Black, Pawn, a1);
}
