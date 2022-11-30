#ifndef _CONSTANTS_H
#define _CONSTANTS_H
#include <cassert>
#include <array>

//For some things, we need to consider some fixed size multidimensional arrays.
//C-style versions of those suck, so let's do something better:
template <class T, size_t I, size_t J> using MultiArray = std::array<std::array<T, J>, I>;
template <class T, size_t I, size_t J, size_t K> using TripleArray = std::array<MultiArray<T, J, K>, I>;

/**
 * Some helpful constants used throughout various board logic
 * This serves to avoid magic numbers and also make things a little more modular
 * if we want to adapt code to work with other chess variants
 */ 
enum Constants {
    MaxDepth = 256,
    MaxNumMoves = 236, //A (not too precise) upper bound on the number of pseudo-legal moves in a chess position. This allows us to allocate our vectors of a certain size right away, without having to do any reallocs.
    NumSquares = 64, NumColors = 2,
    NumRanks = 8, NumFiles = 8,
    NumPieces = 6 //in the future, probably add NumPhases (for middle/endgame) and NumContinuations (for search)?
};
/**
  * The two colors of pieces
  */
enum Color {
   White = 0, Black
};
inline Color flipColor(Color color) {
   return (color == White) ? Black : White;
}
/** 
 * The types of pieces
 */
enum Piece : uint8_t {
    Pawn = 0, Knight, Bishop, Rook, Queen, King
};
/**
 * The types of pieces with colour included (and "Empty", useful for things like empty squares).
 * The integers they correspond to are aligned to work well modulo 4 to get the color/piece.
 */ 
 enum ColorPiece : uint8_t {
    WhitePawn = 0, BlackPawn = 1,
    WhiteKnight = 4, BlackKnight = 5,
    WhiteBishop = 8, BlackBishop = 9,
    WhiteRook = 12, BlackRook = 13,
    WhiteQueen = 16, BlackQueen = 17,
    WhiteKing = 20, BlackKing = 21,
    Empty = 26
 };
//Some useful helper methods to convert between our internal representations using integers quickly
/**
 * Gets the overall piece type from the ColorPiece
 */ 
inline Piece getPieceType(ColorPiece piece) {
    assert(piece != Empty);
    //take advantage of alignment to just truncate and get correct result    
    return static_cast<Piece>(piece / 4);
}
/**
 * Gets the color of the piece from the ColorPiece
 */
inline Color getColorOfPiece(ColorPiece piece) {
    assert(piece != Empty);
    return static_cast<Color>(piece % 4);
}
inline ColorPiece makePiece(Piece type, Color color) {
    return static_cast<ColorPiece>(type * 4 + color);
}
/**
 * To be compatible with internal board representation (LERF as discussed in Board's documentation),
 * this is the enumeration of all squares in a nice alignment
 */ 
enum Square : int32_t {
    None = -1,
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2 = 8, b2, c2, d2, e2, f2, g2, h2,
    a3 = 16, b3, c3, d3, e3, f3, g3, h3,
    a4 = 24, b4, c4, d4, e4, f4, g4, h4,
    a5 = 32, b5, c5, d5, e5, f5, g5, h5,
    a6 = 40, b6, c6, d6, e6, f6, g6, h6,
    a7 = 48, b7, c7, d7, e7, f7, g7, h7,
    a8 = 56, b8, c8, d8, e8, f8, g8, h8
};
inline Square getSquareFromIndex(int square) {
   assert(-1 <= square && square <= 63);
   return static_cast<Square>(square);
}

/**
 * In our board class, we generally distinguish between rank/files indices (which are 0-7 ints, this enum just exists for type checking)
 * and the internal LERF representations (which we can't do quick integer math with to convert between).
 * This is the one drawback involved in our close coupling between bitboard representation and board things
 * but well worth it! 
 */
enum Index {
	Zero = 0, One, Two, Three, Four, Five, Six, Seven
};

/**
 * Stores the game state, to be understood by our display system.
 */
enum GameState {
    Neutral = 0, WhiteResigned, BlackResigned, WhiteGotMated, BlackGotMated, Stalemate, FiftyMove, Threefold, InsufficientMaterial
};

#endif