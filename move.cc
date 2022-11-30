#include "move.h"
#include "board.h"

Move::Move() : from{Square::a1}, to{Square::a1}, moveType{Normal}, promotionType{Piece::Knight} {}

Move::Move(Square from, Square to, Move::MoveType moveType, Piece promoType) : from{from}, to{to}, moveType{moveType}, promotionType{promoType} { }

bool Move::operator==(const Move& other) const {
    return from == other.from && to == other.to && moveType == other.moveType && promotionType == other.promotionType;
}

bool Move::operator!=(const Move& other) const {
    return !(*this == other);
}

Square Move::getFrom() const {
    return from;
}

Square Move::getEnpassantSquareCaptured(Color turn) const {
    assert(moveType == Enpassant);
    return getSquareFromIndex(to - 8 + (turn << 4));
}

bool Move::isMoveNone() const {
    return from == Square::a1 && to == Square::a1 && moveType == Normal && promotionType == Piece::Knight;
}

bool Move::isMovePromotion() const {
    return getMoveType() == Promotion;
}

Square Move::getTo() const {
    return to;
}

Move::MoveType Move::getMoveType() const {
    return moveType;
}

Piece Move::getPromoType() const {
    return promotionType;
}

void Move::print(std::ostream& out) const {
    out << toString() << std::endl;
}

std::string Move::toString() const {
    Square from = getFrom();
    Square to = getTo();
    
    std::string fr = Board::squareToString(from);
    std::string t = Board::squareToString(to);
    fr += t;

    if(isMovePromotion()) {
        switch(getPromoType()) {
	        case Piece::King:	
	        case Piece::Pawn:
                return "AHHHHHHHHHHH"; //that's no bueno
	        case Piece::Knight:
		        fr += "N";
		        break;
	        case Piece::Bishop:	
		        fr += "B";
		        break;
	        case Piece::Rook:
	            fr += "R";
	            break;
	        case Piece::Queen:
	            fr += "Q";
	            break;	
	    }
    }
    return fr;
}

