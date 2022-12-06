#ifndef _EVALUATOR_H
#define _EVALUATOR_H

#include "board.h"
#include "constants.h"
#include <memory>

typedef int CentipawnScore;

class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual CentipawnScore getPieceValue(Piece piece);
    virtual CentipawnScore staticEvaluate(const Board& board) = 0;
    virtual std::unique_ptr<Evaluator> clone() const = 0;
};

class TrivialEvaluator : public Evaluator {
    CentipawnScore staticEvaluate(const Board& board) override {
        return 0;
    }
    std::unique_ptr<Evaluator> clone() const override {
        return std::make_unique<TrivialEvaluator>(*this);
    }
};

class EvalLevelThree : public Evaluator {
public:
    CentipawnScore staticEvaluate(const Board& board) override;
    std::unique_ptr<Evaluator> clone() const override {
        return std::make_unique<EvalLevelThree>(*this);
    }
};

class EvalLevelFour : public Evaluator {
public:
    CentipawnScore staticEvaluate(const Board& board) override;
    std::unique_ptr<Evaluator> clone() const override {
        return std::make_unique<EvalLevelFour>(*this);
    }
private:
    static const CentipawnScore TempoBonus = 20;
    static const CentipawnScore RookOpenFileBonus = 6;
    static const CentipawnScore RookSemiOpenFileBonus = 6;
    static const CentipawnScore QueenOpenFileBonus = 2;
    static const CentipawnScore QueenSemiOpenFileBonus = 3;
    static const CentipawnScore BishopPairBonus = 30;
    static const CentipawnScore IsolatedPawnBonus = -10;
    static const CentipawnScore PassedPawnBouns = 80;
};

#endif
