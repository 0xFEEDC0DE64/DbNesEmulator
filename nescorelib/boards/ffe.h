#pragma once

#include "nescorelib_global.h"
#include "board.h"

class NESCORELIB_EXPORT Ffe : public Board
{
public:
    explicit Ffe(NesEmulator &emu, const Rom &rom);
    virtual ~Ffe();

    void writeEx(quint16 address, quint8 value) Q_DECL_OVERRIDE;
    void onCpuClock() Q_DECL_OVERRIDE;
    void readState(QDataStream &dataStream) Q_DECL_OVERRIDE;
    void writeState(QDataStream &dataStream) const Q_DECL_OVERRIDE;

private:
    bool m_irqEnable;
    int m_irqCounter;
};
