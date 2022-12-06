#ifndef _DIFFICULTY_LEVEL_H
#define _DIFFICULTY_LEVEL_H

#include "board.h"
#include "move.h"
#include "moveorder.h"
#include "evaluator.h"
#include <memory>
#include <vector>

/**
 * A class representing the difficulty level of an AI opponent.
 * Generally speaking, the opponent has 3 components that contribute its strength:
 * The search - the component responsible for the algorithms to determine the best move in a position that relies on
 * The eval - From the perspective of search, a blackbox that gives back a number that says how good a position is.
 * The move orderer - Heuristically orders the moves that search searches through to optimize the algorithm's performance
 * The DifficultyLevel itself represents the search (since that is the outward facing interface, as we want to ask the opponent what move it wants to play)
 * and owns eval and move orderer objects.
 */
class DifficultyLevel {
public:
    DifficultyLevel(const Evaluator& evaluator, const MoveOrderer& moveOrderer) : evaluator{evaluator.clone()} {
        moveOrderers.emplace_back(moveOrderer.clone());
    }
    virtual Move getMove(Board& board) = 0; 
    virtual ~DifficultyLevel() = default;
protected:
    /**
     * The evaluator this difficulty level uses to tell how good a position is.
     */
    std::unique_ptr<Evaluator> evaluator;
    /**
     * The move orderers that this difficulty level uses to pick moves. 
     * Usually indexed by depth. 
     */
    std::vector<std::unique_ptr<MoveOrderer> > moveOrderers;
};

#endif