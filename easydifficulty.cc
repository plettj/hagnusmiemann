#include "easydifficulty.h"

LevelOne::LevelOne() : DifficultyLevel{TrivialEvaluator{}, RandomMoveOrderer{}} {}

LevelTwo::LevelTwo() : DifficultyLevel{TrivialEvaluator{}, RandomMoveOrderer{}} {}

Move LevelOne::getMove(Board& board) {
    //Pick a random move:
    moveOrderers[0]->seedMoveOrderer(board, false);
    Move move = moveOrderers[0]->pickNextMove(false);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderers[0]->pickNextMove(false);
    } while(!move.isMoveNone());
    return Move{};
}

Move LevelTwo::getMove(Board& board) {
    //Pick a random move that preferably is a check or a noisy capture:
    moveOrderers[0]->seedMoveOrderer(board, true);
    Move move = moveOrderers[0]->pickNextMove(true);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderers[0]->pickNextMove(true);
    } while(!move.isMoveNone());

    moveOrderers[0]->seedMoveOrderer(board, false);
    do {
        bool legal = board.applyMove(move);
        if(legal) {
            board.revertMostRecent();
            return move;
        }
        move = moveOrderers[0]->pickNextMove(false);
    } while(!move.isMoveNone());
    return Move{};
}