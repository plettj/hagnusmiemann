#include "board.h"
#include "move.h"
#include "zobrist.h"
#include "evaluator.h"
#include <algorithm>
#include <map>
#include <chrono>

Index Board::getFileIndexOfSquare(Square square) {
    assert(square != None);
    //squares are laid out sequentially in rank, so their file is mod 8
    return static_cast<Index>(square % NumFiles);
}

Index Board::getRankIndexOfSquare(Square square) {
    assert(square != None);
    //ranks are groups of 8 (numFiles) in the layout, so truncate and divide
    return static_cast<Index>(square / NumFiles);
}

Index Board::getMirrorFileIndex(Index fileIndex) {
    //compile time optimization
    /*const static Index Mirror[] = {Zero, One, Two, Three, Three, Two, One};
    return Mirror[fileIndex];*/
    return fileIndex > 3 ? (static_cast<Index>(7 - fileIndex)) : fileIndex;
}

Index Board::getRelativeRankIndexOfSquare(Color side, Square square) {
    assert(square != None);
    //get the rank our piece is on relative to our starting side
    return side == White ? getRankIndexOfSquare(square) : static_cast<Index>(7 - getRankIndexOfSquare(square));
}

Square Board::getSquare(Index rankIndex, Index fileIndex) {
    return getSquare(rankIndex * NumFiles + fileIndex);
}

Square Board::getRelativeSquare(Color side, Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side
    return getSquare(getRelativeRankIndexOfSquare(side, square), getFileIndexOfSquare(square));
}

Square Board::getRelativeSquare32(Color side, Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side and limited to the left half of the board
    return getSquare(4 * getRelativeRankIndexOfSquare(side, square) + getMirrorFileIndex(getFileIndexOfSquare(square)));
}

Board::SquareColor Board::getSquareColor(Square square) {
    assert(square != None);
    return testBit(LightSquares, square) ? LightSquares : DarkSquares; 
}

//Some fast compiler intrinsics for bit math
int Board::popCnt(Bitboard bb) {
    return __builtin_popcountll(bb);
}

int Board::getMsb(Bitboard bb) {
    assert(bb); //msb is undefined for zero
    //count leading zeroes and invert to get index
    return __builtin_clzll(bb) ^ 63;
}	

int Board::getLsb(Bitboard bb) {
    assert(bb); //lsb is undefined for zero
    //count trailing zeroes
    return __builtin_ctzll(bb);
}

int Board::popMsb(Bitboard& bb) {
    //get the msb, xor it out, then return it	
    int msb = getMsb(bb);
    bb ^= 1ull << msb;
    return msb;
}

int Board::popLsb(Bitboard& bb) {
    //get the lsb, and it out, then return it
    int lsb = getLsb(bb);
    bb &= bb - 1;
    return lsb;
}

bool Board::isNonSingular(Bitboard bb) {
    //more than one 1 only if both it and its predecessor have a 1	
    return bb & (bb - 1);
}

void Board::setBit(Bitboard& bb, Square bit) {
    assert(!testBit(bb, bit));
    bb ^= 1ull << bit;
}

void Board::clearBit(Bitboard& bb, Square bit) {
    assert(testBit(bb, bit));
    bb ^= 1ull << bit;
}

bool Board::testBit(Bitboard bb, Square bit) {
    assert(bit != None);
    return bb & (1ull << bit);
}

void Board::debugPrintBitboard(Bitboard bb) {
    for(int i = NumRanks - 1; i >= 0; i--) {
        for(int j = 0; j < NumFiles; j++) {
	        std::cout << testBit(bb, getSquare(i, j));
	    }
	    std::cout << std::endl;
    }
    std::cout << std::endl;
}

bool Board::hasNonPawns(Color side) const {
    return (sides[side] & (pieces[King] & pieces[Pawn])) != sides[side];
}

bool Board::isDrawn() const {
    return isFiftyMoveRuleDraw() || isThreefoldDraw() || isInsufficientMaterialDraw();
}

bool Board::isFiftyMoveRuleDraw() const {
    return plies > 99; 
}

bool Board::isThreefoldDraw() const {
    int repetitions = 0;
    //Look through all the moves played thus far
    for(int i = fullmoves - 2; i >= 0; i -= 2) {
        //we can no longer repeat if a fifty-move-rule resetting move was made
        if(i < fullmoves - plies) {
            break;
        }
        if(undoStack[i].positionHash == positionHash) {
            if(++repetitions == 3) {
                return true;
            }
        }    
    }

    return false;
}

bool Board::isInsufficientMaterialDraw() const {
    //insuff material is only forced if one side has only 1 knight, 1 bishop, or 2 knights	
    return !(pieces[Queen] | pieces[Rook] | pieces[Pawn]) && (!isNonSingular(sides[White]) || !isNonSingular(sides[Black])) && (!isNonSingular(pieces[Bishop] | pieces[Knight]) || (popCnt(pieces[Knight]) <= 2 && !pieces[Bishop])); 
}

std::string Board::squareToString(Square square) {
    std::string output;
    if(square == None) {
        output.append("-");
    } else {
        output += 'a' + getFileIndexOfSquare(square);
        output += '1' + getRankIndexOfSquare(square);
    }
    return output;
}

Square Board::squareFromString(const std::string& string) {
    assert(string.size() <= 2);
    if(string == "-") {
        return None;
    }
    return getSquare(string[1] - '1', string[0] - 'a');
}

Board::Board() : positionHash{0}, kingAttackers{0}, castlingRooks{0}, turn{White}, plies{0}, fullmoves{0}, enpassantSquare{None} {
    for(int i = 0; i < 6; i++) {
        pieces[i] = 0;
    }
    for(int i = 0; i < 2; i++) {
        sides[i] = 0;
    }
    for(int i = 0; i < 64; i++) {
        squares[i] = Empty;
	    castleMasks[i] = 0;
    }
    //shorter thing to type, who said programmers weren't lazy
    PrecomputedBinary::getBinary().init();
}

void Board::setBitboardSquare(Bitboard& board, int rankIndex, int fileIndex) {
    if(0 <= rankIndex && rankIndex < NumRanks && 0 <= fileIndex && fileIndex < NumFiles) {
        board |= 1ull << getSquare(rankIndex, fileIndex);        
    }
}

