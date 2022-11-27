#include "move.h"

Move::Move() : from{Board::Square::a1}, to{Board::Square::a1}, moveType{Normal}, promotionType{Board::Piece::Knight} {}

Move::Move(Board::Square from, Board::Square to, Move::MoveType moveType, Board::Piece promoType) : from{from}, to{to}, moveType{moveType}, promotionType{promoType} { }

Board::Square Move::getFrom() const {
    return from;
}

bool Move::isMoveNone() const {
    return from == Board::Square::a1 && to == Board::Square::a1 && moveType == Normal && promotionType == Board::Piece::Knight;
}

bool Move::isMovePromotion() const {
    return getMoveType() == Promotion;
}

Board::Square Move::getTo() const {
    return to;
}

Move::MoveType Move::getMoveType() const {
    return moveType;
}

Board::Piece Move::getPromoType() const {
    return promotionType;
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
        switch(getPromoType()) {
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

