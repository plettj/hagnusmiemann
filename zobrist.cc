#include "zobrist.h"
#include <stdlib.h>
#include <random>


ZobristNums::ZobristNums() {
    // date of the Sinquefield Cup match between Carlsen and Niemann
    std::mt19937 gen(20220904);
    
    std::uniform_int_distribution<unsigned long long> dist(std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());

    // iterate over every nested square, give each their own random Bitboard
    for (int i = 0; i < 12; ++i)  {
        for (int j = 0; j < 64; ++j) {
            zobristNums[i][j] = dist(gen);    
        }
    }

    // first index is for each type of piece (12 indicies = 6 pieces * 2 colours)
    // second nested array is for each square (0 ... 63)
    // i.e. zorbistNums[2][9] is a white knight on a2 
    // we have leftover sqares in the randomNums array, as pawns are unable to go on the first or final rank.
    // white to move at index [0][0]
    // castling rights at index [0][1-4]
    // En Passent rights at index [1][0-7]
}


ZobristHash::ZobristHash(ZobristNums *zn) : zn{zn} {}


uint64_t ZobristHash::newPosition() {
    return 0;
}


uint64_t ZobristHash::changePiece(uint64_t hash, int pieceType, int pieceLocation, bool pieceIsWhite) {
    if (pieceIsWhite) return hash xor zn->zobristNums[pieceType][pieceLocation];
    return hash xor zn->zobristNums[6 + pieceType][pieceLocation];
}


uint64_t ZobristHash::flipPlayerToMove(uint64_t hash) {
    return hash xor zn->zobristNums[0][0];
}


uint64_t ZobristHash::changeCastleRights(uint64_t hash, bool isWhite, bool isKingside) {
    if (isWhite && isKingside) return hash xor zn->zobristNums[0][1];
    if (isWhite && !isKingside) return hash xor zn->zobristNums[0][2];
    if (!isWhite && isKingside) return hash xor zn->zobristNums[0][3];
    return hash xor zn->zobristNums[0][4];
}


uint64_t ZobristHash::changeEnPassent(uint64_t hash, int file) {
    return hash xor zn->zobristNums[1][file];
}
