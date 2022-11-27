#ifndef _MOVE_H_CHESS
#define _MOVE_H_CHESS
#include <string>
#include <iostream>
#include "board.h"

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

    /* Creates an empty move.
     * NOTE: moves are immutable, so doing this cannot be changed
     */
    Move();
    Move(Board::Square from, Board::Square to, MoveType moveType, Board::Piece promotionPiece = Board::Piece::Knight);

    Board::Square getFrom() const;
    Board::Square getTo() const;
    MoveType getMoveType() const;
    Board::Piece getPromoType() const;
    bool isMoveNone() const;
    bool isMovePromotion() const;

    void print(std::ostream& out) const;
    std::string toString() const;
private:
   // const MoveBits moveBits;
    const Board::Square from;
    const Board::Square to;
    const MoveType moveType;
    const Board::Piece promotionType;
};

#endif