//Trivial constructor since we super care about Board's constructor body running BEFORE we initialize this
Board::PrecomputedBinary::PrecomputedBinary() { }
void Board::PrecomputedBinary::init() {
    if(hasBeenInitialized) {
        return;
    }

    //Initialize attacks:
    //all possible movements of pieces
    const MultiArray<int, 2, 2> pawnAttackDelta{ std::array<int, 2>{1, -1}, std::array<int, 2>{1, 1} };
    const MultiArray<int, 8, 2> knightMovementDelta{ std::array<int, 2>{-2, -1}, std::array<int, 2>{-2, 1}, std::array<int, 2>{-1, -2}, std::array<int, 2>{-1, 2}, std::array<int, 2>{1, -2}, std::array<int, 2>{1, 2}, std::array<int, 2>{2, -1}, std::array<int, 2>{2, 1} };
    const MultiArray<int, 8, 2> kingMovementDelta{ std::array<int, 2>{-1,-1}, std::array<int, 2>{-1, 0}, std::array<int, 2>{-1, 1},  std::array<int, 2>{0,-1}, std::array<int, 2>{0, 1}, std::array<int, 2>{1,-1}, std::array<int, 2>{1, 0}, std::array<int, 2>{1, 1} };
    const MultiArray<int, 4, 2> bishopMovementDelta{ std::array<int, 2>{-1, -1}, std::array<int, 2>{-1, 1}, std::array<int, 2>{1, -1}, std::array<int, 2>{1, 1} };
    const MultiArray<int, 4, 2> rookMovementDelta{ std::array<int, 2>{-1, 0}, std::array<int, 2>{0, -1}, std::array<int, 2>{0, 1}, std::array<int, 2>{1, 0} };

    //we will use the arrays we have allocated already as the initial offsets
    RookTable[a1].offset = RookAttack;
    BishopTable[a1].offset = BishopAttack;
    
    //In what follows, we initialize the attack bitboards.
    //For the basic pieces, we just compute their movements, check if that movement gives valid coordinates (as checked in setBitboardSquare),
    //and if so set the respective bitboard.

    for(int square = a1; square <= h8; square++) {
	    Square sq = getSquare(square);
        for(int direction = 0; direction < 2; direction++) {
	        setBitboardSquare(PawnAttack[White][square], getRankIndexOfSquare(sq) + pawnAttackDelta[direction][0], getFileIndexOfSquare(sq) + pawnAttackDelta[direction][1]);
	        setBitboardSquare(PawnAttack[Black][square], getRankIndexOfSquare(sq) - pawnAttackDelta[direction][0], getFileIndexOfSquare(sq) - pawnAttackDelta[direction][1]);
	        setBitboardSquare(KnightAttack[square], getRankIndexOfSquare(sq) + knightMovementDelta[direction][0], getFileIndexOfSquare(sq) + knightMovementDelta[direction][1]);
	        setBitboardSquare(KingAttack[square], getRankIndexOfSquare(sq) + kingMovementDelta[direction][0], getFileIndexOfSquare(sq) + kingMovementDelta[direction][1]);
        }
	    for(int direction = 2; direction < 8; direction++) {
            setBitboardSquare(KnightAttack[square], getRankIndexOfSquare(sq) + knightMovementDelta[direction][0], getFileIndexOfSquare(sq) + knightMovementDelta[direction][1]);
	        setBitboardSquare(KingAttack[square], getRankIndexOfSquare(sq) + kingMovementDelta[direction][0], getFileIndexOfSquare(sq) + kingMovementDelta[direction][1]);
	    }
        populateHashTable(BishopTable, sq, BishopHashes[sq], bishopMovementDelta);
	    populateHashTable(RookTable, sq, RookHashes[sq], rookMovementDelta);
    }
    //Initialize masks (depend on attacks being done)
    for(int square1 = a1; square1 <= h8; square1++) {
        for(int square2 = a1; square2 <= h8; square2++) {
            //Align this on rank/file if the squares are straight away
            if(testBit(getRookAttacksFromSquare(getSquare(square1), 0), getSquare(square2))) {
                BetweenSquaresMasks[square1][square2] = getRookAttacksFromSquare(getSquare(square1), 1ull << square2) & getRookAttacksFromSquare(getSquare(square2), 1ull << square1);
            }
            //Align this on diagonal if the squares are diagonally away
            if(testBit(getBishopAttacksFromSquare(getSquare(square1), 0), getSquare(square2))) {
                BetweenSquaresMasks[square1][square2] = getBishopAttacksFromSquare(getSquare(square1), 1ull << square2) & getBishopAttacksFromSquare(getSquare(square2), 1ull << square1);
            }
        }
    }

    //Populate masks that indicate adjacent files
    for(int file = 0; file < NumFiles; file++) {
        //The max and min deal with the edge case (literally)
        //And we don't include the current file
        AdjacentFilesMasks[file] = getFile(std::max(0, file - 1));
        AdjacentFilesMasks[file] |= getFile(std::min(NumFiles - 1, file + 1));
        AdjacentFilesMasks[file] &= ~getFile(file);
    }

    for(int color = 0; color < NumColors; color++) {
        for(int square = 0; square < NumSquares; square++) {
            Bitboard files = AdjacentFilesMasks[getFileIndexOfSquare(getSquare(square))] | getFile(getFileIndexOfSquare(getSquare(square)));
            Bitboard ranks = 0;
            for(int rank = getRankIndexOfSquare(getSquare(square)); 0 <= rank && rank < NumRanks; White == color ? rank-- : rank++) {
                ranks |= getRank(rank);
            }
            PassedPawnMasks[color][square] = ~ranks & files;
        }
    }

    hasBeenInitialized = true;
}

Bitboard Board::PrecomputedBinary::getPassedPawnMask(Color side, Square square) {
    assert(square != None);
    return PassedPawnMasks[side][square];
}

Bitboard Board::PrecomputedBinary::getBetweenSquaresMask(Square square1, Square square2) {
    assert(square1 != None && square2 != None);
    return BetweenSquaresMasks[square1][square2];
}

Bitboard Board::PrecomputedBinary::getAdjacentFilesMask(Index fileIndex) {
    return AdjacentFilesMasks[fileIndex];
}

Bitboard Board::PrecomputedBinary::getKnightAttacksFromSquare(Square square) {
    return KnightAttack[square];
}

Bitboard Board::PrecomputedBinary::getKingAttacksFromSquare(Square square) {
    return KingAttack[square];
}

Bitboard Board::PrecomputedBinary::getPawnAttacksFromSquare(Square square, Color side) {
    return PawnAttack[side][square];
}

Bitboard Board::PrecomputedBinary::getBishopAttacksFromSquare(Square square, Bitboard occupiedBoard) {
    return BishopTable[square].offset[computeHashTableIndex(occupiedBoard, BishopTable[square])];
}

Bitboard Board::PrecomputedBinary::getRookAttacksFromSquare(Square square, Bitboard occupiedBoard) {
    return RookTable[square].offset[computeHashTableIndex(occupiedBoard, RookTable[square])];
}

Bitboard Board::PrecomputedBinary::getQueenAttacksFromSquare(Square square, Bitboard occupiedBoard) {
    return getBishopAttacksFromSquare(square, occupiedBoard) | getRookAttacksFromSquare(square, occupiedBoard);
}

Bitboard Board::getPawnLeftAttacks(Bitboard pawnBoard, Bitboard targets, Color side) {
    return targets & (side == White ? (pawnBoard << 7) & ~FileH : (pawnBoard >> 7) & ~FileA);
}

Bitboard Board::getPawnRightAttacks(Bitboard pawnBoard, Bitboard targets, Color side) {
    return targets & (side == White ? (pawnBoard << 9) & ~FileA : (pawnBoard >> 9) & ~FileH);
}

Bitboard Board::getPawnAdvances(Bitboard pawnBoard, Bitboard occupiedBoard, Color side) {
    return ~occupiedBoard & (side == White ? pawnBoard << 8 : pawnBoard >> 8);
}

Bitboard Board::getPawnEnpassantCaptures(Bitboard pawnBoard, Square enpassantSquare, Color side) {
    return (enpassantSquare == None) ? 0 : PrecomputedBinary::getBinary().getPawnAttacksFromSquare(enpassantSquare, flipColor(side)) & pawnBoard;
}

Bitboard Board::getAllSquareAttackers(Bitboard occupiedBoard, Square square) const {
    return (PrecomputedBinary::getBinary().getPawnAttacksFromSquare(square, White) & sides[Black] & pieces[Pawn])
	   | (PrecomputedBinary::getBinary().getPawnAttacksFromSquare(square, Black) & sides[White] & pieces[Pawn])
	   | (PrecomputedBinary::getBinary().getKnightAttacksFromSquare(square) & pieces[Knight])
	   | (PrecomputedBinary::getBinary().getBishopAttacksFromSquare(square, occupiedBoard) & (pieces[Bishop] | pieces[Queen]))
	   | (PrecomputedBinary::getBinary().getRookAttacksFromSquare(square, occupiedBoard) & (pieces[Rook] | pieces[Queen]))
	   | (PrecomputedBinary::getBinary().getKingAttacksFromSquare(square) & pieces[King]);
}

Bitboard Board::getAllKingAttackers() {
    Square square = getSquare(getLsb(sides[turn] & pieces[King]));
    Bitboard occupiedBoard = sides[White] | sides[Black];
    return getAllSquareAttackers(occupiedBoard, square) & sides[flipColor(turn)];
}

bool Board::isSquareAttacked(Square square, Color side) {
    Bitboard enemyPieces = sides[flipColor(side)];
    Bitboard occupiedBoard = sides[White] | sides[Black];

    Bitboard enemyPawns = enemyPieces & pieces[Pawn];
    Bitboard enemyKnights = enemyPieces & pieces[Knight];
    Bitboard enemyBishops = enemyPieces & (pieces[Bishop] | pieces[Queen]);
    Bitboard enemyRooks = enemyPieces & (pieces[Rook] | pieces[Queen]);
    Bitboard enemyKings = enemyPieces & pieces[King];

    //avoid doing hash lookups via short circuit if we can
    return ((PrecomputedBinary::getBinary().getPawnAttacksFromSquare(square, side) & enemyPawns) != 0) 
    || ((PrecomputedBinary::getBinary().getKnightAttacksFromSquare(square) & enemyKnights) != 0)
    || ((PrecomputedBinary::getBinary().getKingAttacksFromSquare(square) & enemyKings) != 0)
    || ((enemyBishops != 0) && ((PrecomputedBinary::getBinary().getBishopAttacksFromSquare(square, occupiedBoard) & enemyBishops) != 0)) 
    || ((enemyRooks != 0) && ((PrecomputedBinary::getBinary().getRookAttacksFromSquare(square, occupiedBoard) & enemyRooks) != 0));
}

