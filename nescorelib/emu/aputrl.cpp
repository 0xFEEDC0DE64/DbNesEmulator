#include "aputrl.h"

// Qt includes
#include <QDataStream>

// local includes
#include "emusettings.h"
#include "apu.h"

ApuTrl::ApuTrl(Apu &apu) :
    m_apu(apu)
{
}

void ApuTrl::apuTrlHardReset()
{
    m_apuTrlLinerControlFlag = false;
    m_apuTrlLinerControlReload = 0;
    m_apuTrlTimer = 0;
    m_apuTrlLengthEnabled = false;
    m_apuTrlLengthCounter = 0;
    m_apuTrlLinerControlReloadFlag = false;
    m_apuTrlLinerCounter = 0;
    m_apuTrlOutput = 0;
    m_apuTrlPeriodDevider = 0;
    m_apuTrlStep = 0;
    m_apuTrlIgnoreReload = false;
}

void ApuTrl::apuTrlSoftReset()
{
    apuTrlHardReset();
}

void ApuTrl::apuTrlClock()
{
    static constexpr std::array<quint8, 32> stepSeq {
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    };

    m_apuTrlPeriodDevider--;
    if (m_apuTrlPeriodDevider <= 0)
    {
        m_apuTrlPeriodDevider = m_apuTrlTimer + 1;

        if (m_apuTrlLengthCounter > 0 && m_apuTrlLinerCounter > 0)
        {
            if (m_apuTrlTimer >= 4)
            {
                m_apuTrlStep++;
                m_apuTrlStep &= 0x1F;
                if (EmuSettings::Audio::ChannelEnabled::TRL)
                    m_apuTrlOutput = stepSeq[m_apuTrlStep];
            }
        }
    }
}

void ApuTrl::apuTrlClockLength()
{
    if (m_apuTrlLengthCounter > 0 && !m_apuTrlLinerControlFlag)
    {
        m_apuTrlLengthCounter--;
        if (m_apu.regAccessHappened())
        {
            // This is not a hack, there is some hidden mechanism in the nes, that do reload and clock stuff
            if (m_apu.regIoAddr() == 0xB && m_apu.regAccessW())
            {
                m_apuTrlIgnoreReload = true;
            }
        }
    }
}

void ApuTrl::apuTrlClockEnvelope()
{
    if (m_apuTrlLinerControlReloadFlag)
    {
        m_apuTrlLinerCounter = m_apuTrlLinerControlReload;
    }
    else
    {
        if (m_apuTrlLinerCounter > 0)
            m_apuTrlLinerCounter--;
    }
    if (!m_apuTrlLinerControlFlag)
        m_apuTrlLinerControlReloadFlag = false;
}

void ApuTrl::apuOnRegister4008()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;
    m_apuTrlLinerControlFlag = (m_apu.regIoDb() & 0x80) == 0x80;
    m_apuTrlLinerControlReload = m_apu.regIoDb() & 0x7F;
}

void ApuTrl::apuOnRegister4009()
{
}

void ApuTrl::apuOnRegister400A()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;
    m_apuTrlTimer = (m_apuTrlTimer & 0x7F00) | m_apu.regIoDb();
}

void ApuTrl::apuOnRegister400B()
{
    // Only writes accepted
    if (!m_apu.regAccessW())
        return;
    m_apuTrlTimer = (m_apuTrlTimer & 0x00FF) | (m_apu.regIoDb() & 0x7) << 8;

    if (m_apuTrlLengthEnabled && !m_apuTrlIgnoreReload)
        m_apuTrlLengthCounter = m_apu.m_sqDurationTable[m_apu.regIoDb() >> 3];
    if (m_apuTrlIgnoreReload)
        m_apuTrlIgnoreReload = false;
    m_apuTrlLinerControlReloadFlag = true;
}

void ApuTrl::apuTrlOn4015()
{
    m_apuTrlLengthEnabled = (m_apu.regIoDb() & 0x04) != 0;
    if (!m_apuTrlLengthEnabled)
        m_apuTrlLengthCounter = 0;
}

void ApuTrl::apuTrlRead4015()
{
    if (m_apuTrlLengthCounter > 0)
        m_apu.setRegIoDb((m_apu.regIoDb() & 0xFB) | 0x04);
}

void ApuTrl::apuTrlWriteState(QDataStream &dataStream) const
{
    dataStream
            << m_apuTrlLinerControlFlag
            << m_apuTrlLinerControlReload
            << m_apuTrlTimer
            << m_apuTrlLengthEnabled
            << m_apuTrlLengthCounter
            << m_apuTrlLinerControlReloadFlag
            << m_apuTrlLinerCounter
            << m_apuTrlOutput
            << m_apuTrlPeriodDevider
            << m_apuTrlStep
            << m_apuTrlIgnoreReload;
}

void ApuTrl::apuTrlReadState(QDataStream &dataStream)
{
    dataStream
            >> m_apuTrlLinerControlFlag
            >> m_apuTrlLinerControlReload
            >> m_apuTrlTimer
            >> m_apuTrlLengthEnabled
            >> m_apuTrlLengthCounter
            >> m_apuTrlLinerControlReloadFlag
            >> m_apuTrlLinerCounter
            >> m_apuTrlOutput
            >> m_apuTrlPeriodDevider
            >> m_apuTrlStep
            >> m_apuTrlIgnoreReload;
}

qint32 ApuTrl::output() const
{
    return m_apuTrlOutput;
}
