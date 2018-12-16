#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class QDataStream;
class Apu;

class NESCORELIB_EXPORT ApuSq1
{
public:
    explicit ApuSq1(Apu &apu);

    void apuSq1HardReset();
    void apuSq1SoftReset();

    void apuSq1Clock();
    void apuSq1ClockLength();
    void apuSq1ClockEnvelope();

    void apuOnRegister4000();
    void apuOnRegister4001();
    void apuOnRegister4002();
    void apuOnRegister4003();

    void apuSq1On4015();
    void apuSq1Read4015();

    void apuSq1CalculateValidFreq();

    void apuSq1WriteState(QDataStream &dataStream) const;
    void apuSq1ReadState(QDataStream &dataStream);

    qint32 output() const;

private:
    Apu &m_apu;

    // Reg 1
    quint8 m_apuSq1DutyCycle {};
    bool m_apuSq1LengthHalt {};
    bool m_apuSq1ConstantVolumeEnvelope {};
    quint8 m_apuSq1VolumeDeviderPeriod {};
    // Reg 2
    bool m_apuSq1SweepEnable {};
    quint8 m_apuSq1SweepDeviderPeriod {};
    bool m_apuSq1SweepNegate {};
    quint8 m_apuSq1SweepShiftCount {};
    // Reg 3
    qint32 m_apuSq1Timer {};

    // Controls
    qint32 m_apuSq1PeriodDevider {};
    quint8 m_apuSq1Seqencer {};
    bool m_apuSq1LengthEnabled {};
    qint32 m_apuSq1LengthCounter {};
    bool m_apuSq1EnvelopeStartFlag {};
    quint8 m_apuSq1EnvelopeDevider {};
    quint8 m_apuSq1EnvelopeDecayLevelCounter {};
    quint8 m_apuSq1Envelope {};
    qint32 m_apuSq1SweepCounter {};
    bool m_apuSq1SweepReload {};
    qint32 m_apuSq1SweepChange {};
    bool m_apuSq1ValidFreq {};
    qint32 m_apuSq1Output {};
    bool m_apuSq1IgnoreReload {};
};
