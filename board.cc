#include "board.h"
#include "move.h"

/**
 * The following are what correspond to the functionality to determine (very efficiently!) if a square is attacked or not.
 * These are arrays of bitboards corresponding to where we can attack with a piece on a given square.
 * Kings, Pawns, and Knights are very simple to determine where they can attack (it's a hard and fast rule, they are never blocked by anything). 
 */
alignas(64) static Bitboard PawnAttack[Board::NumColors][Board::NumSquares]; //these are sided based on direction, since pawns are the only piece that aren't symmetric
alignas(64) static Bitboard KnightAttack[Board::NumSquares];
alignas(64) static Bitboard KingAttack[Board::NumSquares];
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
static constexpr Bitboard RookHashes[Board::NumSquares] = {
    0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull, 0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
    0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull, 0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
    0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull, 0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
    0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull, 0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
    0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull, 0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
    0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull, 0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
    0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull, 0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
    0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull, 0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull
};
static constexpr Bitboard BishopHashes[Board::NumSquares] = {
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
alignas(64) static Bitboard BishopAttack[0x1480];
alignas(64) static Bitboard RookAttack[0x19000];
//these are the hash tables, indexed by square
alignas(64) static Board::HashEntry BishopTable[Board::NumSquares];
alignas(64) static Board::HashEntry RookTable[Board::NumSquares];
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
    Square square = getSquare(getLsb(sides[turn] & pieces[King]));
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
	    square = getSquare(square + c - '0');
	} else if(c == '/') {
	    square = getSquare(square - 16);
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
            square = getSquare(square + 1);	    
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
	    setBit(board.castlingRooks, getSquare(getMsb(white & rooks & Rank1))); 
	} else if(c == 'Q') {
	    setBit(board.castlingRooks, getSquare(getLsb(white & rooks & Rank1)));
	} else if(c == 'k') {
	    setBit(board.castlingRooks, getSquare(getMsb(black & rooks & Rank8)));
	} else if(c == 'q') {
	    setBit(board.castlingRooks, getSquare(getLsb(black & rooks & Rank8)));
	}
    }

    //Create a bit mask of where the kings and rooks are
    for(int sq = 0; sq < NumSquares; sq++) {
	Square s = getSquare(sq);
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

Board::ColorPiece Board::getPieceAt(Square square) const {
    assert(square != Empty);
    return squares[square];
};
Board::Square Board::getKing() const {
    return static_cast<Square>(getLsb(pieces[King] & sides[turn]));
};
Board::Color Board::getTurn() const {
    return turn;
};

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

void Board::applyLegalMove(Move& move) {
    
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
    }

    //if the enpassant square was not updated (i.e. no 2 pawn forward move was played),
    //then enpassant expires and we must remove it
    if(enpassantSquare != undo.enpassantSquare) {
        enpassantSquare = None;
    }
    //flip whose turn it is
    turn = turn == White ? Black : White;
    kingAttackers = getAllKingAttackers();
}

