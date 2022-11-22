#ifndef _BOARD_H
#define _BOARD_H
#include <cassert>
#include <iostream>
#include <string>
#include <array>
#include <vector>


class Move;

typedef uint64_t Bitboard;

/**
 * The main board class that represents a current board position which is allowed to be ___PSEUDO-LEGAL___
 * (which means that it is either a legal position or a position that arose from a legal position where a side left their king in check, which is illegal)
 * Internally, it represents things using bitboards, which are several fancy unsigned 64 bit integers, where each bit corresponds to a square
 * having a state true or false (like if a knight is on the square or not).
 * As a chessprogramming housekeeping note, we use LERF bitboard convention (little endian rank file).
 */ 
class Board {
public:
   /**
    * Some helpful constants used throughout various board logic
    * This serves to avoid magic numbers and also make things a little more modular
    * if we want to adapt code to work with other chess variants
    */ 
    enum Constants {
        NumSquares = 64, NumColors = 2,
	    NumRanks = 8, NumFiles = 8,
	    NumPieces = 6 //in the future, probably add NumPhases (for middle/endgame) and NumContinuations (for search)?
    };

    /**
     * The two colors of pieces
     */
    enum Color {
	    White = 0, Black
	    White = 0, Black
    };

    /** 
     * The types of pieces
     */
    enum Piece : uint8_t {
        Pawn = 0, Knight, Bishop, Rook, Queen, King
    };
   /**
    * The types of pieces with colour included (and "Empty", useful for things like empty squares).
    * The integers they correspond to are aligned to be compatible with the internal board representation
    * using an unsigned 64 bit integer
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
    static inline Piece getPieceType(ColorPiece piece) {
	assert(piece != Empty);
	//take advantage of alignment to just truncate and get correct result    
	return static_cast<Piece>(piece / 4);
    }

    /**
     * Gets the color of the piece from the ColorPiece
     */
    static inline Color getColorOfPiece(ColorPiece piece) {
	    assert(piece != Empty);
	    return static_cast<Color>(piece % 4);
    }

    static inline ColorPiece makePiece(Piece type, Color color) {
	    return static_cast<ColorPiece>(type * 4 + color);
    }

   /**
    * To be compatible with internal board representation (LERF as discussed below),
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

    static inline Square getSquare(int square) {
	    assert(-1 >= square && square <= 63);
	    return static_cast<Square>(square);
    }
    /**
    * For the purposes of the bitboard hash functions for Rooks/
    */
    struct HashEntry {
        Bitboard hash;
	    Bitboard mask;
	    Bitboard shift;
	    //this points to the BishopAttack/RookAttack arrays
	    Bitboard* offset;
    };
   /**
    * LERF (little endian rank file) enumeration for ranks, files, squares, diagonals, et cetera
    * The little-endianness is somewhat awkward to write out, but 
    * more than makes up for it with the convenience of ordering it gives.
    * We combine these bit representations with each other using basic binary math to encode squares in an efficient way!
    */
    enum Rank : uint64_t {
	    Rank1 = 0x00000000000000FFull,
	    Rank2 = 0x000000000000FF00ull,
	    Rank3 = 0x0000000000FF0000ull,
	    Rank4 = 0x00000000FF000000ull,
	    Rank5 = 0x000000FF00000000ull,
	    Rank6 = 0x0000FF0000000000ull,
	    Rank7 = 0x00FF000000000000ull,
	    Rank8 = 0xFF00000000000000ull
    };

    enum File : uint64_t {
	    FileA = 0x0101010101010101ull,
	    FileB = 0x0202020202020202ull,
	    FileC = 0x0404040404040404ull,
	    FileD = 0x0808080808080808ull,
	    FileE = 0x1010101010101010ull,
	    FileF = 0x2020202020202020ull,
	    FileG = 0x4040404040404040ull,
	    FileH = 0x8080808080808080ull
    };

    enum SquareColor : uint64_t {
	    LightSquares = 0x55AA55AA55AA55AAull,
	    DarkSquares = 0xAA55AA55AA55AA55ull
    };

