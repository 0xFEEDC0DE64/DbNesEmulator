#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class QDataStream;
class Apu;

class NESCORELIB_EXPORT ApuNos
{
public:
    explicit ApuNos(Apu &apu);

    void apuNosHardReset();
    void apuNosSoftReset();

    void apuNosClock();
    void apuNosClockLength();
    void apuNosClockEnvelope();

    void apuOnRegister400C();
    void apuOnRegister400D();
    void apuOnRegister400E();
    void apuOnRegister400F();

    void apuNosOn4015();
    void apuNosRead4015();

    void apuNosWriteState(QDataStream &dataStream) const;
    void apuNosReadState(QDataStream &dataStream);

    qint32 output() const;

private:
    Apu &m_apu;

    // Reg 1
    bool m_apuNosLengthHalt {};
    bool m_apuNosConstantVolumeEnvelope {};
    quint8 m_apuNosVolumeDeviderPeriod {};

    // Reg 3
    quint16 m_apuNosTimer {};
    bool m_apuNosMode {};

    // Controls
    qint32 m_apuNosPeriodDevider {};
    bool m_apuNosLengthEnabled {};
    qint32 m_apuNosLengthCounter {};
    bool m_apuNosEnvelopeStartFlag {};
    quint8 m_apuNosEnvelopeDevider {};
    quint8 m_apuNosEnvelopeDecayLevelCounter {};
    quint8 m_apuNosEnvelope {};
    qint32 m_apuNosOutput {};
    qint32 m_apuNosShiftReg {};
    qint32 m_apuNosFeedback {};
    bool m_apuNosIgnoreReload {};
};
