#include "apusq1.h"

// Qt includes
#include <QDataStream>

// local includes
#include "emusettings.h"
#include "apu.h"

ApuSq1::ApuSq1(Apu &apu) :
    m_apu(apu)
{
}

void ApuSq1::apuSq1HardReset()
{
    m_apuSq1DutyCycle = 0;
    m_apuSq1LengthHalt = false;
    m_apuSq1ConstantVolumeEnvelope = false;
    m_apuSq1VolumeDeviderPeriod = 0;
    m_apuSq1SweepEnable = false;
    m_apuSq1SweepDeviderPeriod = 0;
    m_apuSq1SweepNegate = false;
    m_apuSq1SweepShiftCount = 0;
    m_apuSq1Timer = 0;
    m_apuSq1PeriodDevider = 0;
    m_apuSq1Seqencer = 0;
    m_apuSq1LengthEnabled = false;
    m_apuSq1LengthCounter = 0;
    m_apuSq1EnvelopeStartFlag = false;
    m_apuSq1EnvelopeDevider = 0;
    m_apuSq1EnvelopeDecayLevelCounter = 0;
    m_apuSq1Envelope = 0;
    m_apuSq1SweepCounter = 0;
    m_apuSq1SweepReload = false;
    m_apuSq1SweepChange = 0;
    m_apuSq1ValidFreq = false;
    m_apuSq1Output = 0;
    m_apuSq1IgnoreReload = false;
}

void ApuSq1::apuSq1SoftReset()
{
    apuSq1HardReset();
}

void ApuSq1::apuSq1Clock()
{
    m_apuSq1PeriodDevider--;
    if (m_apuSq1PeriodDevider <= 0)
    {
        m_apuSq1PeriodDevider = m_apuSq1Timer + 1;
        m_apuSq1Seqencer = (m_apuSq1Seqencer + 1) & 0x7;
        if (m_apuSq1LengthCounter > 0 && m_apuSq1ValidFreq)
        {
            if (EmuSettings::Audio::ChannelEnabled::SQ1)
                m_apuSq1Output = m_apu.m_sqDutyCycleSequences[m_apuSq1DutyCycle][m_apuSq1Seqencer] * m_apuSq1Envelope;
        }
        else
            m_apuSq1Output = 0;
    }
}

void ApuSq1::apuSq1ClockLength()
{
    if (m_apuSq1LengthCounter > 0 && !m_apuSq1LengthHalt)
    {
        m_apuSq1LengthCounter--;
        if (m_apu.regAccessHappened())
        {
            // This is not a hack, there is some hidden mechanism in the nes, that do reload and clock stuff
            if (m_apu.regIoAddr() == 0x3 && m_apu.regAccessW())
            {
                m_apuSq1IgnoreReload = true;
            }
        }
    }

    // Sweep unit
    m_apuSq1SweepCounter--;
    if (m_apuSq1SweepCounter == 0)
    {
        m_apuSq1SweepCounter = m_apuSq1SweepDeviderPeriod + 1;
        if (m_apuSq1SweepEnable && m_apuSq1SweepShiftCount > 0 && m_apuSq1ValidFreq)
        {
            m_apuSq1SweepChange = m_apuSq1Timer >> m_apuSq1SweepShiftCount;
            m_apuSq1Timer += m_apuSq1SweepNegate ? ~m_apuSq1SweepChange : m_apuSq1SweepChange;
            apuSq1CalculateValidFreq();
        }
    }

    if (m_apuSq1SweepReload)
    {
        m_apuSq1SweepCounter = m_apuSq1SweepDeviderPeriod + 1;
        m_apuSq1SweepReload = false;
    }
}

void ApuSq1::apuSq1ClockEnvelope()
{
    if (m_apuSq1EnvelopeStartFlag)
    {
        m_apuSq1EnvelopeStartFlag = false;
        m_apuSq1EnvelopeDecayLevelCounter = 15;
        m_apuSq1EnvelopeDevider = m_apuSq1VolumeDeviderPeriod + 1;
    }
    else
    {
        if (m_apuSq1EnvelopeDevider > 0)
            m_apuSq1EnvelopeDevider--;
        else
        {
            m_apuSq1EnvelopeDevider = m_apuSq1VolumeDeviderPeriod + 1;
            if (m_apuSq1EnvelopeDecayLevelCounter > 0)
                m_apuSq1EnvelopeDecayLevelCounter--;
            else if (m_apuSq1LengthHalt)
                m_apuSq1EnvelopeDecayLevelCounter = 0xF;
        }
    }
    m_apuSq1Envelope = m_apuSq1ConstantVolumeEnvelope ? m_apuSq1VolumeDeviderPeriod : m_apuSq1EnvelopeDecayLevelCounter;
}

