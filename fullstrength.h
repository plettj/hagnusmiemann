#ifndef _FULL_STRENGTH_H
#define _FULL_STRENGTH_H
#include "difficultylevel.h"
#include "evaluator.h"
#include <array>
#include <unordered_map>

class FullStrength : public DifficultyLevel {
public:
    FullStrength(bool useLevelThree);
    Move getMove(Board& board) override;
private:
    bool useLevelThree;
    long nodeCount = 0;
    int startingMove = 0;
    /**
     * Some useful constants in our search
     */
    static const CentipawnScore Infinite = 30000;
    static const CentipawnScore NoScore = Infinite + 2;
    static const CentipawnScore Checkmate = Infinite - MaxDepth;

    //These are constants for various search heuristics.
    //They more or less are numbers that I've had in the past when coding this
    //taken from a variety of sources (including out of my ass)
    static const int ReverseFutilityDepth = 8;
    static const int ReverseFutilityMargin = 91;

    static const int RazorMargin = 640;
    static const int LateMovePruningDepth = 9;
    static const int LateMoveReductionDepth = 64;

    static const int FutilityDepth = 7;
    static const int FutilityMargin = 91;
    static const int FutilityMarginAdded = 60;
    static const int FutilityMarginNoHistory = 150;

    static const int SeeDepth = 9;
    static const int SeeNoisyMargin = -20;
    static const int SeeQuietMargin = -70;

    static const int QuiesSeeMargin = 100;

    std::array<CentipawnScore, MaxDepth> pastScores;
    std::array<Move, MaxDepth> principalVariation;
    MultiArray<CentipawnScore, LateMoveReductionDepth, LateMoveReductionDepth> lmrTable;
    MultiArray<CentipawnScore, 2, LateMovePruningDepth> lmpTable;

    std::unordered_map<uint64_t, Move> bestMoves;
    CentipawnScore getDeltaPruningMargin(Board& board);
    CentipawnScore quiescence(Board& board, CentipawnScore alpha, CentipawnScore beta);
    CentipawnScore alphabeta(Board& board, CentipawnScore alpha, CentipawnScore beta, int depth);
};

#endif