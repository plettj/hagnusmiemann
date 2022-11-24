#ifndef _BOARD_H
#define _BOARD_H
#include <cassert>
#include <iostream>
#include <string>
#include <array>
#include <vector>


class Move;

typedef uint64_t Bitboard;

//For some attack things, we need to consider some fixed size multidimensional arrays.
//C-style versions of those suck, so let's do something better:
template <class T, size_t I, size_t J> using MultiArray = std::array<std::array<T, J>, I>;

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
    };

    static inline Color flipColor(Color color) {
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
	    assert(-1 <= square && square <= 63);
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
    inline static Square getSquare(int rankIndex, int fileIndex) {
        assert(0 <= rankIndex && rankIndex < NumRanks && 0 <= fileIndex && fileIndex < NumFiles);
        return getSquare(static_cast<Index>(rankIndex), static_cast<Index>(fileIndex));
    }
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
   
    /**
     * Returns number of moves added to the vector. 
     */
    int generateAllLegalMoves(std::vector<Move>& moveList);

    void setSquare(Color color, Piece piece, Square square);
    static Square squareFromString(const std::string& string);
    
    /**
     * Returns whether or not the move was legal (if it was not, it rejects the move)
     */ 
    bool applyMove(Move& move);
    int countLegalMoves(); // TODO
    //TODO: plenty of things that help us later can go here
    //like determining if a move is tactical or not
    bool isMoveLegal(Move& move);
    bool isMovePseudoLegal(Move& move);
    /**
     * Checks legality of last move assuming it was pseudo-legal
     */
    bool didLastMoveLeaveInCheck();
 
    bool isSquareAttacked(Square square, Color side); // the side of the piece on the square, not the attacking team.
    unsigned long long perftTest(int depth);
private:
    /**
     * Your friendly neighbourhood singleton of computation stuff.
     * This doesn't really do anything special versus lots of static data in the .cc file, but is more extensible
     * (replacing these are the bulk of the work involved in move generation, so extending this and changing the singleton slightly is much easier)
     */
    class PrecomputedBinary {
    public:
        static PrecomputedBinary& getBinary() {
            static PrecomputedBinary instance; 
            return instance;
        }
        void init();
        PrecomputedBinary(const PrecomputedBinary& other) = delete;
        void operator=(const PrecomputedBinary& other) = delete;
        /**
         * Encapsulate the array accesses
         */
        Bitboard getPawnAttacksFromSquare(Square square, Color side);
        Bitboard getKnightAttacksFromSquare(Square square);
        Bitboard getKingAttacksFromSquare(Square square);
        Bitboard getBishopAttacksFromSquare(Square square, Bitboard occupiedBoard);
        Bitboard getRookAttacksFromSquare(Square square, Bitboard occupiedBoard);
        Bitboard getQueenAttacksFromSquare(Square square, Bitboard occupiedBoard);
        Bitboard getBetweenSquaresMask(Square square1, Square square2);
        Bitboard getAdjacentFilesMask(Index fileIndex);
    private: 
        bool hasBeenInitialized = false;
        PrecomputedBinary();
        /**
         * Some binary masks (probably need more in future) to do fast computations
         */
        
        MultiArray<Bitboard, NumSquares, NumSquares> BetweenSquaresMasks;
        std::array<Bitboard, NumFiles> AdjacentFilesMasks;
        /**
         * The following are what correspond to the functionality to determine (very efficiently!) if a square is attacked or not.
         * These are arrays of bitboards corresponding to where we can attack with a piece on a given square.
         * Kings, Pawns, and Knights are very simple to determine where they can attack (it's a hard and fast rule, they are never blocked by anything). 
         */
        alignas(64) Bitboard PawnAttack[Board::NumColors][Board::NumSquares]; //these are sided based on direction, since pawns are the only piece that aren't symmetric
        alignas(64) Bitboard KnightAttack[Board::NumSquares];
        alignas(64) Bitboard KingAttack[Board::NumSquares];
        
        /**
         * Bishops and rooks (and queens), on the other hand, are blocked by annoying things called enemy pieces.
         * Now, we could compute this at runtime. But that's slow and bad (no seriously TAs/instructors, if you write a chess program that does that, that's very bad)
         * Thus, we use a strategy called Magic Bitboards.
         * The basic idea is this: Suppose we have a rook on e5. The only possible places it can possibly move are the 5th rank and the e file.
         * Thus, we mask out all other bits except the 5th rank and efile.
         * Then, the other key observation is that a rank like 0001R100, where R is a rook and 1s are pieces on the rank, has the same possible moves
         * as 1111R111, since it being blocked means the other ranks are irrelevant. 
         * So what can we do to take advantage of this? Instead of a full lookup table (like the others), we can do a hashtable, where we _want_ hash collisions
         * whenever two outputs are identical, but CANNOT have hash collisions whenever two outputs are different. 
         * To do this, it more or less involves just taking our simplified masked board, multiplying it by a precomputed board that gives such a hash,
         * and then shifting it as necessary to get the square that is attacked. 
         * Here are our precomputed boards (aligned LERF as in Square).
         * Citation: For these specific precomputed boards, I (Alex) was given them by Terje Kir, the author of the chess program Weiss in 2020 when I was working on badchessengine,
         * an engine I wrote in 2019/2020.
         */ 
        const Bitboard RookHashes[Board::NumSquares] = {
            0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull, 0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
            0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull, 0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
            0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull, 0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
            0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull, 0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
            0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull, 0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
            0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull, 0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
            0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull, 0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
            0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull, 0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull
        };
        const Bitboard BishopHashes[Board::NumSquares] = {
            0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull, 0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
            0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull, 0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
            0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull, 0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
            0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull, 0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
            0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull, 0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
            0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull, 0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
            0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull, 0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
            0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull, 0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull
        };

        //these are the big arrays storing the actual possibilities
        alignas(64) Bitboard BishopAttack[0x1480];
        alignas(64) Bitboard RookAttack[0x19000];
        //these are the hash tables, indexed by square
        alignas(64) HashEntry BishopTable[NumSquares];
        alignas(64) HashEntry RookTable[NumSquares];
        Bitboard calculateRookBishopAttacks(Square square, Bitboard occupiedBoard, const MultiArray<int, 4, 2>& movementDelta);
        void populateHashTable(HashEntry* table, Square square, Bitboard hash, const MultiArray<int, 4, 2>& movementDelta);
        inline int computeHashTableIndex(Bitboard occupiedBoard, HashEntry& entry) {
            return ((occupiedBoard & entry.mask) * entry.hash) >> entry.shift;
        }  
    };
    Board(); //private constructor to force client to use factory method

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
     * All of these methods take every square on the given Bitboard as a target square (iterating over them),
     * and add them to the movelist (where the from is as specified, done implicity for pawns)
    */
    void addEnpassantMoves(std::vector<Move>& moveList, Bitboard sources, Square enpassantSquare);
    void addPawnMoves(std::vector<Move>& moveList, Bitboard targets, int directionPawnIsIn);
    void addPawnPromotions(std::vector<Move>& moveList, Bitboard targets, int directionPawnIsIn);
    /**
     *  Helper function used by the below two functions once they have their stuff figured out
     */
    void addNormalMoves(std::vector<Move>& moveList, Bitboard targets, Square from);
    /**
     * type must be Knight, Bishop, Rook, King, will error otherwise 
     */
    void addNonPawnNormalMoves(std::vector<Move>& moveList, Piece type, Bitboard targets, Bitboard sources, Bitboard occupiedBoard);

    /**
     * A "noisy" move is a capture, promotion, pawn push.
     * Returns number of moves added to vector.
     */
    int generateAllNoisyMoves(std::vector<Move>& moveList);
    /**
     * A quiet move is what's left from the list above.
     * Returns number of moves added to vector.
     */
    int generateAllQuietMoves(std::vector<Move>& moveList);
    bool debugIsSquareAttacked(Square square, Board::Color side);
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

    bool isSquareInBoardAttacked(Bitboard board, Color turn);
    //With these, we can do what is necessary to determine all the attacks
    Bitboard getAllSquareAttackers(Bitboard occupiedBoard, Square square);
    Bitboard getAllKingAttackers();

    //Some special things for pawns (their rules are weird)
    static Bitboard getPawnLeftAttacks(Bitboard pawnBoard, Bitboard targets, Color side);
    static Bitboard getPawnRightAttacks(Bitboard pawnBoard, Bitboard targets, Color side);
    static Bitboard getPawnAdvances(Bitboard pawnBoard, Bitboard occupiedBoard, Color side);
    static Bitboard getPawnEnpassantCaptures(Bitboard pawnBoard, Square enpassantSquare, Color side);
        
    /**
     * Tries to set a prospective bitboard square (if coordinates are valid) and does nothing otherwise (this is how we check to make sure things like knights don't go off the side of the edge)
     */ 
    static void setBitboardSquare(Bitboard& board, int rank, int file);

    //TODO: Write things for SEE and discovered attacks
    
    /**
     * Some binary utilities (that implementation wise should leverage compiler intrinsics)
     */ 
    static int popCnt(Bitboard bb);
    static int getMsb(Bitboard bb);
    static int getLsb(Bitboard bb);
    static int popLsb(Bitboard& bb);
    static int popMsb(Bitboard& bb);
    /**
     * Does the bitboard contain more than one 1, just a faster version of popCnt > 1
     */ 
    static bool isNonSingular(Bitboard bb);

    static void setBit(Bitboard& bb, Square bit);
    static void clearBit(Bitboard& bb, Square bit);
    static bool testBit(Bitboard bb, Square bit);

    static void debugPrintBitboard(Bitboard bb);
};

#endif
