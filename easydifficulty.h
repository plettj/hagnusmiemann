#ifndef _EASY_DIFFICULTY
#define _EASY_DIFFICULTY

#include "difficultylevel.h"

class LevelOne : public DifficultyLevel {
    LevelOne();
    Move getMove(Board& board) override;
};

class LevelTwo : public DifficultyLevel {
    LevelTwo();
    Move getMove(Board& board) override;
};

#endif