    //other board LERF mappings that aren't as important (hence no name) but still nice to have
    enum : uint64_t {
	    a1h8Diagonal = 0x8040201008040201ull,
	    h1a8Diagonal = 0x0102040810204080ull,
	    MainDiagonals = a1h8Diagonal | h1a8Diagonal,
	    CenterFour =  (FileD | FileE) & (Rank4 | Rank5),
	    CenterSixteen = (FileC | FileD | FileE | FileF) & (Rank3 | Rank4 | Rank5 | Rank6),
	    LeftSide = FileA | FileB | FileC | FileD,
	    RightSide = FileE | FileF | FileG | FileH,
	    LastRanks = Rank1 | Rank8
    };

    static inline Rank getRank(int index) {
        static const Rank ranks[NumRanks] = {Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8};
        assert(index >= 0 && index < NumRanks);
        return ranks[index];
    }

    static inline File getFile(int index) {
        static const File files[NumFiles] = {FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH};
        assert(index >= 0 && index < NumFiles);
        return files[index];
    }

    /**
     * In what follows, we generally distinguish between rank/files indices (which are 0-7 ints, this enum just exists for type checking)
     * and the LERF representations (which we can't do quick integer math with to convert between).
     * This is the one drawback involved in our close coupling between bitboard representation and board things
     * but well worth it! 
     */
    enum Index {
	    Zero = 0, One, Two, Three, Four, Five, Six, Seven
    };

    /**
     * Convert between squares and rank/files
     */ 
    static Index getFileIndexOfSquare(Square square);
    static Index getRankIndexOfSquare(Square square);
    static Square getSquare(Index rankIndex, Index fileIndex);
    
    /**
     * Get squares/rank/files relative to the side's perspective
     */ 
    static Index getMirrorFileIndex(Index fileIndex);
    static Index getRelativeRankIndexOfSquare(Color side, Square square);
    static Square getRelativeSquare(Color side, Square square);
    static Square getRelativeSquare32(Color side, Square square);
    
    static SquareColor getSquareColor(Square square);
    static std::string squareToString(Square square);

    /**
     * Factory object pattern. Creates a board object from a FEN (the copy is intentional!), OPERATES UNDER THE INVARIANT THAT THE FEN IS WELL-FORMED
     */ 
    static Board createBoardFromFEN(std::string fen);

    std::string getFEN() const;
    ColorPiece getPieceAt(Square square) const;
    Square getKing() const;
    Color getTurn() const;

    bool hasNonPawns(Color side) const;
    bool isDrawn(int threefoldHeight) const;
    bool isFiftyMoveRuleDraw() const;
    bool isThreefoldDraw(int threefoldHeight) const;
    bool isInsufficientMaterialDraw() const;
   
    void setSquare(Color color, Piece piece, Square square);
    static Square squareFromString(const std::string& string);
    
    /**
     * Returns whether or not the move was illegal (if it was, it rejects the move)
     */ 
    bool applyMove(Move& move);
    int countLegalMoves();
    //TODO: plenty of things that help us later can go here
    //like determining if a move is tactical or not
    bool isMoveLegal(Move& move);
    bool isMovePseudoLegal(Move& move);
    /**
     * ASSUMES MOST RECENT MOVE WAS PSEUDO-LEGAL
     */
    bool wasMostRecentMoveLegal();
 
    bool isSquareAttacked(Square square, Color side);
    unsigned long long perftTest(int depth);
private:
    Board(); //private constructor to force client to use static method

    std::array<ColorPiece, NumSquares> squares;
    //TODO: whoever is implementing zobrist, implement these
    uint64_t positionHash;
    uint64_t pawnKingHash;
    //Bitboards for each of the pieces +  TODO: others (pieces are implemented)
    std::array<Bitboard, 8> pieces;
    //Bitboards for each side's pieces (and empty)
    std::array<Bitboard, 3> sides;
    //Bitboard for where the king is attacked (TODO: useful later, not implemented now)
    Bitboard kingAttackers;
    //Bitboards corresponding to rooks that can castle (non promoted ones)
    Bitboard castlingRooks;
    std::array<Bitboard, NumSquares> castleMasks;
    