void ApuSq1::apuOnRegister4000()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;
    m_apuSq1DutyCycle = (m_apu.regIoDb() & 0xC0) >> 6;
    m_apuSq1VolumeDeviderPeriod = m_apu.regIoDb() & 0xF;
    m_apuSq1LengthHalt = m_apu.regIoDb() & 0x20;
    m_apuSq1ConstantVolumeEnvelope = m_apu.regIoDb() & 0x10;

    m_apuSq1Envelope = m_apuSq1ConstantVolumeEnvelope ? m_apuSq1VolumeDeviderPeriod : m_apuSq1EnvelopeDecayLevelCounter;
}

void ApuSq1::apuOnRegister4001()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq1SweepEnable = (m_apu.regIoDb() & 0x80) == 0x80;
    m_apuSq1SweepDeviderPeriod = (m_apu.regIoDb() >> 4) & 7;
    m_apuSq1SweepNegate = (m_apu.regIoDb() & 0x8) == 0x8;
    m_apuSq1SweepShiftCount = m_apu.regIoDb() & 7;
    m_apuSq1SweepReload = true;
    apuSq1CalculateValidFreq();
}

void ApuSq1::apuOnRegister4002()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq1Timer = (m_apuSq1Timer & 0xFF00) | m_apu.regIoDb();
    apuSq1CalculateValidFreq();
}

void ApuSq1::apuOnRegister4003()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuSq1Timer = (m_apuSq1Timer & 0x00FF) | ((m_apu.regIoDb() & 0x7) << 8);

    if (m_apuSq1LengthEnabled && !m_apuSq1IgnoreReload)
    {
        m_apuSq1LengthCounter = m_apu.m_sqDurationTable[m_apu.regIoDb() >> 3];
    }

    m_apuSq1IgnoreReload = false;

    m_apuSq1Seqencer = 0;
    m_apuSq1EnvelopeStartFlag = true;
    apuSq1CalculateValidFreq();
}

void ApuSq1::apuSq1On4015()
{
    m_apuSq1LengthEnabled = m_apu.regIoDb() & 0x01;
    if (!m_apuSq1LengthEnabled)
        m_apuSq1LengthCounter = 0;
}

void ApuSq1::apuSq1Read4015()
{
    if (m_apuSq1LengthCounter > 0)
        m_apu.setRegIoDb((m_apu.regIoDb() & 0xFE) | 0x01);
}

void ApuSq1::apuSq1CalculateValidFreq()
{
    m_apuSq1ValidFreq = (m_apuSq1Timer >= 0x8) && ((m_apuSq1SweepNegate) || (((m_apuSq1Timer + (m_apuSq1Timer >> m_apuSq1SweepShiftCount)) & 0x800) == 0));
}

void ApuSq1::apuSq1WriteState(QDataStream &dataStream) const
{
    dataStream << m_apuSq1DutyCycle << m_apuSq1LengthHalt << m_apuSq1ConstantVolumeEnvelope << m_apuSq1VolumeDeviderPeriod
               << m_apuSq1SweepEnable << m_apuSq1SweepDeviderPeriod << m_apuSq1SweepNegate << m_apuSq1SweepShiftCount << m_apuSq1Timer
               << m_apuSq1PeriodDevider << m_apuSq1Seqencer << m_apuSq1LengthEnabled << m_apuSq1LengthCounter << m_apuSq1EnvelopeStartFlag
               << m_apuSq1EnvelopeDevider << m_apuSq1EnvelopeDecayLevelCounter << m_apuSq1Envelope << m_apuSq1SweepCounter
               << m_apuSq1SweepReload << m_apuSq1SweepChange << m_apuSq1ValidFreq << m_apuSq1Output << m_apuSq1IgnoreReload;
}

void ApuSq1::apuSq1ReadState(QDataStream &dataStream)
{
    dataStream >> m_apuSq1DutyCycle >> m_apuSq1LengthHalt >> m_apuSq1ConstantVolumeEnvelope >> m_apuSq1VolumeDeviderPeriod
               >> m_apuSq1SweepEnable >> m_apuSq1SweepDeviderPeriod >> m_apuSq1SweepNegate >> m_apuSq1SweepShiftCount >> m_apuSq1Timer
               >> m_apuSq1PeriodDevider >> m_apuSq1Seqencer >> m_apuSq1LengthEnabled >> m_apuSq1LengthCounter >> m_apuSq1EnvelopeStartFlag
               >> m_apuSq1EnvelopeDevider >> m_apuSq1EnvelopeDecayLevelCounter >> m_apuSq1Envelope >> m_apuSq1SweepCounter
               >> m_apuSq1SweepReload >> m_apuSq1SweepChange >> m_apuSq1ValidFreq >> m_apuSq1Output >> m_apuSq1IgnoreReload;
}

qint32 ApuSq1::output() const
{
    return m_apuSq1Output;
}
