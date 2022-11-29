#ifndef _MOVE_ORDER_H
#define _MOVE_ORDER_H

#include "board.h"
#include "move.h"
#include "evaluator.h"
#include <unordered_map>

typedef int HeuristicScore;

/**
 * Abstract base class for move ordering, letting us have difficulty levels with simpler move orderers
 * that we can use with polymorphism.
 * The point of move ordering is to order the moves we give back to the search, ordered heuristically
 * such that it is more likely to cause an alpha beta prune.  
 */
class MoveOrderer {
public:    
    MoveOrderer(Board& board);
    virtual Move pickNextMove(bool noisyOnly) = 0;
    virtual ~MoveOrderer() = default;
protected:
    Board& board;
    std::unordered_map<Move, HeuristicScore> currentMoveScores;    
};

/**
 * Just gives back a random move for dumb opponents
 */
class RandomMoveOrderer : public MoveOrderer {
public:    
    RandomMoveOrderer(Board& board);
    RandomMoveOrderer(const RandomMoveOrderer& other) = delete;
    Move pickNextMove(bool noisyOnly) override;
};

/**
 * The main move orderer that does non trivial useful things. 
 */
class HeuristicMoveOrderer : public MoveOrderer {
public:    
    HeuristicMoveOrderer(Board& board);
    HeuristicMoveOrderer(const HeuristicMoveOrderer& other) = delete;

    //TODO: Static Exchange Eval stuff here
    void updateHeuristics(std::vector<Move>& moveList, int depth);
    void seedMoveOrderer(bool noisyOnly);
    Move pickNextMove(bool noisyOnly) final override;
private:
    bool noisyOnly = false;
    //TODO: Transposition table


    /*
     * The following are moves that are (heuristically) good to check first if the situation arises,
     * as they are likely to produce an alpha beta prune.
     */

    /**
     * Killer moves are refutations that produced beta cutoffs at the same depth in adjacent nodes.
     * These are heuristically good to check, because odds are, a move that refutes moves in sibling positions
     * will also refute moves in our position.
     * Ordered by depth. 
     */
    Move killerOne;
    std::vector<Move> killerHistoryOne;
    Move killerTwo;
    std::vector<Move> killerHistryTwo;
    /**
     * Indexed by [pieceColor][piece][toSquare].
     * Counter moves are refutations to moving a certain piece to a certain square,
     * since usually something that refutes a move like that will repeatedly refute it.
     */
    TripleArray<Move, NumColors, NumPieces, NumSquares> counterMoves;
    Move counter;

    /**
     * Indexed by [pieceColor][piece][toSquare]. 
     * A history of how good moving a piece to a square is as a move in past evaluations,
     * since this is a heuristically good past indicator of how good a move is to be.
     */
    TripleArray<HeuristicScore, NumColors, NumPieces, NumSquares> quietHistory;

    /**
     * Indexed by [aggressor][toSquare][victim].
     * Most Valuable Victim-Least Valuable Aggressor (MVV-LVA) combined with a history-style heuristic.
     * The history heuristic is as described above, we combine this with MVV-LVA,
     * which is a heuristic that says capturing things worth a lot with pieces not worth a lot is a good idea.
     */
    TripleArray<HeuristicScore, NumPieces, NumSquares, NumPieces> captureHistory;
};
#endif