#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class NesEmulator;
class QDataStream;

class NESCORELIB_EXPORT Interrupts
{
public:
    explicit Interrupts(NesEmulator &emu);

    void pollStatus();

    void writeState(QDataStream &dataStream) const;
    void readState(QDataStream &dataStream);

    enum IrqFlag {
        IRQ_APU = 1,
        IRQ_DMC = 2,
        IRQ_BOARD = 8
    };

    qint32 flags() const;
    void addFlag(IrqFlag flag);
    void removeFlag(IrqFlag flag);
    void setFlags(qint32 flags);

    quint16 vector() const;

    void setNmiCurrent(bool nmiCurrent);

private:
    NesEmulator &m_emu;

    qint32 m_flags {}; //Determines that IRQ flags (pins)

    bool m_ppuNmiCurrent {}; //Represents the current NMI pin (connected to ppu)
    bool m_ppuNmiOld {}; //Represents the old status if NMI pin, used to generate NMI in raising edge
    quint16 m_vector {};
};
