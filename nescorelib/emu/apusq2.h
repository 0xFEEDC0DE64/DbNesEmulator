#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class QDataStream;
class Apu;

class NESCORELIB_EXPORT ApuSq2
{
public:
    explicit ApuSq2(Apu &apu);

    void apuSq2HardReset();
    void apuSq2SoftReset();

    void apuSq2Clock();
    void apuSq2ClockLength();
    void apuSq2ClockEnvelope();

    void apuOnRegister4004();
    void apuOnRegister4005();
    void apuOnRegister4006();
    void apuOnRegister4007();

    void apuSq2On4015();
    void apuSq2Read4015();

    void apuSq2CalculateValidFreq();

    void apuSq2WriteState(QDataStream &dataStream) const;
    void apuSq2ReadState(QDataStream &dataStream);

    qint32 output() const;

private:
    Apu &m_apu;

    // Reg 1
    quint8 m_apuSq2DutyCycle {};
    bool m_apuSq2LengthHalt {};
    bool m_apuSq2ConstantVolumeEnvelope {};
    quint8 m_apuSq2VolumeDeviderPeriod {};
    // Reg 2
    bool m_apuSq2SweepEnable {};
    quint8 m_apuSq2SweepDeviderPeriod {};
    bool m_apuSq2SweepNegate {};
    quint8 m_apuSq2SweepShiftCount {};
    // Reg 3
    qint32 m_apuSq2Timer {};

    // Controls
    qint32 m_apuSq2PeriodDevider {};
    quint8 m_apuSq2Seqencer {};
    bool m_apuSq2LengthEnabled {};
    qint32 m_apuSq2LengthCounter {};
    bool m_apuSq2EnvelopeStartFlag {};
    quint8 m_apuSq2EnvelopeDevider {};
    quint8 m_apuSq2EnvelopeDecayLevelCounter {};
    quint8 m_apuSq2Envelope {};
    qint32 m_apuSq2SweepCounter {};
    bool m_apuSq2SweepReload {};
    qint32 m_apuSq2SweepChange {};
    bool m_apuSq2ValidFreq {};
    qint32 m_apuSq2Output {};
    bool m_apuSq2IgnoreReload {};
};