bool Board::debugIsSquareAttacked(Square square, Color side) {
    Bitboard enemyPieces = sides[flipColor(side)];
    Bitboard occupiedBoard = sides[White] | sides[Black];

    Bitboard enemyPawns = enemyPieces & pieces[Pawn];
    Bitboard enemyKnights = enemyPieces & pieces[Knight];
    Bitboard enemyBishops = enemyPieces & (pieces[Bishop] | pieces[Queen]);
    Bitboard enemyRooks = enemyPieces & (pieces[Rook] | pieces[Queen]);
    Bitboard enemyKings = enemyPieces & pieces[King];

    bool attackedByPawns = (PrecomputedBinary::getBinary().getPawnAttacksFromSquare(square, side) & enemyPawns) != 0;
    bool attackedByKnights = (PrecomputedBinary::getBinary().getKnightAttacksFromSquare(square) & enemyKnights) != 0;
    bool attackedByKings = (PrecomputedBinary::getBinary().getKingAttacksFromSquare(square) & enemyKings) != 0;
    bool attackedByBishops = ((enemyBishops != 0) && ((PrecomputedBinary::getBinary().getBishopAttacksFromSquare(square, occupiedBoard) & enemyBishops) != 0));
    bool attackedByRooks = ((enemyRooks != 0) && ((PrecomputedBinary::getBinary().getRookAttacksFromSquare(square, occupiedBoard) & enemyRooks) != 0));

    debugPrintBitboard(enemyPieces);
    std::cerr << std::endl;
    debugPrintBitboard(enemyKnights);
    std::cerr << std::endl;
    std::cerr << attackedByPawns << " " << attackedByKnights << " " << attackedByKings << " " << attackedByBishops << " " << attackedByRooks << std::endl;
    return attackedByPawns || attackedByKnights || attackedByKings || attackedByBishops || attackedByRooks;
}

bool Board::setCastlingRight(Color side, bool isKingside) {
    if(side == White && isKingside) {
        if((sides[White] & pieces[Rook] & Rank1) == 0) {
            return false;
        }
        if(!testBit(castlingRooks, getSquare(getMsb(sides[White] & pieces[Rook] & Rank1)))) {
	        setBit(castlingRooks, getSquare(getMsb(sides[White] & pieces[Rook] & Rank1))); 
            return true;
        }
	} else if(side == White && !isKingside) {
        if((sides[White] & pieces[Rook] & Rank1) == 0) {
            return false;
        }
        if(!testBit(castlingRooks, getSquare(getLsb(sides[White] & pieces[Rook] & Rank1)))) {
            setBit(castlingRooks, getSquare(getLsb(sides[White] & pieces[Rook] & Rank1)));
            return true;
        }
    } else if(isKingside) {
        if((sides[Black] & pieces[Rook] & Rank8) == 0) {
            return false;
        }
        if(!testBit(castlingRooks, getSquare(getMsb(sides[Black] & pieces[Rook] & Rank8)))) {
	        setBit(castlingRooks, getSquare(getMsb(sides[Black] & pieces[Rook] & Rank8))); 
            return true;
        }    
    } else {
        if((sides[Black] & pieces[Rook] & Rank8) == 0) {
            return false;
        }
        if(!testBit(castlingRooks, getSquare(getLsb(sides[Black] & pieces[Rook] & Rank8)))) {
            setBit(castlingRooks, getSquare(getLsb(sides[Black] & pieces[Rook] & Rank8)));
            return true;
        }    
    }
    return false;
}

bool Board::clearCastlingRight(Color side, bool isKingside) {
    if(side == White && isKingside) {
        if((sides[White] & pieces[Rook] & Rank1) == 0) {
            return false;
        }
        if(testBit(castlingRooks, getSquare(getMsb(sides[White] & pieces[Rook] & Rank1)))) {
	        clearBit(castlingRooks, getSquare(getMsb(sides[White] & pieces[Rook] & Rank1)));
            return true; 
        }
	} else if(side == White && !isKingside) {
        if((sides[White] & pieces[Rook] & Rank1) == 0) {
            return false;
        }
        if(testBit(castlingRooks, getSquare(getLsb(sides[White] & pieces[Rook] & Rank1)))) {
            clearBit(castlingRooks, getSquare(getLsb(sides[White] & pieces[Rook] & Rank1)));
            return true;
        }
    } else if(isKingside) {
        if((sides[Black] & pieces[Rook] & Rank8) == 0) {
            return false;
        }
        if(testBit(castlingRooks, getSquare(getMsb(sides[Black] & pieces[Rook] & Rank8)))) {
	        clearBit(castlingRooks, getSquare(getMsb(sides[Black] & pieces[Rook] & Rank8))); 
            return true;
        }    
    } else {
        if((sides[Black] & pieces[Rook] & Rank8) == 0) {
            return false;
        }
        if(testBit(castlingRooks, getSquare(getLsb(sides[Black] & pieces[Rook] & Rank8)))) {
            clearBit(castlingRooks, getSquare(getLsb(sides[Black] & pieces[Rook] & Rank8)));
            return true;
        }    
    }
    return false;
}

std::string Board::getCastlingRights() const {
    std::string output;
    Bitboard castlingRooks = sides[White] & pieces[Rook];
    while(castlingRooks != 0) {
        Square square = getSquare(popMsb(castlingRooks));
        if(testBit(FileH, square)) {
            output += "K";
        } else if(testBit(FileA, square)) {
            output += "Q";
        }
    }
    castlingRooks = sides[Black] & pieces[Rook];
    while(castlingRooks != 0) {
        Square square = getSquare(popMsb(castlingRooks));
        if(testBit(FileA, square)) {
            output += "k";
        } else if(testBit(FileA, square)) {
            output += "q";
        }
    }
    return output.empty() ? "-" : output;
}

void Board::setEnpassantSquare(Square square) {
    enpassantSquare = square;
}

Square Board::getEnpassantSquare() {
    return enpassantSquare;
}

void Board::setSquare(Color side, Piece piece, Square square) {
    assert(square != None);

    squares[square] = makePiece(piece, side);
    setBit(sides[side], square);
    setBit(pieces[piece], square);
}

void Board::clearSquare(Square square) {
    assert(square != None);
    ColorPiece pieceOn = squares[square];
    if(pieceOn == Empty) {
        return;
    }
    squares[square] = Empty;
    clearBit(sides[getColorOfPiece(pieceOn)], square);
    clearBit(pieces[getPieceType(pieceOn)], square);
    if(testBit(castlingRooks, square)) {
        clearBit(castlingRooks, square);
    }
}

Board::BoardLegality Board::getBoardLegalityState() const {
    if(popCnt(pieces[King]) != 2 || popCnt(pieces[King] & sides[White]) != 1 || popCnt(pieces[King] & sides[Black]) != 1) {
        return IllegalKings;
    }
    Bitboard kingAttacks = getAllSquareAttackers(sides[White] | sides[Black], getSquare(getLsb(pieces[King] & sides[flipColor(turn)]))) & sides[turn];
    if(kingAttacks != 0) {
        return IllegalKingPosition;
    }
    if((pieces[Pawn] & Rank1) != 0 || (pieces[Pawn] & Rank8) != 0) {
        return IllegalPawns;
    }
    if(enpassantSquare != None && (getRelativeRankIndexOfSquare(turn, enpassantSquare) != Five || (squares[getSquare(enpassantSquare - 8 + (turn << 4))] != WhitePawn && squares[getSquare(enpassantSquare - 8 + (turn << 4))] != BlackPawn))) {
        return IllegalEnpassant;
    }
    return Legal;
}

