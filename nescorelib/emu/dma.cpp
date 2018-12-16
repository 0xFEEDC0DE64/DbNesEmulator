#include "dma.h"

// Qt includes
#include <QDataStream>

// local includes
#include "nesemulator.h"

Dma::Dma(NesEmulator &emu) :
    m_emu(emu)
{
}

void Dma::hardReset()
{
    m_dmcDmaWaitCycles = 0;
    m_oamDmaWaitCycles = 0;
    m_isOamDma = false;
    m_dmcOn = false;
    m_oamOn = false;
    m_dmcOccurring = false;
    m_oamOccurring = false;
    m_oamFinishCounter = 0;
    m_oamAddress = 0;
    m_oamCycle = 0;
    m_latch = 0;
}

void Dma::softReset()
{
    m_dmcDmaWaitCycles = 0;
    m_oamDmaWaitCycles = 0;
    m_isOamDma = false;
    m_dmcOn = false;
    m_oamOn = false;
    m_dmcOccurring = false;
    m_oamOccurring = false;
    m_oamFinishCounter = 0;
    m_oamAddress = 0;
    m_oamCycle = 0;
    m_latch = 0;
}

void Dma::assertDmcDma()
{
    if(m_oamOccurring)
    {
        if(m_oamCycle < 508)
            // OAM DMA is occurring here
            m_dmcDmaWaitCycles = m_emu.memory().busRw() ? 1 : 0;
        else
        {
            // Here the oam dma is about to finish
            // Remaining cycles of oam dma determines the dmc dma waiting cycles.
            m_dmcDmaWaitCycles = 4 - (512 - m_oamCycle);
        }
    }
    else if(m_dmcOccurring)
    {
        // DMC occurring now !!? is that possible ?
        // Anyway, let's ignore this call !
        return;
    }
    else
    {
        // Nothing occurring, initialize brand new dma
        // DMC DMA depends on r/w flag forthe wait cycles.
        m_dmcDmaWaitCycles = m_emu.memory().busRw() ? 3 : 2;
        // After 2 cycles of oam dma, add extra cycle forthe waiting.
        if(m_oamFinishCounter == 3)
            m_dmcDmaWaitCycles++;
    }
    m_isOamDma = false;
    m_dmcOn = true;
}

void Dma::assertOamDma()
{
    if(m_oamOccurring)
      return;
    // Setup
    // OAM DMA depends on apu odd timer forodd cycles
    m_oamDmaWaitCycles = m_emu.apu().oddCycle() ? 1 : 2;
    m_isOamDma = true;
    m_oamOn = true;
}

void Dma::clock()
{
    if(m_oamFinishCounter > 0)
        m_oamFinishCounter--;

    if(!m_emu.memory().busRw()) // Clocks only on reads
    {
        if(m_dmcDmaWaitCycles > 0)
            m_dmcDmaWaitCycles--;
        if(m_oamDmaWaitCycles > 0)
            m_oamDmaWaitCycles--;
        return;
    }

    if(m_dmcOn)
    {
        m_dmcOccurring = true;
        // This is it !
        m_dmcOn = false;
        // Do wait cycles (extra reads)
        if(m_dmcDmaWaitCycles > 0)
        {
            if(m_emu.memory().busAddress() == 0x4016 || m_emu.memory().busAddress() == 0x4017)
            {
                m_emu.memory().read(m_emu.memory().busAddress());
                m_dmcDmaWaitCycles--;

                while(m_dmcDmaWaitCycles > 0)
                {
                    m_emu.emuClockComponents();
                    m_dmcDmaWaitCycles--;
                }
            }
            else
            {
                while(m_dmcDmaWaitCycles > 0)
                {
                    m_emu.memory().read(m_emu.memory().busAddress());
                    m_dmcDmaWaitCycles--;
                }
            }
        }

        // Do DMC DMA
        m_emu.apu().dmc().apuDmcDoDma();

        m_dmcOccurring = false;
    }

    if(m_oamOn)
    {
        m_oamOccurring = true;
        // This is it ! pause the cpu
        m_oamOn = false;
        // Do wait cycles (extra reads)
        if(m_oamDmaWaitCycles > 0)
        {
            if(m_emu.memory().busAddress() == 0x4016 || m_emu.memory().busAddress() == 0x4017)
            {
                m_emu.memory().read(m_emu.memory().busAddress());
                m_oamDmaWaitCycles--;

                while(m_oamDmaWaitCycles > 0)
                {
                    m_emu.emuClockComponents();
                    m_oamDmaWaitCycles--;
                }
            }
            else
            {
                while(m_oamDmaWaitCycles > 0)
                {
                    m_emu.memory().read(m_emu.memory().busAddress());
                    m_oamDmaWaitCycles--;
                }
            }
        }

        // Do OAM DMA
        m_oamCycle = 0;
        for(auto i = 0; i < 256; i++)
        {
            m_latch = m_emu.memory().read(m_oamAddress);
            m_oamCycle++;
            m_emu.memory().write(0x2004, m_latch);
            m_oamCycle++;
            m_oamAddress++;
            m_oamAddress = m_oamAddress & 0xFFFF;
        }

        m_oamCycle = 0;
        m_oamFinishCounter = 5;
        m_oamOccurring = false;
    }
}

void Dma::writeState(QDataStream &dataStream) const
{
    dataStream << m_dmcDmaWaitCycles << m_oamDmaWaitCycles << m_isOamDma << m_dmcOn << m_oamOn << m_dmcOccurring
               << m_oamOccurring << m_oamFinishCounter << m_oamAddress << m_oamCycle << m_latch;
}

void Dma::readState(QDataStream &dataStream)
{
    dataStream >> m_dmcDmaWaitCycles >> m_oamDmaWaitCycles >> m_isOamDma >> m_dmcOn >> m_oamOn >> m_dmcOccurring
               >> m_oamOccurring >> m_oamFinishCounter >> m_oamAddress >> m_oamCycle >> m_latch;
}

void Dma::setOamAddress(quint16 oamAddress)
{
    m_oamAddress = oamAddress;
}
