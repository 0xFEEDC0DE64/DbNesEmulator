#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class QDataStream;
class Apu;

class NESCORELIB_EXPORT ApuTrl
{
public:
    explicit ApuTrl(Apu &apu);

    void apuTrlHardReset();
    void apuTrlSoftReset();

    void apuTrlClock();
    void apuTrlClockLength();
    void apuTrlClockEnvelope();

    void apuOnRegister4008();
    void apuOnRegister4009();
    void apuOnRegister400A();
    void apuOnRegister400B();

    void apuTrlOn4015();
    void apuTrlRead4015();

    void apuTrlWriteState(QDataStream &dataStream) const;
    void apuTrlReadState(QDataStream &dataStream);

    qint32 output() const;

private:
    Apu &m_apu;

    // Reg1
    bool m_apuTrlLinerControlFlag {};
    quint8 m_apuTrlLinerControlReload {};
    // Reg2
    quint16 m_apuTrlTimer {};

    bool m_apuTrlLengthEnabled {};
    quint8 m_apuTrlLengthCounter {};
    bool m_apuTrlLinerControlReloadFlag {};
    quint8 m_apuTrlLinerCounter {};
    qint32 m_apuTrlOutput {};
    qint32 m_apuTrlPeriodDevider {};
    qint32 m_apuTrlStep {};
    bool m_apuTrlIgnoreReload {};
};
