#include "apunos.h"

// Qt includes
#include <QDataStream>

// local includes
#include "emusettings.h"
#include "apu.h"

ApuNos::ApuNos(Apu &apu) :
    m_apu(apu)
{
}

void ApuNos::apuNosHardReset()
{
    m_apuNosLengthHalt = false;
    m_apuNosConstantVolumeEnvelope = false;
    m_apuNosVolumeDeviderPeriod = 0;
    m_apuNosShiftReg = 1;
    m_apuNosTimer = 0;
    m_apuNosMode = false;
    m_apuNosPeriodDevider = 0;
    m_apuNosLengthEnabled = false;
    m_apuNosLengthCounter = 0;
    m_apuNosEnvelopeStartFlag = false;
    m_apuNosEnvelopeDevider = 0;
    m_apuNosEnvelopeDecayLevelCounter = 0;
    m_apuNosEnvelope = 0;
    m_apuNosOutput = 0;
    m_apuNosFeedback = 0;
    m_apuNosIgnoreReload = false;
}

void ApuNos::apuNosSoftReset()
{
    apuNosHardReset();
}

void ApuNos::apuNosClock()
{
    m_apuNosPeriodDevider--;
    if (m_apuNosPeriodDevider > 0)
        return;

    m_apuNosPeriodDevider = m_apuNosTimer;

    if (m_apuNosMode)
        m_apuNosFeedback = ((m_apuNosShiftReg >> 6) & 0x1) ^ (m_apuNosShiftReg & 0x1);
    else
        m_apuNosFeedback = ((m_apuNosShiftReg >> 1) & 0x1) ^ (m_apuNosShiftReg & 0x1);
    m_apuNosShiftReg >>= 1;
    m_apuNosShiftReg = (m_apuNosShiftReg & 0x3FFF) | ((m_apuNosFeedback & 1) << 14);

    if (m_apuNosLengthCounter > 0 && ((m_apuNosShiftReg & 1) == 0))
    {
        if (EmuSettings::Audio::ChannelEnabled::NOZ)
            m_apuNosOutput = m_apuNosEnvelope;
    }
    else
        m_apuNosOutput = 0;
}

void ApuNos::apuNosClockLength()
{
    if (m_apuNosLengthCounter > 0 && !m_apuNosLengthHalt)
    {
        m_apuNosLengthCounter--;
        if (m_apu.regAccessHappened())
        {
            // This is not a hack, there is some hidden mechanism in the nes, that do reload and clock stuff
            if (m_apu.regIoAddr() == 0xF && m_apu.regAccessW())
            {
                m_apuNosIgnoreReload = true;
            }
        }
    }
}

void ApuNos::apuNosClockEnvelope()
{
    if (m_apuNosEnvelopeStartFlag)
    {
        m_apuNosEnvelopeStartFlag = false;
        m_apuNosEnvelopeDecayLevelCounter = 15;
        m_apuNosEnvelopeDevider = m_apuNosVolumeDeviderPeriod + 1;
    }
    else
    {
        if (m_apuNosEnvelopeDevider > 0)
            m_apuNosEnvelopeDevider--;
        else
        {
            m_apuNosEnvelopeDevider = m_apuNosVolumeDeviderPeriod + 1;
            if (m_apuNosEnvelopeDecayLevelCounter > 0)
                m_apuNosEnvelopeDecayLevelCounter--;
            else if (m_apuNosLengthHalt)
                m_apuNosEnvelopeDecayLevelCounter = 0xF;
        }
    }
    m_apuNosEnvelope = m_apuNosConstantVolumeEnvelope ? m_apuNosVolumeDeviderPeriod : m_apuNosEnvelopeDecayLevelCounter;
}

void ApuNos::apuOnRegister400C()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;
    m_apuNosVolumeDeviderPeriod = m_apu.regIoDb() & 0xF;
    m_apuNosLengthHalt = (m_apu.regIoDb() & 0x20) != 0;
    m_apuNosConstantVolumeEnvelope = m_apu.regIoDb() & 0x10;

    m_apuNosEnvelope = m_apuNosConstantVolumeEnvelope ? m_apuNosVolumeDeviderPeriod : m_apuNosEnvelopeDecayLevelCounter;
}

void ApuNos::apuOnRegister400D()
{
}

void ApuNos::apuOnRegister400E()
{
    static constexpr std::array<qint32, 16> freqTable = [](){
        switch(EmuSettings::region)
        {
        case EmuRegion::NTSC:
            return std::array<qint32, 16> {
                4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
            };
        case EmuRegion::PALB:
            return std::array<qint32, 16> {
                4, 7, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
            };
        case EmuRegion::DENDY:
            return std::array<qint32, 16> {
                4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
            };
        }
    }();

    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    m_apuNosTimer = freqTable[m_apu.regIoDb() & 0x0F] / 2;

    m_apuNosMode = (m_apu.regIoDb() & 0x80) == 0x80;
}

void ApuNos::apuOnRegister400F()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;

    if (m_apuNosLengthEnabled && !m_apuNosIgnoreReload)
        m_apuNosLengthCounter = m_apu.m_sqDurationTable[m_apu.regIoDb() >> 3];
    if (m_apuNosIgnoreReload)
        m_apuNosIgnoreReload = false;
    m_apuNosEnvelopeStartFlag = true;
}

void ApuNos::apuNosOn4015()
{
    m_apuNosLengthEnabled = (m_apu.regIoDb() & 0x08) != 0;
    if (!m_apuNosLengthEnabled)
        m_apuNosLengthCounter = 0;
}

void ApuNos::apuNosRead4015()
{
    if (m_apuNosLengthCounter > 0)
        m_apu.setRegIoDb((m_apu.regIoDb() & 0xF7) | 0x08);
}

void ApuNos::apuNosWriteState(QDataStream &dataStream) const
{
    dataStream
            << m_apuNosLengthHalt
            << m_apuNosConstantVolumeEnvelope
            << m_apuNosVolumeDeviderPeriod
            << m_apuNosTimer
            << m_apuNosMode
            << m_apuNosPeriodDevider
            << m_apuNosLengthEnabled
            << m_apuNosLengthCounter
            << m_apuNosEnvelopeStartFlag
            << m_apuNosEnvelopeDevider
            << m_apuNosEnvelopeDecayLevelCounter
            << m_apuNosEnvelope
            << m_apuNosOutput
            << m_apuNosShiftReg
            << m_apuNosFeedback
            << m_apuNosIgnoreReload;
}

void ApuNos::apuNosReadState(QDataStream &dataStream)
{
    dataStream
            >> m_apuNosLengthHalt
            >> m_apuNosConstantVolumeEnvelope
            >> m_apuNosVolumeDeviderPeriod
            >> m_apuNosTimer
            >> m_apuNosMode
            >> m_apuNosPeriodDevider
            >> m_apuNosLengthEnabled
            >> m_apuNosLengthCounter
            >> m_apuNosEnvelopeStartFlag
            >> m_apuNosEnvelopeDevider
            >> m_apuNosEnvelopeDecayLevelCounter
            >> m_apuNosEnvelope
            >> m_apuNosOutput
            >> m_apuNosShiftReg
            >> m_apuNosFeedback
            >> m_apuNosIgnoreReload;
}

qint32 ApuNos::output() const
{
    return m_apuNosOutput;
}
