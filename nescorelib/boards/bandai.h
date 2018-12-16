#pragma once

#include "nescorelib_global.h"
#include "board.h"

class NESCORELIB_EXPORT Bandai : public Board
{
public:
    explicit Bandai(NesEmulator &emu, const Rom &rom);
    virtual ~Bandai();
};