Board Board::createBoardFromFEN(std::string fen) {
    Board board;
    int square = a8;
    
    std::string token = fen.substr(0, fen.find(" "));
    for(char& c : token) {
        if(std::isdigit(c)) {
            //in FEN, a digit in this place corresponds to skipping that many positions
	        square += c - '0';
	    } else if(c == '/') {
            //go down a rank
	        square -= 2 * NumRanks;
	    } else {
            //place a piece
            Color color = std::islower(c) ? Black : White;
	        Piece piece;
            switch(std::toupper(c)) {
	            case 'P':
		            piece = Pawn;
		            break;
		        case 'N':
		            piece = Knight;
		            break;
		        case 'B':
		            piece = Bishop;
		            break;
		        case 'R':
		            piece = Rook;
		            break;
		        case 'Q':
		            piece = Queen; 
		            break;
		        case 'K':
		            piece = King;
		            break;
		        default:
		            std::cerr << "hey you suck this isn't a chess piece give me a real chess piece" << std::endl;
		            return board;    
	        }
            board.setSquare(color, piece, getSquare(square));
	        //this won't overflow under the assumption the FEN is well-formed (since a / should follow)
            square++;   
	    }
    }
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.turn = token[0] == 'w' ? White : Black;
    
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    //castling rights
    for(char& c : token) {
        if(c == 'K') {
	        setBit(board.castlingRooks, getSquare(getMsb(board.sides[White] & board.pieces[Rook] & Rank1))); 
	    } else if(c == 'Q') {
	        setBit(board.castlingRooks, getSquare(getLsb(board.sides[White] & board.pieces[Rook] & Rank1)));
	    } else if(c == 'k') {
	        setBit(board.castlingRooks, getSquare(getMsb(board.sides[Black] & board.pieces[Rook] & Rank8)));
	    } else if(c == 'q') {
	        setBit(board.castlingRooks, getSquare(getLsb(board.sides[Black] & board.pieces[Rook] & Rank8)));
	    }
    }


    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.enpassantSquare = squareFromString(token);

    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.plies = std::stoi(token);
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.fullmoves = 0;

    return board;
}

void Board::validateLegality() {
    assert(getBoardLegalityState() == Legal);
    //Create a bit mask of where the kings and rooks are
    for(int sq = 0; sq < NumSquares; sq++) {
	    Square s = getSquare(sq);
        castleMasks[s] = ~0ull;
	    if(testBit(castlingRooks, s)) {
	        clearBit(castleMasks[sq], s);
	    } else if(testBit(sides[White] & pieces[King], s)) {
	        castleMasks[sq] &= ~sides[White];
	    } else if(testBit(sides[Black] & pieces[King], s)) {
	        castleMasks[sq] &= ~sides[Black];
	    }
    }
    kingAttackers = getAllKingAttackers();
    initMaterialEval();
}

std::string Board::getFEN() const {
    return "not implemented yet";
}

ColorPiece Board::getPieceAt(Square square) const {
    assert(square != None);
    return squares[square];
}

Square Board::getKing() const {
    if((pieces[King] & sides[turn]) == 0) {
        return None;
    }
    return getSquare(getLsb(pieces[King] & sides[turn]));
}
Color Board::getTurn() const {
    return turn;
}

void Board::setTurn(Color turn) {
    this->turn = turn;
}

void Board::perftTest(int depth) {
    perftRootDepth = depth;
    std::map<std::string, int> divideTree;
    int promotions = 0;
    int castles = 0;
    int enpassant = 0;

    auto start = std::chrono::system_clock::now();
    unsigned long long nodes = perft(divideTree, depth, enpassant, promotions, castles);
    auto end = std::chrono::system_clock::now();

    std::cout << " ◌ Perft test generated " << nodes << " in "  << std::chrono::duration_cast<std::chrono::milliseconds>((end - start)).count() << " milliseconds." << std::endl;
    std::cout << " ◌ Promotion moves in leaf nodes: " << promotions << std::endl;
    std::cout << " ◌ Castling moves in leaf nodes: " << castles << std::endl;
    std::cout << " ◌ Enpassant moves in leaf nodes: " << enpassant << std::endl;
    std::cout << " ◌ Divide tree:" << std::endl;
    for(auto const& x : divideTree) {
        std::cout << " ◌ " << x.first << ":" << x.second << std::endl;
    }
 }

unsigned long long Board::perft(std::map<std::string, int>& divideTree, int depth, int& enpassant, int& promotions, int& castles) {
    if(depth == 0) {
        return 1;
    }
    unsigned long long numMoves = 0;
    
    undoStack.emplace_back();
    std::vector<Move> moveList;
    moveList.reserve(MaxNumMoves);

    generateAllNoisyMoves(moveList);
    generateAllQuietMoves(moveList);

    for(Move& move : moveList) {
        applyMoveWithUndo(move, undoStack.back());
        if(!didLastMoveLeaveInCheck()) {
            if(depth == 1) {
                if(move.getMoveType() == Move::MoveType::Enpassant) {
                    enpassant++;
                } else if(move.getMoveType() == Move::MoveType::Promotion) {
                    promotions++;
                } else if(move.getMoveType() == Move::MoveType::Castle) {
                    castles++;
                }
            }
            int subNodes = perft(divideTree, depth - 1, enpassant, promotions, castles);
            if(depth == perftRootDepth) {
                divideTree[move.toString()] += subNodes;
            }
            numMoves += subNodes;
        }
        revertMove(undoStack.back());
    }
    undoStack.pop_back();
    return numMoves;
}

Bitboard Board::PrecomputedBinary::calculateRookBishopAttacks(Square square, Bitboard occupiedBoard, const MultiArray<int, 4, 2>& movementDelta) {
    Bitboard result = 0;
    for(int i = 0; i < 4; i++) {
        int rankChange = movementDelta[i][0];
	    int fileChange = movementDelta[i][1];

	    for(int rankIndex = getRankIndexOfSquare(square) + rankChange, fileIndex = getFileIndexOfSquare(square) + fileChange; 0 <= rankIndex && rankIndex < NumRanks && 0 <= fileIndex && fileIndex < NumFiles; rankIndex += rankChange, fileIndex += fileChange) {
	        //in the direction we move, add a bit to the bitboard
	        setBit(result, getSquare(rankIndex, fileIndex));
	        //stop if we hit something that blocks us
	        if(testBit(occupiedBoard, getSquare(rankIndex, fileIndex))) {
	            break;
	        }
	    }
    }
    return result;
}

void Board::PrecomputedBinary::populateHashTable(HashEntry* table, Square square, Bitboard hash, const MultiArray<int, 4, 2>& movementDelta) {
    Bitboard occupiedBoard = 0;

    table[square].hash = hash;
    //Subtract the edges of the board that we aren't on, since if we hit the edge we can't be blocked by anything (there is nowhere left to go)
    table[square].mask = calculateRookBishopAttacks(square, 0, movementDelta) & ~(((FileA | FileH) & ~getFile(getFileIndexOfSquare(square))) | ((Rank1 | Rank8) & ~getRank(getRankIndexOfSquare(square))));
    table[square].shift = 64 - popCnt(table[square].mask);

    if(square != h8) {
        table[square + 1].offset = table[square].offset + (1 << popCnt(table[square].mask));
    }
    
    //Initialize the attacks at every square
    table[square].offset[computeHashTableIndex(occupiedBoard, table[square])] = calculateRookBishopAttacks(square, occupiedBoard, movementDelta);
    occupiedBoard = (occupiedBoard - table[square].mask) & table[square].mask;
    while(occupiedBoard != 0) {
        table[square].offset[computeHashTableIndex(occupiedBoard, table[square])] = calculateRookBishopAttacks(square, occupiedBoard, movementDelta);
        occupiedBoard = (occupiedBoard - table[square].mask) & table[square].mask;
    }
}

void Board::evalAddPiece(ColorPiece piece, Square location) {
    currentEval += psqt.at(piece)[location];
}

void Board::evalRemovePiece(ColorPiece piece, Square location) {
    currentEval -= psqt.at(piece)[location];
}

void Board::initMaterialEval() {
    currentEval = 0;
    for (int i = 0; i < NumSquares; ++i) {
        evalAddPiece(squares[getSquare(i)], getSquare(i));
    }
}


bool Board::applyMove(Move& move) {
    if(move.isMoveNone()) {
        return false;
    }
    undoStack.emplace_back();
    applyMoveWithUndo(move, undoStack.back());
    if(didLastMoveLeaveInCheck()) {
        revertMove(undoStack.back());
        undoStack.pop_back();
        return false;
    }
    return true;
}

bool Board::didLastMoveLeaveInCheck() {
    Square kingSquare = getSquare(getLsb(sides[flipColor(turn)] & pieces[King]));
    return isSquareAttacked(kingSquare, flipColor(turn));
}

