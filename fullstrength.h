#ifndef _FULL_STRENGTH_H
#define _FULL_STRENGTH_H
#include "difficultylevel.h"


class FullStrength : public DifficultyLevel {
public:
    FullStrength();
    Move getMove(Board& board) override;
private:

};

#endif