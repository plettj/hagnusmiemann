#include <numeric>
#include "evaluator.h"

/**
 * Approximate material values.
 */
static CentipawnScore pawnPoints = 100, knightPoints = 320, bishopPoints = 330, rookPoints = 510, queenPoints = 880;

CentipawnScore Evaluator::getPieceValue(Piece piece) {
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
    return 0;
}

/**
 * Just counts material
 */
CentipawnScore EvalLevelThree::staticEvaluate(const Board& board) {

    CentipawnScore eval = 0;

    for (int i = 0; i < NumSquares; ++i) {
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

CentipawnScore EvalLevelFour::staticEvaluate(const Board& board) {
    if(board.isBoardMaterialDraw()) {
        return 0;
    }
    // Give bonuses to positionally good things (like rooks on open files)
    // and penalize bad things (like isolated pawns).
    CentipawnScore isolatedPawns = IsolatedPawnBonus * (board.getNumberOfIsolatedPawns(White) - board.getNumberOfIsolatedPawns(Black));
    CentipawnScore passedPawns = PassedPawnBouns * (board.getNumberOfPassedPawns(White) - board.getNumberOfPassedPawns(Black));
    CentipawnScore bishopPair = 0;
    if(board.getSidePieceCount(White, Bishop) >= 2) {
        bishopPair += BishopPairBonus;
    }
    if(board.getSidePieceCount(Black, Bishop) >= 2) {
        bishopPair -= BishopPairBonus;
    }
    CentipawnScore rookBonus = RookOpenFileBonus * (board.getNumberOfPiecesOnOpenFile(White, Rook) - board.getNumberOfPiecesOnOpenFile(Black, Rook));
    rookBonus += RookSemiOpenFileBonus * (board.getNumberOfPiecesOnSemiOpenFile(White, Rook) - board.getNumberOfPiecesOnSemiOpenFile(Black, Rook));
    CentipawnScore queenBonus = QueenOpenFileBonus * (board.getNumberOfPiecesOnOpenFile(White, Queen) - board.getNumberOfPiecesOnOpenFile(Black, Queen));
    queenBonus += QueenSemiOpenFileBonus * (board.getNumberOfPiecesOnSemiOpenFile(White, Queen) - board.getNumberOfPiecesOnSemiOpenFile(Black, Queen));

    CentipawnScore subtotal = board.getCurrentPsqt() + isolatedPawns + bishopPair + passedPawns + rookBonus + queenBonus;
    return TempoBonus + (board.getTurn() == White ? subtotal : -subtotal);
}
