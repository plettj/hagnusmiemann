#include "easydifficulty.h"

LevelOne::LevelOne() : DifficultyLevel{TrivialEvaluator{}, RandomMoveOrderer{}} {}

LevelTwo::LevelTwo() : DifficultyLevel{TrivialEvaluator{}, RandomMoveOrderer{}} {}

Move LevelOne::getMove(Board& board) {
    moveOrderer->seedMoveOrderer(board, false);
    Move move = moveOrderer->pickNextMove(false);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderer->pickNextMove(false);
    } while(!move.isMoveNone());
    return Move{};
}

Move LevelTwo::getMove(Board& board) {
    moveOrderer->seedMoveOrderer(board, true);
    Move move = moveOrderer->pickNextMove(true);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderer->pickNextMove(true);
    } while(!move.isMoveNone());

    moveOrderer->seedMoveOrderer(board, false);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderer->pickNextMove(false);
    } while(!move.isMoveNone());
    return Move{};
}