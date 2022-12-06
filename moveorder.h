#ifndef _MOVE_ORDER_H
#define _MOVE_ORDER_H

#include "board.h"
#include "move.h"
#include "evaluator.h"
#include <unordered_map>
#include <random>
#include <memory>

typedef int HeuristicScore;

/**
 * Abstract base class for move ordering, letting us have difficulty levels with simpler move orderers
 * that we can use with polymorphism.
 * The point of move ordering is to order the moves we give back to the search, ordered heuristically
 * such that it is more likely to cause an alpha beta prune.  
 */
class MoveOrderer {
public:    
    MoveOrderer();
    virtual void seedMoveOrderer(Board& board, bool noisyOnly) = 0;
    virtual Move pickNextMove(bool noisyOnly) = 0;
    virtual ~MoveOrderer() {}
    virtual std::unique_ptr<MoveOrderer> clone() const = 0;
protected:
    Board* board = nullptr;
    bool tacticalSearch = false;
    std::vector<Move> moveList;
    int size = 0;
};

/**
 * Just gives back a random move for dumb opponents
 */
class RandomMoveOrderer : public MoveOrderer {
public:    
    RandomMoveOrderer();
    RandomMoveOrderer(const RandomMoveOrderer& other) = default;
    void seedMoveOrderer(Board& board, bool noisyOnly) override;
    Move pickNextMove(bool noisyOnly) override;
    std::unique_ptr<MoveOrderer> clone() const override;

private:
    int size = 0;
    std::mt19937 rng;
};

/**
 * The main move orderer that does non trivial useful things. 
 */
class HeuristicMoveOrderer : public MoveOrderer {
public:    
    HeuristicMoveOrderer();
    HeuristicMoveOrderer(const HeuristicMoveOrderer& other) = default;
    /**
     * Static exchange evaluation looks at the possible trades on a square
     * and determines if one side can clearly materially win it without having to actually
     * calculate further. For instance, if a pawn takes a queen, then that side will always win the trade
     * because they can choose to stop and just be up a queen for a pawn. 
     * Returns whether the side that plays the move wins the trade or not.
     */
    static bool staticExchangeEvaluation(Board& board, const Move& move, CentipawnScore margin);
    static void setSeeMarginInOrdering(CentipawnScore margin);

    static void init();
    static void updateQuietHeuristics(const Board& board, std::vector<Move>& moveList, int depth);
    static void updateNoisyHeuristics(const Board& board, std::vector<Move>& moveList, Move& best, int depth);
    void seedMoveOrderer(Board& board, bool tacticalSearch) final override;
    Move pickNextMove(bool noisyOnly) final override;
    std::unique_ptr<MoveOrderer> clone() const override;

    static HeuristicScore getNoisyHeuristic(const Board& board, const Move& move);
    static HeuristicScore getQuietHeuristic(const Board& board, const Move& move);
    bool isAtQuiets();
private:
    std::unordered_map<Move, HeuristicScore> currentMoveScores;
    enum Stage {
        GenerateNoisy = 0, GoodNoisy, KillerOne, KillerTwo, Counter, GenerateQuiet, Quiet, BadNoisy
    };
    Stage currentStage;

    Move popBestMove(int beginRange, int endRange);
    Move popFirstMove();
    static HeuristicScore getNewHistoryValue(HeuristicScore oldValue, int depth, bool positiveBonus);

    int noisySize;
    int quietSize;
    //TODO: Transposition table

    /*
     * The following are moves that are (heuristically) good to check first if the situation arises,
     * as they are likely to produce an alpha beta prune (see explanation in .cc file)
     */
    Move killerOne;
    Move killerTwo;
    Move counter;

    //This normalizes the scores to be centered approximately around 0
    static const HeuristicScore NormalizationConstant = 66666;
};
#endif