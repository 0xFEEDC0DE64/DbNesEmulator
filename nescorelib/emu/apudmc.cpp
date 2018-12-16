#include "apudmc.h"

// Qt includes
#include <QDataStream>

// local includes
#include "emusettings.h"
#include "apu.h"
#include "nesemulator.h"

ApuDmc::ApuDmc(Apu &apu) :
    m_apu(apu)
{
}

void ApuDmc::apuDmcHardReset()
{
    m_apuDmcOutputA = 0;
    m_apuDmcOutput = 0;
    m_apuDmcPeriodDevider = 0;
    m_apuDmcLoopFlag = false;
    m_apuDmcRateIndex = 0;

    m_apuDmcIrqEnabled = false;
    m_apuDmcDmaAddr = 0xC000;
    m_apuDmcAddrRefresh = 0xC000;
    m_apuDmcSizeRefresh = 0;
    m_apuDmcDmaBits = 1;
    m_apuDmcDmaByte = 1;
    m_apuDmcPeriodDevider = 0;
    m_apuDmcDmaEnabled = false;
    m_apuDmcBufferFull = false;
    m_apuDmcDmaSize = 0;
}

void ApuDmc::apuDmcSoftReset()
{
    apuDmcHardReset();
}

void ApuDmc::apuDmcClock()
{
    static constexpr std::array<qint32, 32> freqTable = [](){
        switch(EmuSettings::region)
        {
        case EmuRegion::NTSC:
            return std::array<qint32, 32> {
                428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
            };
        case EmuRegion::PALB:
            return std::array<qint32, 32> {
                398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50
            };
        case EmuRegion::DENDY:
            return std::array<qint32, 32> {
                428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
            };
        }
    }();

    if (--m_apuDmcPeriodDevider <= 0)
    {
        m_apuDmcPeriodDevider = freqTable[m_apuDmcRateIndex];

        if (m_apuDmcDmaEnabled)
        {
            if ((m_apuDmcDmaByte & 0x01) != 0)
            {
                if (m_apuDmcOutputA <= 0x7D)
                    m_apuDmcOutputA += 2;
            }
            else
            {
                if (m_apuDmcOutputA >= 0x02)
                    m_apuDmcOutputA -= 2;
            }
            m_apuDmcDmaByte >>= 1;
        }

        if (--m_apuDmcDmaBits == 0)
        {
            m_apuDmcDmaBits = 8;
            if (m_apuDmcBufferFull)
            {
                m_apuDmcBufferFull = false;
                m_apuDmcDmaEnabled = true;
                m_apuDmcDmaByte = m_apuDmcDmaBuffer;
                // RDY ?
                if (m_apuDmcDmaSize > 0)
                    m_apu.emu().dma().assertDmcDma();
            }
            else
                m_apuDmcDmaEnabled = false;
        }

        if (EmuSettings::Audio::ChannelEnabled::DMC)
            m_apuDmcOutput = m_apuDmcOutputA;
    }
}

void ApuDmc::apuDmcDoDma()
{
    m_apuDmcBufferFull = true;

    m_apuDmcDmaBuffer = m_apu.emu().memory().read(m_apuDmcDmaAddr);

    if (m_apuDmcDmaAddr == 0xFFFF)
        m_apuDmcDmaAddr = 0x8000;
    else
        m_apuDmcDmaAddr++;

    if (m_apuDmcDmaSize > 0)
        m_apuDmcDmaSize--;

    if (m_apuDmcDmaSize == 0)
    {
        if (m_apuDmcLoopFlag)
        {
            m_apuDmcDmaSize = m_apuDmcSizeRefresh;
            m_apuDmcDmaAddr = m_apuDmcAddrRefresh;
        }
        else if (m_apuDmcIrqEnabled)
        {
            m_apu.emu().interrupts().addFlag(Interrupts::IRQ_DMC);
            m_apu.setIrqDeltaOccur(true);
        }
    }
}

void ApuDmc::apuOnRegister4010()
{
    if (!m_apu.regAccessW())
        return;

    m_apuDmcIrqEnabled = m_apu.regIoDb() & 0x80;
    m_apuDmcLoopFlag = m_apu.regIoDb() & 0x40;

    if (!m_apuDmcIrqEnabled)
    {
        m_apu.setIrqDeltaOccur(false);
        m_apu.emu().interrupts().removeFlag(Interrupts::IRQ_DMC);
    }

    m_apuDmcRateIndex = m_apu.regIoDb() & 0x0F;
}

void ApuDmc::apuOnRegister4011()
{
    if (!m_apu.regAccessW())
        return;

    m_apuDmcOutputA = m_apu.regIoDb() & 0x7F;
}

void ApuDmc::apuOnRegister4012()
{
    if (!m_apu.regAccessW())
        return;

    m_apuDmcAddrRefresh = (m_apu.regIoDb() << 6) | 0xC000;
}

void ApuDmc::apuOnRegister4013()
{
    if (!m_apu.regAccessW())
        return;

    m_apuDmcSizeRefresh = (m_apu.regIoDb() << 4) | 0x0001;
}

void ApuDmc::apuDmcOn4015()
{
    // DMC
    if (m_apu.regIoDb() & 0x10)
    {
        if (m_apuDmcDmaSize == 0)
        {
            m_apuDmcDmaSize = m_apuDmcSizeRefresh;
            m_apuDmcDmaAddr = m_apuDmcAddrRefresh;
        }
    }
    else
        m_apuDmcDmaSize = 0;

    // Disable DMC IRQ
    m_apu.setIrqDeltaOccur(false);
    m_apu.emu().interrupts().removeFlag(Interrupts::IRQ_DMC);

    // RDY ?
    if (!m_apuDmcBufferFull && m_apuDmcDmaSize > 0)
        m_apu.emu().dma().assertDmcDma();
}

void ApuDmc::apuDmcRead4015()
{
    if (m_apuDmcDmaSize > 0)
        m_apu.setRegIoDb((m_apu.regIoDb() & 0xEF) | 0x10);
}

void ApuDmc::apuDmcWriteState(QDataStream &dataStream) const
{
    dataStream << m_apuDmcOutputA << m_apuDmcOutput << m_apuDmcPeriodDevider << m_apuDmcIrqEnabled << m_apuDmcLoopFlag
               << m_apuDmcRateIndex << m_apuDmcAddrRefresh << m_apuDmcSizeRefresh << m_apuDmcDmaEnabled << m_apuDmcDmaByte
               << m_apuDmcDmaBits << m_apuDmcBufferFull << m_apuDmcDmaBuffer << m_apuDmcDmaSize << m_apuDmcDmaAddr;
}

void ApuDmc::apuDmcReadState(QDataStream &dataStream)
{
    dataStream >> m_apuDmcOutputA >> m_apuDmcOutput >> m_apuDmcPeriodDevider >> m_apuDmcIrqEnabled >> m_apuDmcLoopFlag
               >> m_apuDmcRateIndex >> m_apuDmcAddrRefresh >> m_apuDmcSizeRefresh >> m_apuDmcDmaEnabled >> m_apuDmcDmaByte
               >> m_apuDmcDmaBits >> m_apuDmcBufferFull >> m_apuDmcDmaBuffer >> m_apuDmcDmaSize >> m_apuDmcDmaAddr;
}

qint32 ApuDmc::output() const
{
    return m_apuDmcOutput;
}
