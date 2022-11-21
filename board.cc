#include "board.h"

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

Board::Board() : positionHash{0}, pawnKingHash{0}, kingAttackers{0}, threats{0}, castlingRooks{0}, turn{White}, plies{0}, fullmoves{0} {
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

    //TODO: Set kingAttackers and threats
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