void Board::applyNormalMoveWithUndo(Move& move, UndoData& undo) {
    ColorPiece from = squares[move.getFrom()];
    ColorPiece to = squares[move.getTo()];

    Piece fromType = getPieceType(from);
    Piece toType = getPieceType(to);
   
    //If we capture a piece OR move a pawn, reset the fifty move rule
    if(fromType == Pawn || to != Empty) {
        plies = 0;
    } else {
        plies++;
    }
    pieces[fromType] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    pieces[toType] ^= (1ull << move.getTo());
    sides[turn == White ? Black : White] ^= (1ull << move.getTo());

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = from;

    castlingRooks &= castleMasks[move.getFrom()];
    castlingRooks &= castleMasks[move.getTo()];

    //TODO:
    //psqt and hash

    //if we move 2 forward, set enpassant data
    if(fromType == Pawn && (move.getTo() ^ move.getFrom()) == 16) {
        //TODO: need adjacent file mask
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

    Square kingTo = getKingCastlingSquare(kingFrom, rookFrom);
    Square rookTo = getRookCastlingSquare(kingFrom, rookFrom);
    
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
    Color inverseSide = turn == White ? Black : White;
    
    //en passant is a capture, so reset the fifty move rule
    plies = 0;
    pieces[Pawn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    pieces[Pawn] ^= (1ull << capturedSquare);
    sides[inverseSide] ^= (1ull << capturedSquare);

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = makePiece(Pawn, turn);
    squares[capturedSquare] = Empty;
    undo.pieceCaptured = makePiece(Pawn, inverseSide);
    //TODO: hash
}

void Board::applyPromotionMoveWithUndo(Move& move, Board::UndoData& undo) {
    ColorPiece promotedPiece = makePiece(move.getMovePromoPiece(), turn);
    ColorPiece capturedPiece = squares[move.getTo()];

    //promotion resets the fifty move rule
    plies = 0;
    pieces[Pawn] ^= (1ull << move.getFrom());
    pieces[move.getMovePromoPiece()] ^= (1ull << move.getTo());
    sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

    pieces[getPieceType(capturedPiece)] ^= (1ull << move.getTo());
    sides[getColorOfPiece(capturedPiece)] ^= (1ull << move.getTo());

    squares[move.getFrom()] = Empty;
    squares[move.getTo()] = promotedPiece;
    undo.pieceCaptured = capturedPiece;

    castlingRooks &= castleMasks[move.getTo()];

    //TODO: hash
}

void Board::revertMostRecent(Move& move) {
    //todo: Add stuff here for NMP
    revertMove(move, undoStack.back());
}

void Board::revertMove(Move& move, UndoData& undo) {
    positionHash = undo.positionHash;
    pawnKingHash = undo.pawnKingHash;
    kingAttackers = undo.kingAttackers;
    enpassantSquare = undo.enpassantSquare;
    plies = undo.plies;
    castlingRooks = undo.castlingRooks;

    turn = turn == White ? Black : White;
    moveCounter--;
    fullmoves--;

    switch(move.getMoveType()) {
        case Move::MoveType::Normal: {
            Piece fromType = getPieceType(squares[move.getTo()]);
            Piece captureType = getPieceType(undo.pieceCaptured);
            Color capturedColor = getColorOfPiece(undo.pieceCaptured);

            pieces[fromType] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            pieces[captureType] ^= (1ull << move.getTo());
            sides[capturedColor] ^= (1ull << move.getTo());

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
            Piece capturedPiece = getPieceType(undo.pieceCaptured);
            Color capturedColor = getColorOfPiece(undo.pieceCaptured);

            pieces[Pawn] ^= (1ull << move.getFrom());
            pieces[move.getMovePromoPiece()] ^= (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            pieces[capturedPiece] ^= (1ull << move.getTo());
            sides[capturedColor] ^= (1ull << move.getTo());

            squares[move.getFrom()] = makePiece(Pawn, turn);
            squares[move.getTo()] = undo.pieceCaptured;
            break;
        }
        case Move::MoveType::Enpassant: {
            Square enpassantCaptureSquare = getSquare(move.getTo() - 8 + (turn >> 4));

            pieces[Pawn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());
            sides[turn] ^= (1ull << move.getFrom()) ^ (1ull << move.getTo());

            pieces[Pawn] ^= (1ull << enpassantCaptureSquare);
            sides[turn == White ? Black : White] ^= (1ull << enpassantCaptureSquare);

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
    if(move.isMoveNone() || getColorOfPiece(squares[move.getFrom()]) != turn || (move.getPromoType() != Move::MovePromotionType::KnightPromotion && !move.isMovePromotion()) || (move.getMoveType() == Move::MoveType::Castle && fromType != King)) {
        return false;
    }

    Bitboard occupiedBoard = sides[White] | sides[Black];

    //Nonspecial moves from nonpawns are pseudo legal as long as they are normal and previously were attacking the squares


    if(fromType == Knight) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(KnightAttack[move.getFrom()] & ~sides[turn], move.getTo());
    }
    if(fromType == Bishop) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(getBishopAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
    }
    if(fromType == Rook) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(getRookAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
    }
    if(fromType == Queen) {
        return move.getMoveType() == Move::MoveType::Normal && testBit(getQueenAttacksFromSquare(move.getFrom(), occupiedBoard), move.getTo());
    }
    if(fromType == King && move.getMoveType() == Move::MoveType::Normal) {
        return testBit(KingAttack[move.getFrom()] & ~sides[turn], move.getTo());
    }
    if(fromType == Pawn) {
        if(move.getMoveType() == Move::MoveType::Enpassant) {
            return move.getTo() == enpassantSquare && testBit(PawnAttack[turn][move.getFrom()], move.getTo());
        }

        Bitboard pawnAdvance = getPawnAdvances(1ull << move.getFrom(), occupiedBoard, turn);

        if(move.getMoveType() == Move::MoveType::Promotion) {
            return testBit(LastRanks & ((PawnAttack[turn][move.getFrom()] & sides[turn == White ? Black : White]) | pawnAdvance), move.getTo());
        }
        //Alright, not enpassant or promotion, must be normal.
        //This includes 2 forward at start, so add those by including the 3rd rank as a starting point for a forward pawn move 
        pawnAdvance |= getPawnAdvances(pawnAdvance & (turn == White ? Rank3 : Rank6), occupiedBoard, turn);

        return testBit(~LastRanks & ((PawnAttack[turn][move.getFrom()] & sides[turn == White ? Black : White]) | pawnAdvance), move.getTo());
    }
    //must be castling (we ruled out king normals above)
    if(move.getMoveType() != Move::MoveType::Castle) {
        return false;
    }

}

bool Board::isMoveLegal(Move& move) {
    Color previousTurn = turn == White ? Black : White;
    Square kingSquare = getSquare(getLsb(pieces[King] & sides[previousTurn]));
    return isMovePseudoLegal(move) && !isSquareAttacked(kingSquare, previousTurn);
}
