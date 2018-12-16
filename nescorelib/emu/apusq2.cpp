#include "apusq2.h"

// Qt includes
#include <QDataStream>

// local includes
#include "emusettings.h"
#include "apu.h"

ApuSq2::ApuSq2(Apu &apu) :
    m_apu(apu)
{
}

void ApuSq2::apuSq2HardReset()
{
    m_apuSq2DutyCycle = 0;
    m_apuSq2LengthHalt = false;
    m_apuSq2ConstantVolumeEnvelope = false;
    m_apuSq2VolumeDeviderPeriod = 0;
    m_apuSq2SweepEnable = false;
    m_apuSq2SweepDeviderPeriod = 0;
    m_apuSq2SweepNegate = false;
    m_apuSq2SweepShiftCount = 0;
    m_apuSq2Timer = 0;
    m_apuSq2PeriodDevider = 0;
    m_apuSq2Seqencer = 0;
    m_apuSq2LengthEnabled = false;
    m_apuSq2LengthCounter = 0;
    m_apuSq2EnvelopeStartFlag = false;
    m_apuSq2EnvelopeDevider = 0;
    m_apuSq2EnvelopeDecayLevelCounter = 0;
    m_apuSq2Envelope = 0;
    m_apuSq2SweepCounter = 0;
    m_apuSq2SweepReload = false;
    m_apuSq2SweepChange = 0;
    m_apuSq2ValidFreq = false;
    m_apuSq2Output = 0;
    m_apuSq2IgnoreReload = false;
}

void ApuSq2::apuSq2SoftReset()
{
    apuSq2HardReset();
}

void ApuSq2::apuSq2Clock()
{
    m_apuSq2PeriodDevider--;
    if (m_apuSq2PeriodDevider <= 0)
    {
        m_apuSq2PeriodDevider = m_apuSq2Timer + 1;
        m_apuSq2Seqencer = (m_apuSq2Seqencer + 1) & 0x7;
        if (m_apuSq2LengthCounter > 0 && m_apuSq2ValidFreq)
        {
            if (EmuSettings::Audio::ChannelEnabled::SQ2)
                m_apuSq2Output = m_apu.m_sqDutyCycleSequences[m_apuSq2DutyCycle][m_apuSq2Seqencer] * m_apuSq2Envelope;
        }
        else
            m_apuSq2Output = 0;
    }
}

void ApuSq2::apuSq2ClockLength()
{
    if (m_apuSq2LengthCounter > 0 && !m_apuSq2LengthHalt)
    {
        m_apuSq2LengthCounter--;
        if (m_apu.regAccessHappened())
        {
            // This is not a hack, there is some hidden mechanism in the nes, that do reload and clock stuff
            if (m_apu.regIoAddr() == 0x7 && m_apu.regAccessW())
                m_apuSq2IgnoreReload = true;
        }
    }

    // Sweep unit
    m_apuSq2SweepCounter--;
    if (m_apuSq2SweepCounter == 0)
    {
        m_apuSq2SweepCounter = m_apuSq2SweepDeviderPeriod + 1;
        if (m_apuSq2SweepEnable && m_apuSq2SweepShiftCount > 0 && m_apuSq2ValidFreq)
        {
            m_apuSq2SweepChange = m_apuSq2Timer >> m_apuSq2SweepShiftCount;
            m_apuSq2Timer += m_apuSq2SweepNegate ? -m_apuSq2SweepChange : m_apuSq2SweepChange;
            apuSq2CalculateValidFreq();
        }
    }
    else if (m_apuSq2SweepReload)
    {
        m_apuSq2SweepCounter = m_apuSq2SweepDeviderPeriod + 1;
        m_apuSq2SweepReload = false;
    }
}

void ApuSq2::apuSq2ClockEnvelope()
{
    if (m_apuSq2EnvelopeStartFlag)
    {
        m_apuSq2EnvelopeStartFlag = false;
        m_apuSq2EnvelopeDecayLevelCounter = 15;
        m_apuSq2EnvelopeDevider = m_apuSq2VolumeDeviderPeriod + 1;
    }
    else
    {
        if (m_apuSq2EnvelopeDevider > 0)
            m_apuSq2EnvelopeDevider--;
        else
        {
            m_apuSq2EnvelopeDevider = m_apuSq2VolumeDeviderPeriod + 1;
            if (m_apuSq2EnvelopeDecayLevelCounter > 0)
                m_apuSq2EnvelopeDecayLevelCounter--;
            else if (m_apuSq2LengthHalt)
                m_apuSq2EnvelopeDecayLevelCounter = 0xF;
        }
    }

    m_apuSq2Envelope = m_apuSq2ConstantVolumeEnvelope ? m_apuSq2VolumeDeviderPeriod : m_apuSq2EnvelopeDecayLevelCounter;
}

