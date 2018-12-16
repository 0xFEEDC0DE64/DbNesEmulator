#pragma once

#include "nescorelib_global.h"
#include "board.h"

class NESCORELIB_EXPORT Namcot106 : public Board
{
public:
    Namcot106(NesEmulator &emu, const Rom &rom);
    virtual ~Namcot106();
};
