#include "board.h"

static bool staticAttacksInitialized = false;

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
    return static_cast<Square>(rankIndex * NumFiles + fileIndex);
}

Board::Square Board::getRelativeSquare(Board::Color side, Board::Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side
    return getSquare(getRelativeRankIndexOfSquare(side, square), getFileIndexOfSquare(square));
}

Board::Square Board::getRelativeSquare32(Board::Color side, Board::Square square) {
    assert(square != None);
    //get the relative square ours is on relative to our side and limited to the left half of the board
    return static_cast<Square>(4 * getRelativeRankIndexOfSquare(side, square) + getMirrorFileIndex(getFileIndexOfSquare(square)));
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
    for(int i = 7; i >= 0; i--) {
        for(int j = 0; j < 7; j++) {
	    std::cout << testBit(bb, getSquare(static_cast<Index>(i), static_cast<Index>(j)));
	}
	std::cout << std::endl;
    }
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
    return getSquare(static_cast<Index>(string[1] - '1'), static_cast<Index>(string[0] - 'a'));
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
    initializeStaticAttacks();
}

void Board::setBitboardSquare(Bitboard& board, int rankIndex, int fileIndex) {
    if(0 <= rankIndex && rankIndex <= NumRanks && 0 <= fileIndex && fileIndex <= NumFiles) {
        board |= 1ull << getSquare(static_cast<Index>(rankIndex), static_cast<Index>(fileIndex));        
    }
}

void Board::initializeStaticAttacks() {
    if(staticAttacksInitialized) {
        return;
    }
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
	Square sq = static_cast<Square>(square);
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

    staticAttacksInitialized = true;
}

Bitboard Board::getBishopAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
    return BishopTable[square].offset[computeHashTableIndex(occupiedBoard, BishopTable[square])];
}

Bitboard Board::getRookAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
    return RookTable[square].offset[computeHashTableIndex(occupiedBoard, RookTable[square])];
}

Bitboard Board::getQueenAttacksFromSquare(Board::Square square, Bitboard occupiedBoard) {
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
    Color inverseSide = side == White ? Black : White;
    return enpassantSquare == None ? 0 : PawnAttack[inverseSide][enpassantSquare] & pawnBoard;
}

Bitboard Board::getAllSquareAttackers(Bitboard occupiedBoard, Board::Square square) {
    return (PawnAttack[White][square] & sides[Black] & pieces[Pawn])
	   | (PawnAttack[Black][square] & sides[White] & pieces[Pawn])
	   | (KnightAttack[square] & pieces[Knight])
	   | (getBishopAttacksFromSquare(square, occupiedBoard) & (pieces[Bishop] | pieces[Queen]))
	   | (getRookAttacksFromSquare(square, occupiedBoard) & (pieces[Rook] | pieces[Rook]))
	   | (KingAttack[square] & pieces[King]);
}

Bitboard Board::getAllKingAttackers() {
    Square square = static_cast<Square>(getLsb(sides[turn] & pieces[King]));
    Bitboard occupiedBoard = sides[White] | sides[Black];
    return getAllSquareAttackers(occupiedBoard, square);
}

