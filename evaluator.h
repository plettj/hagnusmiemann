#ifndef _EVALUATOR_H
#define _EVALUATOR_H

#include "board.h"

typedef int CentipawnScore;

class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual CentipawnScore staticEvaluate(const Board& board) = 0;
};

#endif