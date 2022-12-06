#ifndef _EVALUATOR_H
#define _EVALUATOR_H

#include "board.h"
#include "constants.h"
#include <memory>

typedef int CentipawnScore;

class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual CentipawnScore getPieceValue(Piece piece) = 0;
    virtual CentipawnScore staticEvaluate(const Board& board) = 0;
    virtual std::unique_ptr<Evaluator> clone() const = 0;
};

class TrivialEvaluator : public Evaluator {
    CentipawnScore staticEvaluate(const Board& board) override {
        return 0;
    }
    CentipawnScore getPieceValue(Piece piece) {
        return 0;
    }
    std::unique_ptr<Evaluator> clone() const override {
        return std::make_unique<TrivialEvaluator>(*this);
    }
};

class EvalLevelThree : public Evaluator {
public:
    CentipawnScore getPieceValue(Piece piece) override;
    CentipawnScore staticEvaluate(const Board& board) override;
    std::unique_ptr<Evaluator> clone() const override {
        return std::make_unique<EvalLevelThree>(*this);
    }
};

#endif