bool Board::isSquareAttacked(Square square, Board::Color side) {
    Board::Color inverseColor = side == White ? Black : White;
    Bitboard enemyPieces = sides[inverseColor];
    Bitboard occupiedBoard = sides[White] | sides[Black];

    Bitboard enemyPawns = enemyPieces & pieces[Pawn];
    Bitboard enemyKnights = enemyPieces & pieces[Knight];
    Bitboard enemyBishops = enemyPieces & (pieces[Bishop] | pieces[Queen]);
    Bitboard enemyRooks = enemyPieces & (pieces[Bishop] | pieces[Queen]);
    Bitboard enemyKings = enemyPieces & pieces[King];

    //avoid doing hash lookups via short circuit if we can
    return (PawnAttack[side][square] & enemyPawns) || (KnightAttack[square] & enemyKnights) || (KingAttack[square] & enemyKings) || (enemyBishops != 0 && (getBishopAttacksFromSquare(square, occupiedBoard) & enemyBishops)) || (enemyRooks != 0 && (getRookAttacksFromSquare(square, occupiedBoard) & enemyRooks));
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
    Square square = a8;

    Bitboard rooks;
    Bitboard kings;
    Bitboard white;
    Bitboard black;
    
    std::string token = fen.substr(0, fen.find(" "));
    for(char& c : token) {
        if(std::isdigit(c)) {
	    square = static_cast<Square>(square + c - '0');
	} else if(c == '/') {
	    square = static_cast<Square>(square - 16);
	} else {
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
            board.setSquare(color, piece, square);
	    //this won't overflow under the assumption the FEN is well-formed
            square = static_cast<Square>(square + 1);	    
	}
    }
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    board.turn = token[0] == 'w' ? White : Black;
    //TODO: update hashes throughout
    
    fen.erase(0, fen.find(" ") + 1);
    token = fen.substr(0, fen.find(" "));
    rooks = board.pieces[Rook];
    kings = board.pieces[King];
    white = board.sides[White];
    black = board.sides[Black];
    //castling rights
    for(char& c : token) {
        if(c == 'K') {
	    setBit(board.castlingRooks, static_cast<Square>(getMsb(white & rooks & Rank1))); 
	} else if(c == 'Q') {
	    setBit(board.castlingRooks, static_cast<Square>(getLsb(white & rooks & Rank1)));
	} else if(c == 'k') {
	    setBit(board.castlingRooks, static_cast<Square>(getMsb(black & rooks & Rank8)));
	} else if(c == 'q') {
	    setBit(board.castlingRooks, static_cast<Square>(getLsb(black & rooks & Rank8)));
	}
    }

    //Create a bit mask of where the kings and rooks are
    for(int sq = 0; sq < NumSquares; sq++) {
	Square s = static_cast<Square>(sq);
        board.castleMasks[s] = ~0ull;
	if(board.testBit(board.castlingRooks, s)) {
	    clearBit(board.castleMasks[sq], s);
	} else if(board.testBit(white & kings, s)) {
	    board.castleMasks[sq] &= ~white;
	} else if(board.testBit(black & kings, s)) {
	    board.castleMasks[sq] &= ~black;
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

    //TODO: Set kingAttackers
    return board;
}

std::string Board::getFEN() const {
    return "not implemented yet";
}

void Board::printBoard(std::ostream& out) const {
    out << "TODO" << std::endl;
}

unsigned long long Board::perftTest(int depth) {
    //TODO:
    return 0;
}

//TODO: Attacks, then moves, then move generation, then verify perft
Bitboard Board::calculateRookBishopAttacks(Square square, Bitboard occupiedBoard, const MultiArray<int, 4, 2>& movementDelta) {
    Bitboard result = 0;
    for(int i = 0; i < 4; i++) {
        int rankChange = movementDelta[i][0];
	int fileChange = movementDelta[i][1];

	for(int rankIndex = getRankIndexOfSquare(square) + rankChange, fileIndex = getFileIndexOfSquare(square) + fileChange; 0 <= rankIndex && rankIndex <= NumRanks && 0 <= fileIndex && fileIndex <= NumFiles; rankIndex += rankChange, fileIndex += fileChange) {
	    //in the direction we move, add a bit to the bitboard
	    setBit(result, getSquare(static_cast<Index>(rankIndex), static_cast<Index>(fileIndex)));
	    //stop if we hit something that blocks us
	    if(testBit(occupiedBoard, getSquare(static_cast<Index>(rankIndex), static_cast<Index>(fileIndex)))) {
	        break;
	    }
	}
    }
    return result;
}

void Board::populateHashTable(HashEntry* table, Square square, Bitboard hash, const MultiArray<int, 4, 2>& movementDelta) {
    Bitboard occupiedBoard = 0;

    table[square].hash = hash;
    //Subtract the edges of the board that we aren't on, since if we hit the edge we can't be blocked by anything (there is nowhere left to go)
    table[square].mask = calculateRookBishopAttacks(square, 0, movementDelta) & ~(((FileA | FileH) & ~getFile(getFileIndexOfSquare(square))) | ((Rank1 | Rank8) & ~getRank(getRankIndexOfSquare(square))));
    table[square].shift = 64 - popCnt(table[square].mask);

    if(square != h8) {
        table[square + 1].offset = table[square].offset + (1 << popCnt(table[square].mask));
    }
    
    //Initialize the attacks at every square
    do {
        table[square].offset[computeHashTableIndex(occupiedBoard, table[square])] = calculateRookBishopAttacks(square, occupiedBoard, movementDelta);
        occupiedBoard = (occupiedBoard - table[square].mask) & table[square].mask;
    } while(occupiedBoard != 0);
}