bool Board::isSideInCheck(Color side) {
   return isSquareAttacked(getSquare(getLsb(pieces[King] & sides[side])), side);   
}

void Board::applyLegalMove(Move& move) {
    undoStack.emplace_back();
    applyMoveWithUndo(move, undoStack.back());
    assert(!didLastMoveLeaveInCheck());
}

void Board::applyMoveWithUndo(Move& move, UndoData& undo) {
    undo.positionHash = positionHash;
    undo.kingAttackers = kingAttackers;
    undo.castlingRooks = castlingRooks;
    undo.enpassantSquare = enpassantSquare;
    undo.plies = plies;
    undo.move = move;
    undo.currentEval = currentEval;

    fullmoves++;

    switch(move.getMoveType()) {
	    case Move::MoveType::Normal:
	        applyNormalMoveWithUndo(move, undo);
	        break;
	    case Move::MoveType::Castle:
	        applyCastlingMoveWithUndo(move, undo);
	        break;
	    case Move::MoveType::Enpassant:
	        applyEnpassantMoveWithUndo(move, undo);
	        break;
	    case Move::MoveType::Promotion:
	        applyPromotionMoveWithUndo(move, undo);  
            break;  
    }

    //if the enpassant square was not updated (i.e. no 2 pawn forward move was played),
    //then enpassant expires and we must remove it
    if(enpassantSquare == undo.enpassantSquare) {
        if (enpassantSquare != None) {
            ZobristNums::changeEnPassant(positionHash, getFileIndexOfSquare(enpassantSquare));
        } 
        enpassantSquare = None;
    }

    // if castling permissions are different, reflect this in zobrist
    Bitboard rookChanges = castlingRooks & (~undo.castlingRooks);
    if ((rookChanges & sides[White] & FileA) != 0) {
        ZobristNums::changeCastleRights(positionHash, White, true);
    }
    if ((rookChanges & sides[Black] & FileA) != 0) {
        ZobristNums::changeCastleRights(positionHash, White, false);
    }
    if ((rookChanges & sides[White] & FileH) != 0) {
        ZobristNums::changeCastleRights(positionHash, Black, true);
    }
    if ((rookChanges & sides[Black] & FileH) != 0) {
        ZobristNums::changeCastleRights(positionHash, Black, false);
    }

    //flip whose turn it is
    turn = flipColor(turn);
    ZobristNums::flipColor(positionHash);
    kingAttackers = getAllKingAttackers();
}

void Board::applyNormalMoveWithUndo(Move& move, UndoData& undo) {
    ColorPiece from = squares[move.getFrom()];
    ColorPiece to = squares[move.getTo()];
  
    //If we capture a piece OR move a pawn, reset the fifty move rule
    if(getPieceType(from) == Pawn || to != Empty) {
        plies = 0;
    } else {
        plies++;
    }
    pieces[getPieceType(from)] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    //zobrist hash update
    ZobristNums::changePiece(positionHash, getColorOfPiece(from), getPieceType(from), move.getFrom());
    ZobristNums::changePiece(positionHash, getColorOfPiece(from), getPieceType(from), move.getTo());

    // material eval update
    evalAddPiece(from, move.getTo());
    evalRemovePiece(from, move.getFrom());

    //if we captured
    if(to != Empty) {
        pieces[getPieceType(to)] ^= (1ull << move.getTo());
        sides[flipColor(turn)] ^= (1ull << move.getTo());
        ZobristNums::changePiece(positionHash, getColorOfPiece(to), getPieceType(to), move.getTo());
        evalRemovePiece(to, move.getTo());
    }

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = from;

    castlingRooks &= castleMasks[move.getFrom()];
    castlingRooks &= castleMasks[move.getTo()];
    undo.pieceCaptured = to;

    //if we move 2 forward, set enpassant data
    if(getPieceType(from) == Pawn && (move.getTo() ^ move.getFrom()) == 16
    && 0 != (pieces[Pawn] & sides[flipColor(turn)] & PrecomputedBinary::getBinary().getAdjacentFilesMask(getFileIndexOfSquare(move.getFrom())) & ((turn == White) ? Rank4 : Rank5))) {
        enpassantSquare = getSquare((turn == White) ? move.getFrom() + 8 : move.getFrom() - 8);
        ZobristNums::changeEnPassant(positionHash, getFileIndexOfSquare(enpassantSquare));
    }
}

Square Board::getKingCastlingSquare(Square king, Square rook) {
    return getSquare(getRankIndexOfSquare(king), static_cast<Index>(rook > king ? 6 : 2)); //return the castling square on the king's rank (which corresponds to the file)
}

Square Board::getRookCastlingSquare(Square king, Square rook) {
    return getSquare(getRankIndexOfSquare(king), static_cast<Index>(rook > king ? 5 : 3));
}

void Board::applyCastlingMoveWithUndo(Move& move, Board::UndoData& undo) {
    Square kingFrom = move.getFrom();
    Square rookFrom = move.getTo();
    assert(getPieceType(squares[kingFrom]) == King);

    Square kingTo = getKingCastlingSquare(kingFrom, rookFrom);
    Square rookTo = getRookCastlingSquare(kingFrom, rookFrom);

    //zobrist hash update
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), King, kingTo);
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), King, kingFrom);
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), Rook, rookTo);
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), Rook, rookFrom);

    //material eval update
    evalAddPiece(squares[kingFrom], kingTo);
    evalRemovePiece(squares[kingFrom], kingFrom);
    evalAddPiece(squares[rookFrom], rookTo);
    evalRemovePiece(squares[rookFrom], rookFrom);
    
    pieces[King] ^= (1ull << kingFrom) ^ (1ull << kingTo);
    sides[turn] ^= (1ull << kingFrom) ^ (1ull << kingTo);

    pieces[Rook] ^= (1ull << rookFrom) ^ (1ull << rookTo);
    sides[turn] ^= (1ull << rookFrom) ^ (1ull << rookTo);

    squares[kingFrom] = Empty;
    squares[rookFrom] = Empty;

    squares[kingTo] = makePiece(King, turn);
    squares[rookTo] = makePiece(Rook, turn);
    
    castlingRooks &= castleMasks[kingFrom]; 

    undo.pieceCaptured = Empty;

    plies++;
}

void Board::applyEnpassantMoveWithUndo(Move& move, Board::UndoData& undo) { 
    Square capturedSquare = getSquare(move.getTo() - 8 + (turn << 4));

    //zobrist hash update
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), Pawn, move.getTo());
    ZobristNums::changePiece(positionHash, getColorOfPiece(squares[move.getFrom()]), Pawn, move.getFrom());
    ZobristNums::changePiece(positionHash, flipColor(getColorOfPiece(squares[move.getFrom()])), Pawn, capturedSquare);

    //materialEval update
    evalAddPiece(squares[move.getFrom()], move.getTo());
    evalRemovePiece(squares[move.getFrom()], move.getFrom());
    evalRemovePiece(squares[capturedSquare], capturedSquare);
    
    //en passant is a capture, so reset the fifty move rule
    plies = 0;
    pieces[Pawn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    pieces[Pawn] ^= (1ull << capturedSquare);
    sides[flipColor(turn)] ^= (1ull << capturedSquare);

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = makePiece(Pawn, turn);
    squares[capturedSquare] = Empty;
    undo.pieceCaptured = makePiece(Pawn, flipColor(turn));
}

