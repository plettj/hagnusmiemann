#ifndef _MOVE_H_CHESS
#define _MOVE_H_CHESS
#include <string>
#include <iostream>
#include "board.h"

typedef uint16_t MoveBits;
/**
 * More or less a POD class to store moves,
 * but in an efficient way internally
 * (more binary tricks!)
 */ 
class Move {
public:
    /**
     * Some binary encodings to store a move in 16 bits
     */ 
    enum MoveType : uint16_t {
	//TODO: nullmove    
	Normal = 0, //= 0b00 << 12, note that this is combined with other things (the squares) that makes it _NONZERO_
	Castle = 0b01 << 12, //= b00001000000000000
	Enpassant = 0b10 << 12, //= b00010000000000000
	Promotion = 0b11 << 12 //= b00011000000000000
    };
    enum MovePromotionType : uint16_t {
        PromoteKnight = 0, //= 0b00 << 14, this is combined with Promotion internally to differ from a zero
	PromoteBishop = 0b01 << 14, //= b0100000000000000
	PromoteRook = 0b10 << 14, //= b1000000000000000
	PromoteQueen = 0b11 << 14, //= b1100000000000000
        	
	KnightPromotion = Promotion | PromoteKnight,
	BishopPromotion = Promotion | PromoteBishop,
	RookPromotion = Promotion | PromoteRook,
	QueenPromotion = Promotion | PromoteQueen
    };     

    /* Creates an empty move.
     * NOTE: moves are immutable, so doing this cannot be changed
     */
    Move();
    Move(Board::Square from, Board::Square to, MoveType moveType, MovePromotionType promoType);

    Board::Square getFrom() const;
    Board::Square getTo() const;
    MoveType getMoveType() const;
    MovePromotionType getPromoType() const;
    Board::Piece getMovePromoPiece() const;
    bool isMoveNone() const;
    bool isMovePromotion() const;

    void print(std::ostream& out) const;
    std::string toString() const;
private:
    const MoveBits moveBits;
};

#endif
