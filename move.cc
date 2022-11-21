#include "move.h"

Move::Move() : moveBits{0} {}

Move::Move(Board::Square from, Board::Square to, Move::MoveType moveType, Move::MovePromotionType promoType) : moveBits{(uint16_t) (from | (to << 6) | promoType)} { }

Board::Square Move::getFrom() const {
    return static_cast<Board::Square>(moveBits & 63);
}

bool Move::isMoveNone() const {
    return moveBits == 0;
}

bool Move::isMovePromotion() const {
    return getMoveType() == Promotion;
}

Board::Square Move::getTo() const {
    return static_cast<Board::Square>((moveBits >> 6) & 63);
}

Move::MoveType Move::getMoveType() const {
    return static_cast<MoveType>(moveBits & (0b11 << 12));
}

Move::MovePromotionType Move::getPromoType() const {
    return static_cast<MovePromotionType>(moveBits & (0b11 << 14));
}

Board::Piece Move::getMovePromoPiece() const {
    return static_cast<Board::Piece>(1 + (moveBits >> 14));
}

void Move::print(std::ostream& out) const {
    out << toString() << std::endl;
}

std::string Move::toString() const {
    Board::Square from = getFrom();
    Board::Square to = getTo();
    
    std::string fr = Board::squareToString(from);
    std::string t = Board::squareToString(to);
    fr += t;

    if(isMovePromotion()) {
        switch(getMovePromoPiece()) {
	    case Board::Piece::King:	
	    case Board::Piece::Pawn:
                return "AHHHHHHHHHHH"; //that's no bueno
	    case Board::Piece::Knight:
		fr += "N";
		break;
	    case Board::Piece::Bishop:	
		fr += "B";
		break;
	    case Board::Piece::Rook:
	        fr += "R";
	        break;
	    case Board::Piece::Queen:
	        fr += "Q";
	        break;	
	}
    }
    return fr;
}

