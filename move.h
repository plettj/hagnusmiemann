#ifndef _MOVE_H_CHESS
#define _MOVE_H_CHESS
#include <string>
#include <iostream>
#include "constants.h"

/**
 * More or less a POD class to store moves
 */ 
class Move {
public:
    /**
     * Some binary encodings to store a move in 16 bits
     */ 
    enum MoveType : uint16_t {
	//TODO: nullmove    
	    Normal = 0, //= 0b00 << 12, note that this is combined with other things (the squares) that makes it _NONZERO_
	    Castle,
	    Enpassant,
	    Promotion
    };

    /* 
     * Creates an empty move.
     * NOTE: moves are immutable, so doing this cannot be changed
     */
    Move();
    Move(Square from, Square to, MoveType moveType, Piece promotionPiece = Piece::Knight);
    Move& operator=(const Move& move);

    Square getFrom() const;
    Square getTo() const;
    MoveType getMoveType() const;
    Piece getPromoType() const;
    bool isMoveNone() const;
    bool isMovePromotion() const;

    void print(std::ostream& out) const;
    std::string toString() const;
private:
    Square from;
    Square to;
    MoveType moveType;
    Piece promotionType;
};

#endif