    //the current turn, half move counter (called plies in chess programming land), and full move counter (1 move = 2 plies)
    Color turn;
    int plies;
    int fullmoves;
    int moveCounter;
    Square enpassantSquare;

    /**
     * Some annoying to recompute data for undoing a move
     */ 
    struct UndoData {
        uint64_t positionHash;
	    uint64_t pawnKingHash;
	    Bitboard kingAttackers;
	    Bitboard castlingRooks;
	    Square enpassantSquare;
	    int plies;
	    //TODO: psqt?
	    ColorPiece pieceCaptured;
    };
    std::vector<UndoData> undoStack;

    static Square getKingCastlingSquare(Square king, Square rook);
    static Square getRookCastlingSquare(Square king, Square rook);

    void applyLegalMove(Move& move);
    void applyMoveWithUndo(Move& move, UndoData& undo);
    void applyNormalMoveWithUndo(Move& move, UndoData& undo);
    void applyCastlingMoveWithUndo(Move& move, UndoData& undo);
    void applyEnpassantMoveWithUndo(Move& move, UndoData& undo);
    void applyPromotionMoveWithUndo(Move& move, UndoData& undo);
    //TODO: null move would go here
    /**
     * This takes the most recent move from the undo stack
     */ 
    void revertMostRecent(Move& move);
    void revertMove(Move& move, UndoData& undo);

    static void initializeStaticAttacks(); 

    inline static int computeHashTableIndex(Bitboard occupiedBoard, HashEntry& entry) {
        return ((occupiedBoard & entry.mask) * entry.hash) >> entry.shift;
    }

    //With these, we can do what is necessary to determine all the attacks
    Bitboard getAllSquareAttackers(Bitboard occupiedBoard, Square square);
    Bitboard getAllKingAttackers();

    //Some special things for pawns (their rules are weird)
    static Bitboard getPawnLeftAttacks(Bitboard pawnBoard, Bitboard targets, Color side);
    static Bitboard getPawnRightAttacks(Bitboard pawnBoard, Bitboard targets, Color side);
    static Bitboard getPawnAdvances(Bitboard pawnBoard, Bitboard occupiedBoard, Color side);
    static Bitboard getPawnEnpassantCaptures(Bitboard pawnBoard, Square enpassantSquare, Color side);
    
    //don't need versions of these methods for PKN since they are just simple array accesses
    static Bitboard getBishopAttacksFromSquare(Square square, Bitboard occupiedBoard);
    static Bitboard getRookAttacksFromSquare(Square square, Bitboard occupiedBoard);
    static Bitboard getQueenAttacksFromSquare(Square square, Bitboard occupiedBoard);
    
    /**
     * Tries to set a prospective bitboard square (if coordinates are valid) and does nothing otherwise (this is how we check to make sure things like knights don't go off the side of the edge)
     */ 
    static void setBitboardSquare(Bitboard& board, int rank, int file);

    //TODO: Write things for SEE and discovered attacks

    //For some attack things, we need to consider some fixed size multidimensional arrays.
    //C-style versions of those suck, so let's do something better:
    template <class T, size_t I, size_t J> using MultiArray = std::array<std::array<T, J>, I>;

    static Bitboard calculateRookBishopAttacks(Square square, Bitboard occupiedBoard, const MultiArray<int, 4, 2>& movementDelta);
    static void populateHashTable(HashEntry* table, Square square, Bitboard hash, const MultiArray<int, 4, 2>& movementDelta);
    /**
     * Some binary utilities (that implementation wise should leverage compiler intrinsics)
     */ 
    static int popCnt(Bitboard bb);
    static int getMsb(Bitboard bb);
    static int getLsb(Bitboard bb);
    static int popLsb(Bitboard& bb);
    static int popMsb(Bitboard& bb);
    /**
     * Does the bitboard contain more than one 1
     */ 
    static bool isNonSingular(Bitboard bb);

    static void setBit(Bitboard& bb, Square bit);
    static void clearBit(Bitboard& bb, Square bit);
    static bool testBit(Bitboard bb, Square bit);

    static void debugPrintBitboard(Bitboard bb);

};

#endif
