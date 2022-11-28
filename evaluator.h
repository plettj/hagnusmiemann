#ifndef _EVALUATOR_H
#define _EVALUATOR_H

typedef int CentipawnScore;

class Evaluator {
public:
    Evaluator() = default;
    virtual ~Evaluator() = default;
    virtual CentipawnScore staticEvaluate(const Board& board) = 0;
};

#endif