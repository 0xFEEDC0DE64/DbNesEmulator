#pragma once

#include "nescorelib_global.h"
#include "board.h"

class NESCORELIB_EXPORT Mmc2 : public Board
{
public:
    explicit Mmc2(NesEmulator &emu, const Rom &rom);
    virtual ~Mmc2();
};
