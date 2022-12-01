#include <numeric>
#include "evaluator.h"


CentipawnScore evalLevelThree::staticEvaluate(const Board& board) {
    CentipawnScore pawnPoints, knightPoints, bishopPoints, rookPoints, queenPoints;
    pawnPoints = 100;
    knightPoints = 320;
    bishopPoints = 330;
    rookPoints = 510;
    queenPoints = 880;

    CentipawnScore eval = 0;

    for (int i = 0; i <= NumSquares; ++i) {
        #include<stdio.h>
        std::cout << eval << std::endl;
        switch(board.getPieceAt(static_cast<Square>(i))) {
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

    return eval;
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
