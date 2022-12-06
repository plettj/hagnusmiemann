#ifndef _CONSTANTS_H
#define _CONSTANTS_H
#include <cassert>
#include <array>
#include <map>

// many modules require CentipawnScore
typedef int CentipawnScore;

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
inline Piece charToPiece(char piece) {
    switch (piece) {
        case 'P': return Piece::Pawn;
        case 'N': return Piece::Knight;
        case 'B': return Piece::Bishop;
        case 'R': return Piece::Rook;
        case 'Q': return Piece::Queen;
        case 'K': return Piece::King;
        default: return Piece::Pawn; // This function assumes `piece` is one of the 6 options, anyway.
    }
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

/**
 * Stores PSQT for static evaluation.
 * Based off Sunfish's PSQT: https://github.com/thomasahle/sunfish/blob/master/sunfish.py
 */
static const std::map<ColorPiece, std::array<CentipawnScore, NumSquares>> psqt = {
{
	WhitePawn,
    {	100,100,100,100,100,100,100,100,
        69,108,93,63,64,86,103,69,
        78,109,105,89,90,98,103,81,
        74,103,110,109,106,101,100,77,
        83,116,98,115,114,100,115,87,
        107,129,121,144,140,131,144,107,
        178,183,186,173,202,182,185,190,
        100,100,100,100,100,100,100,100,
    }
},
{
	BlackPawn,
    {	-100,-100,-100,-100,-100,-100,-100,-100,
        -178,-183,-186,-173,-202,-182,-185,-190,
        -107,-129,-121,-144,-140,-131,-144,-107,
        -83,-116,-98,-115,-114,-100,-115,-87,
        -74,-103,-110,-109,-106,-101,-100,-77,
        -78,-109,-105,-89,-90,-98,-103,-81,
        -69,-108,-93,-63,-64,-86,-103,-69,
        -100,-100,-100,-100,-100,-100,-100,-100,
    }
},
{
	WhiteKnight,
    {	206,257,254,256,261,245,258,211,
        257,265,282,280,282,280,257,260,
        262,290,293,302,298,295,291,266,
        279,285,311,301,302,315,282,280,
        304,304,325,317,313,321,305,297,
        290,347,281,354,353,307,342,278,
        277,274,380,244,284,342,276,266,
        214,227,205,205,270,225,222,210,
    }
},
{
	BlackKnight,
    {	-214,-227,-205,-205,-270,-225,-222,-210,
        -277,-274,-380,-244,-284,-342,-276,-266,
        -290,-347,-281,-354,-353,-307,-342,-278,
        -304,-304,-325,-317,-313,-321,-305,-297,
        -279,-285,-311,-301,-302,-315,-282,-280,
        -262,-290,-293,-302,-298,-295,-291,-266,
        -257,-265,-282,-280,-282,-280,-257,-260,
        -206,-257,-254,-256,-261,-245,-258,-211,
    }
},
{
	WhiteBishop,
    {	313,322,305,308,306,305,310,310,
        339,340,331,326,327,326,340,336,
        334,345,344,335,328,345,340,335,
        333,330,337,343,337,336,320,327,
        345,337,340,354,346,345,335,330,
        311,359,288,361,372,310,348,306,
        309,340,355,278,281,351,322,298,
        261,242,238,244,297,213,283,270,
    }
},
{
	BlackBishop,
    {	-261,-242,-238,-244,-297,-213,-283,-270,
        -309,-340,-355,-278,-281,-351,-322,-298,
        -311,-359,-288,-361,-372,-310,-348,-306,
        -345,-337,-340,-354,-346,-345,-335,-330,
        -333,-330,-337,-343,-337,-336,-320,-327,
        -334,-345,-344,-335,-328,-345,-340,-335,
        -339,-340,-331,-326,-327,-326,-340,-336,
        -313,-322,-305,-308,-306,-305,-310,-310,
    }
},
{
	WhiteRook,
    {	449,455,461,484,477,461,448,447,
        426,441,448,453,450,436,435,426,
        437,451,437,454,454,444,453,433,
        451,444,463,458,466,450,433,449,
        479,484,495,492,497,475,470,473,
        498,514,507,512,524,506,504,494,
        534,508,535,546,534,541,513,539,
        514,508,512,483,516,512,535,529,
    },
},
{
	BlackRook,
    {	-514,-508,-512,-483,-516,-512,-535,-529,
        -534,-508,-535,-546,-534,-541,-513,-539,
        -498,-514,-507,-512,-524,-506,-504,-494,
        -479,-484,-495,-492,-497,-475,-470,-473,
        -451,-444,-463,-458,-466,-450,-433,-449,
        -437,-451,-437,-454,-454,-444,-453,-433,
        -426,-441,-448,-453,-450,-436,-435,-426,
        -449,-455,-461,-484,-477,-461,-448,-447,
    }
},
{
	WhiteQueen,
    {	890,899,898,916,898,893,895,887,
        893,911,929,910,914,914,908,891,
        899,923,916,918,913,918,913,902,
        915,914,927,924,928,919,909,907,
        930,913,951,946,954,949,916,923,
        927,972,961,989,1001,992,972,931,
        943,961,989,919,949,1005,986,953,
        935,930,921,825,998,953,1017,955,
    }
},
{
	BlackQueen,
    {	-935,-930,-921,-825,-998,-953,-1017,-955,
        -943,-961,-989,-919,-949,-1005,-986,-953,
        -927,-972,-961,-989,-1001,-992,-972,-931,
        -930,-913,-951,-946,-954,-949,-916,-923,
        -915,-914,-927,-924,-928,-919,-909,-907,
        -899,-923,-916,-918,-913,-918,-913,-902,
        -893,-911,-929,-910,-914,-914,-908,-891,
        -890,-899,-898,-916,-898,-893,-895,-887,
    }
},
{
	WhiteKing,
    {	6017,6030,5997,5986,6006,5999,6040,6018,
        5996,6003,5986,5950,5943,5982,6013,6004,
        5953,5958,5957,5921,5936,5968,5971,5968,
        5945,5957,5948,5972,5949,5953,5992,5950,
        5945,6050,6011,5996,5981,6013,6000,5951,
        5938,6012,5943,6044,5933,6028,6037,5969,
        5968,6010,6055,6056,6056,6055,6010,6003,
        6004,6054,6047,5901,5901,6060,6083,5938,
    }
},
{
	BlackKing,
    {	-6004,-6054,-6047,-5901,-5901,-6060,-6083,-5938,
        -5968,-6010,-6055,-6056,-6056,-6055,-6010,-6003,
        -5938,-6012,-5943,-6044,-5933,-6028,-6037,-5969,
        -5945,-6050,-6011,-5996,-5981,-6013,-6000,-5951,
        -5945,-5957,-5948,-5972,-5949,-5953,-5992,-5950,
        -5953,-5958,-5957,-5921,-5936,-5968,-5971,-5968,
        -5996,-6003,-5986,-5950,-5943,-5982,-6013,-6004,
        -6017,-6030,-5997,-5986,-6006,-5999,-6040,-6018,
    }
},
{
    Empty,
    {   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    }
}
};



#endif