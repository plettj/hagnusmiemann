#include "board.h"
#include "move.h"
#include "zobrist.h"
#include <algorithm>
#include <map>
#include <chrono>

Board::Index Board::getFileIndexOfSquare(Board::Square square) {
    assert(square != None);
    //squares are laid out sequentially in rank, so their file is mod 8
    return static_cast<Index>(square % NumFiles);
}

Board::Index Board::getRankIndexOfSquare(Board::Square square) {
    assert(square != None);
    //ranks are groups of 8 (numFiles) in the layout, so truncate and divide
    return static_cast<Index>(square / NumFiles);
}

Board::Index Board::getMirrorFileIndex(Board::Index fileIndex) {
    //compile time optimization
    /*const static Index Mirror[] = {Zero, One, Two, Three, Three, Two, One};
    return Mirror[fileIndex];*/
    return fileIndex > 3 ? (static_cast<Index>(7 - fileIndex)) : fileIndex;
}

Board::Index Board::getRelativeRankIndexOfSquare(Board::Color side, Board::Square square) {
    assert(square != None);
    //get the rank our piece is on relative to our starting side
    return side == White ? getRankIndexOfSquare(square) : static_cast<Index>(7 - getRankIndexOfSquare(square));
}

Board::Square Board::getSquare(Board::Index rankIndex, Board::Index fileIndex) {
    return getSquare(rankIndex * NumFiles + fileIndex);
}

Board::Square Board::getRelativeSquare(Board::Color side, Board::Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side
    return getSquare(getRelativeRankIndexOfSquare(side, square), getFileIndexOfSquare(square));
}

Board::Square Board::getRelativeSquare32(Board::Color side, Board::Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side and limited to the left half of the board
    return getSquare(4 * getRelativeRankIndexOfSquare(side, square) + getMirrorFileIndex(getFileIndexOfSquare(square)));
}

