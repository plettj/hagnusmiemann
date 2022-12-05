#ifndef _EASY_DIFFICULTY
#define _EASY_DIFFICULTY

#include "difficultylevel.h"

class LevelOne : public DifficultyLevel {
public:    
    LevelOne();
    Move getMove(Board& board) override;
};

class LevelTwo : public DifficultyLevel {
public:    
    LevelTwo();
    Move getMove(Board& board) override;
};

#endif