void Board::applyPromotionMoveWithUndo(Move& move, Board::UndoData& undo) {
    ColorPiece promotedPiece = makePiece(move.getPromoType(), turn);
    ColorPiece capturedPiece = squares[move.getTo()];

    //zobrist hash update
    ZobristNums::changePiece(positionHash, getColorOfPiece(promotedPiece), Pawn, move.getFrom());
    ZobristNums::changePiece(positionHash, getColorOfPiece(promotedPiece), move.getPromoType(), move.getTo());

    //material eval
    evalAddPiece(promotedPiece, move.getTo());
    evalRemovePiece(squares[move.getFrom()], move.getFrom());

    //promotion resets the fifty move rule
    plies = 0;
    pieces[Pawn] ^= (1ull << move.getFrom());
    pieces[move.getPromoType()] ^= (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    if(capturedPiece != Empty) {
        ZobristNums::changePiece(positionHash, getColorOfPiece(capturedPiece), getPieceType(capturedPiece), move.getTo());
        evalRemovePiece(squares[move.getTo()], move.getTo());
        pieces[getPieceType(capturedPiece)] ^= (1ull << move.getTo());
        sides[getColorOfPiece(capturedPiece)] ^= (1ull << move.getTo());
    }

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = promotedPiece;
    undo.pieceCaptured = capturedPiece;

    castlingRooks &= castleMasks[move.getTo()];
}

int Board::countLegalMoves() {
    std::vector<Move> moveList;
    moveList.reserve(MaxNumMoves);
    return generateAllLegalMoves(moveList);
}

void Board::revertMostRecent() {
    revertMove(undoStack.back());
    undoStack.pop_back();
}

void Board::revertMove(UndoData& undo) {
    positionHash = undo.positionHash;
    kingAttackers = undo.kingAttackers;
    enpassantSquare = undo.enpassantSquare;
    plies = undo.plies;
    castlingRooks = undo.castlingRooks;
    Move& move = undo.move;
    currentEval = undo.currentEval;

    turn = flipColor(turn);
    fullmoves--;

    switch(move.getMoveType()) {
        case Move::MoveType::Normal: {
            Piece fromType = getPieceType(squares[move.getTo()]);

            pieces[fromType] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            if(undo.pieceCaptured != Empty) {
                pieces[getPieceType(undo.pieceCaptured)] ^= (1ull << move.getTo());
                sides[getColorOfPiece(undo.pieceCaptured)] ^= (1ull << move.getTo());
            }

            squares[move.getFrom()] = squares[move.getTo()];
            squares[move.getTo()] = undo.pieceCaptured;
            break;
        }
        case Move::MoveType::Castle: {
            Square rookFrom = move.getTo();
            Square rookTo = getRookCastlingSquare(move.getFrom(), rookFrom);
            Square kingTo = getKingCastlingSquare(move.getFrom(), rookFrom);

            pieces[King] ^= (1ull << move.getFrom()) ^ (1ull << kingTo);
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << kingTo);

            pieces[Rook] ^= (1ull << rookFrom) ^ (1ull << rookTo);
            sides[turn] ^= (1ull << rookFrom) ^ (1ull << rookTo);

            squares[kingTo] = Empty;
            squares[rookTo] = Empty;

            squares[move.getFrom()] = makePiece(King, turn);
            squares[rookFrom] = makePiece(Rook, turn);
            break;
        }
        case Move::MoveType::Promotion: {
            pieces[Pawn] ^= (1ull << move.getFrom());
            pieces[move.getPromoType()] ^= (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            if(undo.pieceCaptured != Empty) {
                pieces[getPieceType(undo.pieceCaptured)] ^= (1ull << move.getTo());
                sides[getColorOfPiece(undo.pieceCaptured)] ^= (1ull << move.getTo());
            }

            squares[move.getFrom()] = makePiece(Pawn, turn);
            squares[move.getTo()] = undo.pieceCaptured;
            break;
        }
        case Move::MoveType::Enpassant: {
            int turnBit = turn == White ? 0 : 1;
            Square enpassantCaptureSquare = getSquare(move.getTo() - 8 + (turnBit << 4));

            pieces[Pawn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            pieces[Pawn] ^= (1ull << enpassantCaptureSquare);
            sides[flipColor(turn)] ^= (1ull << enpassantCaptureSquare);

            squares[move.getFrom()] = squares[move.getTo()];
            squares[move.getTo()] = Empty;
            squares[enpassantCaptureSquare] = undo.pieceCaptured;
            break;
        }
        default:
            std::cerr << "AHHHHHHH" << std::endl;    
    }
}

bool Board::isMovePseudoLegal(Move& move) {
    if(move.isMoveNone() || squares[move.getFrom()] == Empty) {
        return false;
    }
    Piece fromType = getPieceType(squares[move.getFrom()]);

    //The following are illegal moves that are theoretically valid as moves in our encoding
    if(getColorOfPiece(squares[move.getFrom()]) != turn || (move.getPromoType() != Knight && !move.isMovePromotion()) || (move.getMoveType() == Move::MoveType::Castle && fromType != King)) {
        return false;
    }

    Bitboard occupiedBoard = sides[White] | sides[Black];

    //Nonspecial moves from nonpawns are pseudo legal as long as they are normal and previously were attacking the squares


    if(fromType == Knight) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getKnightAttacksFromSquare(move.getFrom()) & ~sides[turn], move.getTo());
    }
    if(fromType == Bishop) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getBishopAttacksFromSquare(move.getFrom(), occupiedBoard) & ~sides[turn], move.getTo());
    }
    if(fromType == Rook) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getRookAttacksFromSquare(move.getFrom(), occupiedBoard) & ~sides[turn], move.getTo());
    }
    if(fromType == Queen) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getQueenAttacksFromSquare(move.getFrom(), occupiedBoard) & ~sides[turn], move.getTo());
    }
    if(fromType == King && move.getMoveType() == Move::MoveType::Normal) {
        return testBit(PrecomputedBinary::getBinary().getKingAttacksFromSquare(move.getFrom()) & ~sides[turn], move.getTo());
    }
    if(fromType == Pawn) {
        if(move.getMoveType() == Move::MoveType::Enpassant) {
            return move.getTo() == enpassantSquare && testBit(PrecomputedBinary::getBinary().getPawnAttacksFromSquare(move.getFrom(), turn), move.getTo());
        }

        Bitboard pawnAdvance = getPawnAdvances(1ull << move.getFrom(), occupiedBoard, turn);

        if(move.getMoveType() == Move::MoveType::Promotion) {
            return testBit(LastRanks & ((PrecomputedBinary::getBinary().getPawnAttacksFromSquare(move.getFrom(), turn) & sides[flipColor(turn)]) | pawnAdvance), move.getTo());
        }
        //Alright, not enpassant or promotion, must be normal.
        //This includes 2 forward at start, so add those by including the 3rd rank as a starting point for a forward pawn move 
        pawnAdvance |= getPawnAdvances(pawnAdvance & (turn == White ? Rank3 : Rank6), occupiedBoard, turn);

        return testBit(~LastRanks & ((PrecomputedBinary::getBinary().getPawnAttacksFromSquare(move.getFrom(), turn) & sides[flipColor(turn)]) | pawnAdvance), move.getTo());
    }
    //must be castling (we ruled out king normals above)
    if(move.getMoveType() != Move::MoveType::Castle) {
        return false;
    }
    //To castle, we must not be in check
    if(kingAttackers != 0) {
        return false;
    }
    //iterate through all the possible rooks on the board to see if they were the one that was castled with
    Bitboard rookCopy = sides[turn] & castlingRooks;
    while(rookCopy != 0) {
        Square rookFrom = getSquare(popLsb(rookCopy));
        //Castling is encoded as the rook's square being the king's destination
        if(rookFrom != move.getTo()) {
            continue;
        }
        //Alright, we know this rook matches what we have.
        Square rookTo = getRookCastlingSquare(move.getFrom(), rookFrom);
        Square kingTo = getKingCastlingSquare(move.getFrom(), rookFrom);
        
        //Check if we went through stuff
        Bitboard between = PrecomputedBinary::getBinary().getBetweenSquaresMask(move.getFrom(), kingTo) | (1ull << kingTo)
                            | PrecomputedBinary::getBinary().getBetweenSquaresMask(rookFrom, rookTo) | (1ull << rookTo);
        between &= ~((1ull << move.getFrom()) | (1ull << rookFrom));
        if((occupiedBoard & between) != 0) {
            continue; //we went through stuff
        }
        
        //if we go through check
        if(isSquareInBoardAttacked(PrecomputedBinary::getBinary().getBetweenSquaresMask(move.getFrom(), kingTo), turn)) {
            continue;
        }
        //we did a totally legal castling move
        return true;
    }
    return false;
}

bool Board::isMoveLegal(Move& move) {
    Square kingSquare = getSquare(getLsb(pieces[King] & sides[flipColor(turn)]));
    return isMovePseudoLegal(move) && !isSquareAttacked(kingSquare, flipColor(turn));
}
void Board::addEnpassantMoves(std::vector<Move>& moveList, Bitboard sources, Square enpassantSquare) {
    while(sources != 0) {
        moveList.emplace_back(getSquare(popLsb(sources)), enpassantSquare, Move::MoveType::Enpassant);
    }
}

