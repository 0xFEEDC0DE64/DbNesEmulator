#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class QDataStream;
class Apu;

class NESCORELIB_EXPORT ApuDmc
{
public:
    explicit ApuDmc(Apu &apu);

    void apuDmcHardReset();
    void apuDmcSoftReset();

    void apuDmcClock();
    void apuDmcDoDma();

    void apuOnRegister4010();
    void apuOnRegister4011();
    void apuOnRegister4012();
    void apuOnRegister4013();

    void apuDmcOn4015();
    void apuDmcRead4015();

    void apuDmcWriteState(QDataStream &dataStream) const;
    void apuDmcReadState(QDataStream &dataStream);

    qint32 output() const;

private:
    Apu &m_apu;

    qint32 m_apuDmcOutputA {};
    qint32 m_apuDmcOutput {};
    qint32 m_apuDmcPeriodDevider {};
    bool m_apuDmcIrqEnabled {};
    bool m_apuDmcLoopFlag {};
    quint8 m_apuDmcRateIndex {};
    quint16 m_apuDmcAddrRefresh {};
    qint32 m_apuDmcSizeRefresh {};

    bool m_apuDmcDmaEnabled {};
    quint8 m_apuDmcDmaByte {};
    qint32 m_apuDmcDmaBits {};
    bool m_apuDmcBufferFull {};
    quint8 m_apuDmcDmaBuffer {};
    qint32 m_apuDmcDmaSize {};
    quint16 m_apuDmcDmaAddr {};
};
