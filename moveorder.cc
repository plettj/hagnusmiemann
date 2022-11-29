#include "moveorder.h"

MoveOrderer::MoveOrderer(Board& board) : board{board} {
    currentMoveScores.reserve(Board::MaxNumMoves);
}

HeuristicMoveOrderer::HeuristicMoveOrderer(Board& board) : MoveOrderer{board} {}

void HeuristicMoveOrderer::updateHeuristics(std::vector<Move>& moveList, int depth) {

}

void HeuristicMoveOrderer::seedMoveOrderer(bool noisyOnly) {
    this->noisyOnly = noisyOnly;
    std::vector<Move> moveList;
    if(noisyOnly) {
        //Don't play refutation moves on noisy moves, they're tactical enough such that we
        //should just calculate them and not heuristically refute them.
        killerOne = Move{};
        killerTwo = Move{};
        counter = Move{};
        board.generateAllNoisyMoves(moveList);
    } else {
        //Generate refutation moves
        
        board.generateAllPseudoLegalMoves(moveList);
    }

    for(auto const& move : moveList) {

    }
}