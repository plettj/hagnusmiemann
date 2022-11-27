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
    
    //First index is for color of piece
    // Second index is for each type of piece (ordered as in board)
    // second nested array is for each square (0 ... 63)
    // i.e. zorbistNums[0][1][9] is a white knight on a2 
    // we have leftover sqares in the randomNums array, as pawns are unable to go on the first or final rank.
    // white to move at index [0][0][0]
    // castling rights at index [0][0][1-4]
    // En Passent rights at index [1][0][0-7]
}


ZobristHash::ZobristHash(ZobristNums *zn) : zn{zn} {}


uint64_t ZobristHash::newPosition() {
    return 0;
}


uint64_t ZobristHash::changePiece(uint64_t hash, Board::Piece pieceType, Board::Square pieceLocation, Board::Color pieceColor) {
   return hash xor zn->zobristNums[pieceColor][pieceType][pieceLocation];
}


uint64_t ZobristHash::flipPlayerToMove(uint64_t hash) {
    return hash xor zn->zobristNums[0][0][0];
}


uint64_t ZobristHash::changeCastleRights(uint64_t hash, Board::Color side, bool isKingside) {
    if (side == Board::White && isKingside) return hash xor zn->zobristNums[0][0][1];
    if (side == Board::White && !isKingside) return hash xor zn->zobristNums[0][0][2];
    if (side != Board::White && isKingside) return hash xor zn->zobristNums[0][0][3];
    return hash xor zn->zobristNums[0][0][4];
}


uint64_t ZobristHash::changeEnPassant(uint64_t hash, Board::Index file) {
    return hash xor zn->zobristNums[1][0][file];
}
