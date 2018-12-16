#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class NesEmulator;
class QDataStream;

class NESCORELIB_EXPORT Dma
{
public:
    explicit Dma(NesEmulator &emu);

    void hardReset();
    void softReset();

    void assertDmcDma();
    void assertOamDma();

    void clock();

    void writeState(QDataStream &dataStream) const;
    void readState(QDataStream &dataStream);

    void setOamAddress(quint16 oamAddress);

private:
    NesEmulator &m_emu;

    qint32 m_dmcDmaWaitCycles {};
    qint32 m_oamDmaWaitCycles {};
    bool m_isOamDma {};
    bool m_dmcOn {};
    bool m_oamOn {};
    bool m_dmcOccurring {};
    bool m_oamOccurring {};
    qint32 m_oamFinishCounter {};
    quint16 m_oamAddress {};
    qint32 m_oamCycle {};
    quint8 m_latch {};
};
