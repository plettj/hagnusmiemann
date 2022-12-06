#include <numeric>
#include "evaluator.h"

static CentipawnScore pawnPoints = 100, knightPoints = 320, bishopPoints = 330, rookPoints = 510, queenPoints = 880;

CentipawnScore EvalLevelThree::getPieceValue(Piece piece) {
    switch(piece) {
        case Pawn:
            return pawnPoints;
        case Knight:
            return knightPoints;
        case Bishop:
            return bishopPoints;
        case Rook:
            return rookPoints;
        case Queen:
            return queenPoints;
        default:
            assert(false);                    
    }
}

CentipawnScore EvalLevelThree::staticEvaluate(const Board& board) {

    CentipawnScore eval = 0;

    for (int i = 0; i <= NumSquares; ++i) {
        switch(board.getPieceAt(getSquareFromIndex(i))) {
            case Empty:
                break;
            case WhitePawn:
                eval += pawnPoints;
                break;
            case BlackPawn:
                eval -= pawnPoints;
                break;
            case WhiteKnight:
                eval += knightPoints;
                break;
            case BlackKnight:
                eval -= knightPoints;
                break;
            case WhiteBishop:
                eval += bishopPoints;
                break;
            case BlackBishop:
                eval -= bishopPoints;
                break;
            case WhiteRook:
                eval += rookPoints;
                break;
            case BlackRook:
                eval -= rookPoints;
                break;
            case WhiteQueen:
                eval += queenPoints;
                break;
            case BlackQueen:
                eval -= queenPoints;
                break;
            default:
                break;
        }
    }

    return board.getTurn() == White ? eval : -eval;
}

/*
CentipawnScore pawnPoints[NumSquares] = {
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,
    };
    CentipawnScore knightPoints[NumSquares] = {
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
        320,320,320,320,320,320,320,320,
    };
    CentipawnScore bishopPoints[NumSquares] = {
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
        330,330,330,330,330,330,330,330,
    };
    CentipawnScore rookPoints[NumSquares] = {
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
        510,510,510,510,510,510,510,510,
    };
    CentipawnScore queenPoints[NumSquares] = {
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
        880,880,880,880,880,880,880,880,
    };
*/