void Board::addPawnMoves(std::vector<Move>& moveList, Bitboard targets, int directionPawnIsIn) {
    while(targets != 0) {
        Square square = getSquare(popLsb(targets));
        moveList.emplace_back(getSquare(square + directionPawnIsIn), square, Move::MoveType::Normal);
    }
}

void Board::addPawnPromotions(std::vector<Move>& moveList, Bitboard targets, int directionPawnIsIn) {
    while(targets != 0) {
        Square square = getSquare(popLsb(targets));
        //bias our move ordering QNRB since that's the most likely order of promotions (a moral victory, there is a 0% chance this has impact on playing strength)
        moveList.emplace_back(getSquare(square + directionPawnIsIn), square, Move::MoveType::Promotion, Queen);
        moveList.emplace_back(getSquare(square + directionPawnIsIn), square, Move::MoveType::Promotion, Knight);
        moveList.emplace_back(getSquare(square + directionPawnIsIn), square, Move::MoveType::Promotion, Rook);
        moveList.emplace_back(getSquare(square + directionPawnIsIn), square, Move::MoveType::Promotion, Bishop);
    }
}

void Board::addNormalMoves(std::vector<Move>& moveList, Bitboard targets, Square from) {
    while(targets != 0) {
        moveList.emplace_back(from, getSquare(popLsb(targets)), Move::MoveType::Normal);
    }
}

void Board::addNonPawnNormalMoves(std::vector<Move>& moveList, Piece type, Bitboard targets, Bitboard sources, Bitboard occupiedBoard) {
    assert(type != Pawn && type != Queen); //queen should be given separately as rook and bishop
    //Have the switch on the outside of the loops so we don't have to rely on branch prediction not being dumb
    switch(type) {
        case King: {
            while(sources != 0) {
                Square fromSquare = getSquare(popLsb(sources));
                addNormalMoves(moveList, PrecomputedBinary::getBinary().getKingAttacksFromSquare(fromSquare) & targets, fromSquare);
            }
            break;
        }
        case Knight: {
            while(sources != 0) {
                Square fromSquare = getSquare(popLsb(sources));
                addNormalMoves(moveList, PrecomputedBinary::getBinary().getKnightAttacksFromSquare(fromSquare) & targets, fromSquare);
            }
            break;
        }
        case Bishop: {
            while(sources != 0) {
                Square fromSquare = getSquare(popLsb(sources));
                addNormalMoves(moveList, PrecomputedBinary::getBinary().getBishopAttacksFromSquare(fromSquare, occupiedBoard) & targets, fromSquare);
            }
            break;
        }
        case Rook: {
            while(sources != 0) {
                Square fromSquare = getSquare(popLsb(sources));
                addNormalMoves(moveList, PrecomputedBinary::getBinary().getRookAttacksFromSquare(fromSquare, occupiedBoard) & targets, fromSquare);
            }
            break;
        }
        default:
            assert(false);
    }
}

bool Board::isSquareInBoardAttacked(Bitboard board, Color turn) {
    while(board != 0) {
        if(isSquareAttacked(getSquare(popLsb(board)), turn)) {
            return true;
        }
    }
    return false;
}

int Board::generateAllNoisyMoves(std::vector<Move>& moveList) {
    const int startSize = moveList.size();
    Bitboard occupiedBoard = sides[White] | sides[Black];

    Bitboard opponents = sides[flipColor(turn)];

    //if the king is in check, the only noisy way to get out of it is to capture a checker
    if(kingAttackers != 0) {
        opponents &= kingAttackers;
    }

    //If we are in check by >1 piece, the only legal way to get out of it is to move the king
    if(isNonSingular(kingAttackers)) {
        addNonPawnNormalMoves(moveList, King, opponents, sides[turn] & pieces[King], occupiedBoard);
        return moveList.size() - startSize;
    }

    Bitboard enpassantSources = getPawnEnpassantCaptures(sides[turn] & pieces[Pawn], enpassantSquare, turn);
    addEnpassantMoves(moveList, enpassantSources, enpassantSquare);

    const int sideDirection = turn == White ? -1 : 1;
    const int left = sideDirection * (NumFiles - 1);
    const int right = sideDirection * (NumFiles + 1);
    const int forward = sideDirection * NumFiles;

    Bitboard leftPawnAttacks = getPawnLeftAttacks(sides[turn] & pieces[Pawn], sides[flipColor(turn)], turn);
    Bitboard rightPawnAttacks = getPawnRightAttacks(sides[turn] & pieces[Pawn], sides[flipColor(turn)], turn);
    //Promoting forward is a pseudo-legal noisy move that can block the check if there is one
    //So leave it to full legality to verify if it does that
    Bitboard promoteForward = getPawnAdvances(sides[turn] & pieces[Pawn], occupiedBoard, turn) & LastRanks;
    addPawnPromotions(moveList, promoteForward, forward);

    Bitboard promoteLeftCapture = leftPawnAttacks & LastRanks;
    leftPawnAttacks &= ~LastRanks; //don't double count
    Bitboard promoteRightCapture = rightPawnAttacks & LastRanks;
    rightPawnAttacks &= ~LastRanks; //don't double count
    addPawnMoves(moveList, leftPawnAttacks & opponents, left);
    addPawnMoves(moveList, rightPawnAttacks & opponents, right);
    addPawnPromotions(moveList, promoteLeftCapture, left);
    addPawnPromotions(moveList, promoteRightCapture, right);
    addNonPawnNormalMoves(moveList, Knight, opponents, sides[turn] & pieces[Knight], occupiedBoard);
    addNonPawnNormalMoves(moveList, Bishop, opponents, sides[turn] & (pieces[Bishop] | pieces[Queen]), occupiedBoard);
    addNonPawnNormalMoves(moveList, Rook, opponents, sides[turn] & (pieces[Rook] | pieces[Queen]), occupiedBoard);
    //Note: This is not opponents in the third argument, since a valid noisy move to get out of check
    //is for the king to capture a non-checking piece. This was a very annoying bug to find.
    addNonPawnNormalMoves(moveList, King, sides[flipColor(turn)], sides[turn] & pieces[King], occupiedBoard);

    return moveList.size() - startSize;
}

int Board::generateAllNoisyMovesAndChecks(std::vector<Move>& moveList) {
    const int startSize = moveList.size();
    std::vector<Move> checks;
    generateAllNoisyMoves(moveList);
    generateAllQuietMoves(checks);
    undoStack.emplace_back();
    for(Move& move : checks) {
        applyMoveWithUndo(move, undoStack.back());
        if(kingAttackers != 0) {
            moveList.emplace_back(move);
        }
        revertMove(undoStack.back());
    }
    undoStack.pop_back();
    return moveList.size() - startSize;
}