Board::SquareColor Board::getSquareColor(Board::Square square) {
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

void Board::setBit(Bitboard& bb, Board::Square bit) {
    assert(!testBit(bb, bit));
    bb ^= 1ull << bit;
}

void Board::clearBit(Bitboard& bb, Board::Square bit) {
    assert(testBit(bb, bit));
    bb ^= 1ull << bit;
}

bool Board::testBit(Bitboard bb, Board::Square bit) {
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

bool Board::isDrawn(int threefoldHeight) const {
    return isFiftyMoveRuleDraw() || isThreefoldDraw(threefoldHeight) || isInsufficientMaterialDraw();
}

//TODO: CS246 might not want us to implement this
bool Board::isFiftyMoveRuleDraw() const {
    return plies > 99; 
}

bool Board::isThreefoldDraw(int threefoldHeight) const {
    //TODO: this requires Zobrist keys to be done 
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

Board::Square Board::squareFromString(const std::string& string) {
    assert(string.size() <= 2);
    if(string == "-") {
        return None;
    }
    return getSquare(string[1] - '1', string[0] - 'a');
}

Board::Board() : positionHash{0}, pawnKingHash{0}, kingAttackers{0}, castlingRooks{0}, turn{White}, plies{0}, fullmoves{0} {
    for(int i = 0; i < 8; i++) {
        pieces[i] = 0;
    }
    for(int i = 0; i < 3; i++) {
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

    hasBeenInitialized = true;
}

Bitboard Board::PrecomputedBinary::getBetweenSquaresMask(Board::Square square1, Board::Square square2) {
    assert(square1 != None && square2 != None);
    return BetweenSquaresMasks[square1][square2];
}

Bitboard Board::PrecomputedBinary::getAdjacentFilesMask(Board::Index fileIndex) {
    return AdjacentFilesMasks[fileIndex];
}

Bitboard Board::PrecomputedBinary::getKnightAttacksFromSquare(Board::Square square) {
    return KnightAttack[square];
}

Bitboard Board::PrecomputedBinary::getKingAttacksFromSquare(Board::Square square) {
    return KingAttack[square];
}

Bitboard Board::PrecomputedBinary::getPawnAttacksFromSquare(Board::Square square, Board::Color side) {
    return PawnAttack[side][square];
}

Bitboard Board::PrecomputedBinary::getBishopAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
    return BishopTable[square].offset[computeHashTableIndex(occupiedBoard, BishopTable[square])];
}

Bitboard Board::PrecomputedBinary::getRookAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
    return RookTable[square].offset[computeHashTableIndex(occupiedBoard, RookTable[square])];
}

Bitboard Board::PrecomputedBinary::getQueenAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
    return getBishopAttacksFromSquare(square, occupiedBoard) | getRookAttacksFromSquare(square, occupiedBoard);
}

Bitboard Board::getPawnLeftAttacks(Bitboard pawnBoard, Bitboard targets, Board::Color side) {
    return targets & (side == White ? (pawnBoard << 7) & ~FileH : (pawnBoard >> 7) & ~FileA);
}

Bitboard Board::getPawnRightAttacks(Bitboard pawnBoard, Bitboard targets, Board::Color side) {
    return targets & (side == White ? (pawnBoard << 9) & ~FileA : (pawnBoard >> 9) & ~FileH);
}

Bitboard Board::getPawnAdvances(Bitboard pawnBoard, Bitboard occupiedBoard, Board::Color side) {
    return ~occupiedBoard & (side == White ? pawnBoard << 8 : pawnBoard >> 8);
}

Bitboard Board::getPawnEnpassantCaptures(Bitboard pawnBoard, Board::Square enpassantSquare, Color side) {
    return (enpassantSquare == None) ? 0 : PrecomputedBinary::getBinary().getPawnAttacksFromSquare(enpassantSquare, flipColor(side)) & pawnBoard;
}

Bitboard Board::getAllSquareAttackers(Bitboard occupiedBoard, Board::Square square) {
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

bool Board::isSquareAttacked(Square square, Board::Color side) {
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

bool Board::debugIsSquareAttacked(Square square, Board::Color side) {
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



void Board::setSquare(Color side, Piece piece, Square square) {
    assert(square != None);

    squares[square] = makePiece(piece, side);
    setBit(sides[side], square);
    setBit(pieces[piece], square);

    //TODO:
    //update PSQT (probably) and hashes
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
    //TODO: update hashes throughout
    
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

    //Create a bit mask of where the kings and rooks are
    for(int sq = 0; sq < NumSquares; sq++) {
	    Square s = getSquare(sq);
        board.castleMasks[s] = ~0ull;
	    if(board.testBit(board.castlingRooks, s)) {
	        clearBit(board.castleMasks[sq], s);
	    } else if(board.testBit(board.sides[White] & board.pieces[King], s)) {
	        board.castleMasks[sq] &= ~board.sides[White];
	    } else if(board.testBit(board.sides[Black] & board.pieces[King], s)) {
	        board.castleMasks[sq] &= ~board.sides[Black];
	    }
    }

    //TODO: hash here probably (reset the rooks bits)

    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.enpassantSquare = squareFromString(token);
    if(board.enpassantSquare != None) {
        //add this to hash
    }

    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.plies = std::stoi(token);
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.fullmoves = std::stoi(token);

    board.kingAttackers = board.getAllKingAttackers();
    return board;
}

std::string Board::getFEN() const {
    return "not implemented yet";
}

Board::ColorPiece Board::getPieceAt(Square square) const {
    assert(square != None);
    return squares[square];
};
Board::Square Board::getKing() const {
    return getSquare(getLsb(pieces[King] & sides[turn]));
};
Board::Color Board::getTurn() const {
    return turn;
};

void Board::perftTest(int depth) {
    perftRootDepth = depth;
    std::map<std::string, int> divideTree;
    int promotions = 0;
    int castles = 0;
    int enpassant = 0;

    auto start = std::chrono::system_clock::now();
    unsigned long long nodes = perft(divideTree, depth, enpassant, promotions, castles);
    auto end = std::chrono::system_clock::now();

    std::cout << "Perft test generated " << nodes << " in "  << std::chrono::duration_cast<std::chrono::milliseconds>((end - start)).count() << " milliseconds." << std::endl;
    std::cout << "Promotion moves in leaf nodes:" << promotions << std::endl;
    std::cout << "Castling moves in leaf nodes:" << castles << std::endl;
    std::cout << "Enpassant moves in leaf nodes:" << enpassant << std::endl;
    std::cout << "Divide tree:" << std::endl;
    for(auto const& x : divideTree) {
        std::cout << x.first << ":" << x.second << std::endl;
    }
 }

unsigned long long Board::perft(std::map<std::string, int>& divideTree, int depth, int& enpassant, int& promotions, int& castles) {
    if(depth == 0) {
        return 1;
    }
    unsigned long long numMoves = 0;
    
    undoStack.emplace_back();
    std::vector<Move> moveList;

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
        revertMove(move, undoStack.back());
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

bool Board::applyMove(Move& move) {
    undoStack.emplace_back();
    applyMoveWithUndo(move, undoStack.back());
    if(!didLastMoveLeaveInCheck()) {
        revertMove(move, undoStack.back());
        undoStack.pop_back();
        return false;
    }
    return true;
}

bool Board::didLastMoveLeaveInCheck() {
    Square kingSquare = getSquare(getLsb(sides[flipColor(turn)] & pieces[King]));
    return isSquareAttacked(kingSquare, flipColor(turn));
}

void Board::applyLegalMove(Move& move) {
    undoStack.emplace_back();
    applyMoveWithUndo(move, undoStack.back());
    assert(!didLastMoveLeaveInCheck());
}

void Board::applyMoveWithUndo(Move& move, UndoData& undo) {
    undo.positionHash = positionHash;
    undo.pawnKingHash = pawnKingHash;
    undo.kingAttackers = kingAttackers;
    undo.castlingRooks = castlingRooks;
    undo.enpassantSquare = enpassantSquare;
    undo.plies = plies;
    
    //TODO: store hash in history
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
        if (0 <= enpassantSquare && enpassantSquare <= 63) zobristChangeEnPassant(positionHash, static_cast<Board::Index>(enpassantSquare%8));
        enpassantSquare = None;
    }

    //flip whose turn it is
    turn = flipColor(turn);
    zobristFlipColor(positionHash);
    kingAttackers = getAllKingAttackers();
}

void Board::applyNormalMoveWithUndo(Move& move, UndoData& undo) {
    ColorPiece from = squares[move.getFrom()];
    ColorPiece to = squares[move.getTo()];
   
    //TODO: There's a bug here
    if(from == Empty) {
        std::cout << move.toString() << std::endl;
        debugPrintBitboard(sides[turn]);
        debugPrintBitboard(sides[flipColor(turn)]);
        std::cout << to << std::endl;
        if(to != Empty) {
            debugPrintBitboard(pieces[getPieceType(to)]);
        }
    }

    //zobrist hash update
    zobristChangePiece(undo.positionHash, getColorOfPiece(from), getPieceType(from), move.getFrom());
    zobristChangePiece(undo.positionHash, getColorOfPiece(from), getPieceType(from), move.getTo());
    if (to != Empty) zobristChangePiece(undo.positionHash, getColorOfPiece(to), getPieceType(to), move.getTo());
    if(getPieceType(from) == Pawn && (move.getTo() ^ move.getFrom()) == 16
    && 0 != (pieces[Pawn] & sides[flipColor(turn)] & PrecomputedBinary::getBinary().getAdjacentFilesMask(getFileIndexOfSquare(move.getFrom())) & ((turn == White) ? Rank4 : Rank5))) {
        zobristChangeEnPassant(positionHash, static_cast<Board::Index>(enpassantSquare%8));
    }

    //If we capture a piece OR move a pawn, reset the fifty move rule
    if(getPieceType(from) == Pawn || to != Empty) {
        plies = 0;
    } else {
        plies++;
    }
    pieces[getPieceType(from)] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    //if we captured
    if(to != Empty) {
        pieces[getPieceType(to)] ^= (1ull << move.getTo());
        sides[flipColor(turn)] ^= (1ull << move.getTo());
    }

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = from;

    castlingRooks &= castleMasks[move.getFrom()];
    castlingRooks &= castleMasks[move.getTo()];
    undo.pieceCaptured = to;
    //TODO:
    //psqt and hash

    //if we move 2 forward, set enpassant data
    if(getPieceType(from) == Pawn && (move.getTo() ^ move.getFrom()) == 16
    && 0 != (pieces[Pawn] & sides[flipColor(turn)] & PrecomputedBinary::getBinary().getAdjacentFilesMask(getFileIndexOfSquare(move.getFrom())) & ((turn == White) ? Rank4 : Rank5))) {
        enpassantSquare = getSquare((turn == White) ? move.getFrom() + 8 : move.getFrom() - 8);
    }
}

Board::Square Board::getKingCastlingSquare(Board::Square king, Board::Square rook) {
    return getSquare(getRankIndexOfSquare(king), static_cast<Index>(rook > king ? 6 : 2)); //return the castling square on the king's rank (which corresponds to the file)
}

Board::Square Board::getRookCastlingSquare(Board::Square king, Board::Square rook) {
    return getSquare(getRankIndexOfSquare(king), static_cast<Index>(rook > king ? 5 : 3));
}

void Board::applyCastlingMoveWithUndo(Move& move, Board::UndoData& undo) {
    Square kingFrom = move.getFrom();
    Square rookFrom = move.getTo();
    assert(getPieceType(squares[kingFrom]) == King);

    Square kingTo = getKingCastlingSquare(kingFrom, rookFrom);
    Square rookTo = getRookCastlingSquare(kingFrom, rookFrom);

    //zobrist hash update
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), King, kingTo);
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), King, kingFrom);
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), Rook, rookTo);
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), Rook, rookFrom);
    
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
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), Pawn, move.getTo());
    zobristChangePiece(undo.positionHash, getColorOfPiece(squares[move.getFrom()]), Pawn, move.getFrom());
    zobristChangePiece(undo.positionHash, flipColor(getColorOfPiece(squares[move.getFrom()])), Pawn, capturedSquare);
    
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
    zobristChangePiece(undo.positionHash, getColorOfPiece(promotedPiece), Pawn, move.getFrom());
    zobristChangePiece(undo.positionHash, getColorOfPiece(promotedPiece), move.getPromoType(), move.getTo());
    if (capturedPiece != Empty) zobristChangePiece(undo.positionHash, getColorOfPiece(capturedPiece), getPieceType(capturedPiece), move.getTo());

    //promotion resets the fifty move rule
    plies = 0;
    pieces[Pawn] ^= (1ull << move.getFrom());
    pieces[move.getPromoType()] ^= (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    if(capturedPiece != Empty) {
        pieces[getPieceType(capturedPiece)] ^= (1ull << move.getTo());
        sides[getColorOfPiece(capturedPiece)] ^= (1ull << move.getTo());
    }

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = promotedPiece;
    undo.pieceCaptured = capturedPiece;

    castlingRooks &= castleMasks[move.getTo()];
}