void ApuSq2::apuOnRegister4004()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq2DutyCycle = (m_apu.regIoDb() & 0xC0) >> 6;
    m_apuSq2VolumeDeviderPeriod = m_apu.regIoDb() & 0xF;
    m_apuSq2LengthHalt = m_apu.regIoDb() & 0x20;
    m_apuSq2ConstantVolumeEnvelope = m_apu.regIoDb() & 0x10;

    m_apuSq2Envelope = m_apuSq2ConstantVolumeEnvelope ? m_apuSq2VolumeDeviderPeriod : m_apuSq2EnvelopeDecayLevelCounter;
}

void ApuSq2::apuOnRegister4005()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq2SweepEnable = (m_apu.regIoDb() & 0x80) == 0x80;
    m_apuSq2SweepDeviderPeriod = (m_apu.regIoDb() >> 4) & 7;
    m_apuSq2SweepNegate = (m_apu.regIoDb() & 0x8) == 0x8;
    m_apuSq2SweepShiftCount = m_apu.regIoDb() & 7;
    m_apuSq2SweepReload = true;
    apuSq2CalculateValidFreq();
}

void ApuSq2::apuOnRegister4006()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq2Timer = (m_apuSq2Timer & 0xFF00) | m_apu.regIoDb();
    apuSq2CalculateValidFreq();
}

void ApuSq2::apuOnRegister4007()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq2Timer = (m_apuSq2Timer & 0x00FF) | ((m_apu.regIoDb() & 0x7) << 8);

    if (m_apuSq2LengthEnabled && !m_apuSq2IgnoreReload)
        m_apuSq2LengthCounter = m_apu.m_sqDurationTable[m_apu.regIoDb() >> 3];
    if (m_apuSq2IgnoreReload)
        m_apuSq2IgnoreReload = false;

    m_apuSq2Seqencer = 0;
    m_apuSq2EnvelopeStartFlag = true;
    apuSq2CalculateValidFreq();
}

void ApuSq2::apuSq2On4015()
{
    m_apuSq2LengthEnabled = (m_apu.regIoDb() & 0x02) != 0;
    if (!m_apuSq2LengthEnabled)
        m_apuSq2LengthCounter = 0;
}

void ApuSq2::apuSq2Read4015()
{
    if (m_apuSq2LengthCounter > 0)
        m_apu.setRegIoDb((m_apu.regIoDb() & 0xFD) | 0x02);
}

void ApuSq2::apuSq2CalculateValidFreq()
{
    m_apuSq2ValidFreq = (m_apuSq2Timer >= 0x8) && ((m_apuSq2SweepNegate) || (((m_apuSq2Timer + (m_apuSq2Timer >> m_apuSq2SweepShiftCount)) & 0x800) == 0));
}

void ApuSq2::apuSq2WriteState(QDataStream &dataStream) const
{
    dataStream
            << m_apuSq2DutyCycle << m_apuSq2LengthHalt << m_apuSq2ConstantVolumeEnvelope << m_apuSq2VolumeDeviderPeriod << m_apuSq2SweepEnable
            << m_apuSq2SweepDeviderPeriod << m_apuSq2SweepNegate << m_apuSq2SweepShiftCount << m_apuSq2Timer << m_apuSq2PeriodDevider << m_apuSq2Seqencer
            << m_apuSq2LengthEnabled << m_apuSq2LengthCounter << m_apuSq2EnvelopeStartFlag << m_apuSq2EnvelopeDevider << m_apuSq2EnvelopeDecayLevelCounter
            << m_apuSq2Envelope << m_apuSq2SweepCounter << m_apuSq2SweepReload << m_apuSq2SweepChange << m_apuSq2ValidFreq << m_apuSq2Output << m_apuSq2IgnoreReload;
}

void ApuSq2::apuSq2ReadState(QDataStream &dataStream)
{
    dataStream
            >> m_apuSq2DutyCycle >> m_apuSq2LengthHalt >> m_apuSq2ConstantVolumeEnvelope >> m_apuSq2VolumeDeviderPeriod >> m_apuSq2SweepEnable
            >> m_apuSq2SweepDeviderPeriod >> m_apuSq2SweepNegate >> m_apuSq2SweepShiftCount >> m_apuSq2Timer >> m_apuSq2PeriodDevider >> m_apuSq2Seqencer
            >> m_apuSq2LengthEnabled >> m_apuSq2LengthCounter >> m_apuSq2EnvelopeStartFlag >> m_apuSq2EnvelopeDevider >> m_apuSq2EnvelopeDecayLevelCounter
            >> m_apuSq2Envelope >> m_apuSq2SweepCounter >> m_apuSq2SweepReload >> m_apuSq2SweepChange >> m_apuSq2ValidFreq >> m_apuSq2Output >> m_apuSq2IgnoreReload;
}

qint32 ApuSq2::output() const
{
    return m_apuSq2Output;
}