int Board::generateAllQuietMoves(std::vector<Move>& moveList) {
    const int startSize = moveList.size();

    Bitboard occupiedBoard = sides[White] | sides[Black];
    //move the king if >1 checker, since that's the only legal way to get out of check
    if(isNonSingular(kingAttackers)) {
        addNonPawnNormalMoves(moveList, King, ~occupiedBoard, sides[turn] & pieces[King], occupiedBoard);
        return moveList.size() - startSize;
    }
    //All pseudo-legal quiet king moves that aren't castling are just moving the king
    addNonPawnNormalMoves(moveList, King, ~occupiedBoard, sides[turn] & pieces[King], occupiedBoard);
    //Castling (similar logic to isMovePseudoLegal)
    Bitboard rookCopy = castlingRooks & sides[turn];
    if(kingAttackers == 0) { //if we aren't in check
        while(rookCopy != 0) {
            Square rookFrom = getSquare(popLsb(rookCopy));
            Square kingFrom = getSquare(getLsb(sides[turn] & pieces[King]));

            Square rookTo = getRookCastlingSquare(kingFrom, rookFrom);
            Square kingTo = getKingCastlingSquare(kingFrom, rookFrom);
            
            Bitboard betweenSquares = PrecomputedBinary::getBinary().getBetweenSquaresMask(kingFrom, kingTo) | PrecomputedBinary::getBinary().getBetweenSquaresMask(rookFrom, rookTo) | (1ull << kingTo) | (1ull << rookTo);
            //don't count the squares they are on
            betweenSquares &= ~((1ull << rookFrom) | (1ull << kingFrom));
            if((occupiedBoard & betweenSquares) != 0) {
                //we cannot castle here, as we pass through things
                continue;
            }
            if(isSquareInBoardAttacked(PrecomputedBinary::getBinary().getBetweenSquaresMask(kingFrom, kingTo), turn)) {
                continue; //we went through check
            }
            moveList.emplace_back(kingFrom, rookFrom, Move::MoveType::Castle);
        }
    }

    //If we are not in check, all the noncaptures (the remaining quiet moves) are just spots
    //on ~occupiedBoard. If we are in check, the only quiet way to get out is to block the piece
    Bitboard targetSquares;
    if(kingAttackers != 0) {
        targetSquares = PrecomputedBinary::getBinary().getBetweenSquaresMask(getSquare(getLsb(pieces[King] & sides[turn])), getSquare(getLsb(kingAttackers)));
    } else {
        targetSquares = ~occupiedBoard;
    }

    //pawn moves forward 1 and 2 that don't promote
    Bitboard pawnsForwardOne = ~LastRanks & getPawnAdvances(pieces[Pawn] & sides[turn], occupiedBoard, turn);
    addPawnMoves(moveList, pawnsForwardOne & targetSquares, turn == White ? -NumFiles : NumFiles);

    Bitboard pawnsForwardTwo = getPawnAdvances(pawnsForwardOne & (turn == White ? Rank3 : Rank6), occupiedBoard, turn);
    addPawnMoves(moveList, pawnsForwardTwo & targetSquares, turn == White ? -(2 * NumFiles) : (2 * NumFiles));

    addNonPawnNormalMoves(moveList, Knight, targetSquares, sides[turn] & pieces[Knight], occupiedBoard);
    addNonPawnNormalMoves(moveList, Bishop, targetSquares, sides[turn] & (pieces[Bishop] | pieces[Queen]), occupiedBoard);
    addNonPawnNormalMoves(moveList, Rook, targetSquares, sides[turn] & (pieces[Rook] | pieces[Queen]), occupiedBoard);
    return moveList.size() - startSize;
}

int Board::generateAllPseudoLegalMoves(std::vector<Move>& moveList) {
    const int startSize = moveList.size();

    generateAllNoisyMoves(moveList);
    generateAllQuietMoves(moveList);

    return moveList.size() - startSize;
}

int Board::generateAllLegalMoves(std::vector<Move>& moveList) {
    const int startSize = moveList.size();
    std::vector<Move> pseudoLegalMoves;
    pseudoLegalMoves.reserve(MaxNumMoves);

    generateAllPseudoLegalMoves(pseudoLegalMoves);

    undoStack.emplace_back();
    for(Move& move : pseudoLegalMoves) {
        //check legality
        applyMoveWithUndo(move, undoStack.back());
        if(!didLastMoveLeaveInCheck()) {
            moveList.emplace_back(move);
        }
        revertMove(undoStack.back());
    }
    undoStack.pop_back();

    return moveList.size() - startSize;
}

Move Board::getLastPlayedMove() const {
    if(undoStack.size() == 0) {
        return Move{};
    }
    return undoStack.back().move;
}

int Board::getPlies() const {
    return plies;
}

int Board::getTotalPlies() const {
    return fullmoves;
}

Piece Board::getLastMovedPiece() const {
    if(undoStack.back().move.getMoveType() == Move::Castle) {
        return King;
    }
    if(squares[undoStack.back().move.getTo()] == Empty) {
        std::cout << "HERE" << std::endl;
    }
    return getPieceType(squares[undoStack.back().move.getTo()]);
}

bool Board::isCurrentTurnInCheck() const {
    return kingAttackers != 0;
}

bool Board::isMoveTactical(const Move& move) {
    return (move.getMoveType() == Move::Enpassant || move.getMoveType() == Move::Promotion) || (squares[move.getTo()] != Empty && move.getMoveType() != Move::Castle);
}

bool Board::currentSideAboutToPromote() const {
    return (pieces[Pawn] & sides[turn] & (turn == White ? Rank7 : Rank2)) != 0;
}

bool Board::currentSideHasPiece(Piece piece) const {
    return (pieces[Pawn] & sides[turn]) != 0;
}

uint64_t Board::getBoardHash() const {
    return positionHash;
}

int Board::getCurrentPsqt() const {
    return currentEval;
}

bool Board::isBoardMaterialDraw() const {
    //never a draw with pawns or queens
    if(pieces[Pawn] != 0 || pieces[Queen] != 0) {
        return false;
    }

    if(pieces[Rook] == 0) {
        if(pieces[Bishop] == 0) {
            //0-2 knights+king vs 0-2knights+king is draw
            return popCnt(pieces[Knight] & sides[White]) <= 2 && popCnt(pieces[Knight] & sides[Black]) <= 2;
        } else if(pieces[Knight] == 0) {
            //draw unless one side has two extra bishops
            return abs(popCnt(pieces[Bishop] & sides[White]) - popCnt(pieces[Bishop] & sides[Black])) < 2;
        } else if((popCnt(pieces[Knight] & sides[White]) <= 2 && popCnt(pieces[Bishop] & sides[Black]) == 1) || (popCnt(pieces[Knight] & sides[Black]) <= 2 && popCnt(pieces[Bishop] & sides[White]) == 1)) {
            //draw if 1-2 knights vs 1 b
            return true;
        }
    } else if(popCnt(pieces[Rook] & sides[White]) == 1 && popCnt(pieces[Rook] & sides[Black]) == 1) {
        //exactly 1 rook each, draw if versus 0-1 minors
        return popCnt((pieces[Knight] | pieces[Bishop]) & sides[White]) <= 1 && popCnt((pieces[Knight] | pieces[Bishop]) & sides[Black]);
    } else if(popCnt(pieces[Rook]) == 1) {
        //1 rook draws vs 1-2 minors
        if((pieces[Rook] & sides[White]) != 0) {
            int blackMinors = popCnt((pieces[Bishop] | pieces[Knight]) & sides[Black]);
            return popCnt((pieces[Rook] | pieces[Bishop] | pieces[Knight]) & sides[White]) == 1 && blackMinors >= 1 && blackMinors <= 2;
        } else {
            int whiteMinors = popCnt((pieces[Bishop] | pieces[Knight]) & sides[White]);
            return popCnt((pieces[Rook] | pieces[Bishop] | pieces[Knight]) & sides[Black]) == 1 && whiteMinors >= 1 && whiteMinors <= 2;
        }
    }
    return false;
}

int Board::getSidePieceCount(Color side, Piece piece) const {
    return popCnt(sides[side] & pieces[piece]);
}

bool Board::isFileOpen(Index fileIndex) const {
    return (getFile(fileIndex) & pieces[Pawn]) == 0;
}

bool Board::isFileSemiOpen(Color side, Index fileIndex) const {
    return (getFile(fileIndex) & pieces[Pawn] & sides[side]) == 0;
}

int Board::getNumberOfIsolatedPawns(Color side) const {
    Bitboard pawns = pieces[Pawn] & sides[side];
    int count = 0;
    while(pawns != 0) {
        Square square = getSquare(popLsb(pawns));
        if((PrecomputedBinary::getBinary().getAdjacentFilesMask(getFileIndexOfSquare(square)) & pieces[Pawn] & sides[side]) == 0) {
            count++;
        }
    }
    return count;
}

int Board::getNumberOfPassedPawns(Color side) const {
    Bitboard pawns = pieces[Pawn] & sides[side];
    int count = 0;
    while(pawns != 0) {
        Square square = getSquare(popLsb(pawns));
        if((PrecomputedBinary::getBinary().getPassedPawnMask(side, square) & pieces[Pawn] & sides[flipColor(side)]) == 0) {
            count++;
        }
    }
    return count;
}

int Board::getNumberOfPiecesOnOpenFile(Color side, Piece piece) const {
    Bitboard pieceBoard = pieces[piece] & sides[side];
    int count = 0;
    while(pieceBoard != 0) {
        if(isFileOpen(getFileIndexOfSquare(getSquare(popLsb(pieceBoard))))) {
            count++;
        }
    }
    return count;
}

int Board::getNumberOfPiecesOnSemiOpenFile(Color side, Piece piece) const {
    Bitboard pieceBoard = pieces[piece] & sides[side];
    int count = 0;
    while(pieceBoard != 0) {
        if(isFileSemiOpen(side, getFileIndexOfSquare(getSquare(popLsb(pieceBoard))))) {
            count++;
        }
    }
    return count;
}