void Board::revertMostRecent(Move& move) {
    //todo: Add stuff here for NMP
    revertMove(move, undoStack.back());
    undoStack.pop_back();
}

void Board::revertMove(Move& move, UndoData& undo) {
    positionHash = undo.positionHash;
    pawnKingHash = undo.pawnKingHash;
    kingAttackers = undo.kingAttackers;
    enpassantSquare = undo.enpassantSquare;
    plies = undo.plies;
    castlingRooks = undo.castlingRooks;

    turn = flipColor(turn);
    moveCounter--;
    fullmoves--;

    switch(move.getMoveType()) {
        case Move::MoveType::Normal: {
            if(squares[move.getTo()] == Empty) {
                std::cout << move.toString() << std::endl;
                debugPrintBitboard(sides[flipColor(turn)]);
                debugPrintBitboard(sides[turn]);
                debugPrintBitboard(pieces[Pawn]);
                std::cout << enpassantSquare << std::endl;
            }
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
    Piece fromType = getPieceType(squares[move.getFrom()]);

    //The following are illegal moves that are theoretically valid as moves in our encoding
    if(move.isMoveNone() || getColorOfPiece(squares[move.getFrom()]) != turn || (move.getPromoType() != Knight && !move.isMovePromotion()) || (move.getMoveType() == Move::MoveType::Castle && fromType != King)) {
        return false;
    }

    Bitboard occupiedBoard = sides[White] | sides[Black];

    //Nonspecial moves from nonpawns are pseudo legal as long as they are normal and previously were attacking the squares


    if(fromType == Knight) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getKnightAttacksFromSquare(move.getFrom()) & ~sides[turn], move.getTo());
    }
    if(fromType == Bishop) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getBishopAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
    }
    if(fromType == Rook) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getRookAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
    }
    if(fromType == Queen) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(PrecomputedBinary::getBinary().getQueenAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
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

int Board::generateAllLegalMoves(std::vector<Move>& moveList) {
    const int startSize = moveList.size();
    std::vector<Move> pseudoLegalMoves;

    generateAllNoisyMoves(pseudoLegalMoves);
    generateAllQuietMoves(pseudoLegalMoves);

    undoStack.emplace_back();
    for(Move& move : pseudoLegalMoves) {
        //check legality
        applyMoveWithUndo(move, undoStack.back());
        if(!didLastMoveLeaveInCheck()) {
            moveList.emplace_back(move);
        }
        revertMove(move, undoStack.back());
    }

    return moveList.size() - startSize;
}
