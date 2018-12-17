#include "nesemulator.h"

// system includes
#include <cmath>
#include <algorithm>

// local includes
#include "emusettings.h"

NesEmulator::NesEmulator() :
    m_apu(*this),
    m_cpu(*this),
    m_dma(*this),
    m_interrupts(*this),
    m_memory(*this),
    m_ports(*this),
    m_ppu(*this)
{
    QObject::connect(&m_ppu, &Ppu::frameFinished, this, &NesEmulator::frameFinished);
}

void NesEmulator::load(const Rom &rom)
{
    m_memory.initialize(rom);

    hardReset();

    if(m_memory.board()->enableExternalSound())
        m_memory.board()->apuApplyChannelsSettings();
}

void NesEmulator::hardReset()
{
    m_memory.hardReset();
    m_cpu.hardReset();
    m_ppu.hardReset();
    m_apu.hardReset();
    m_dma.hardReset();
}

void NesEmulator::softReset()
{
    m_cpu.softReset();
    m_apu.softReset();
}

void NesEmulator::emuClockFrame()
{
    m_frameFinished = false;
    while(!m_frameFinished)
        m_cpu.clock();

    /*
    m_emuTimeFrame = QDateTime::currentMSecsSinceEpoch() - m_emuTimePrevious;

    if (EmuSettings::frameLimiterEnabled)
    {
        const auto emuTimeDead = EmuSettings::emuTimeFramePeriod - m_emuTimeFrame;
        if (emuTimeDead > 0)
        {
            // 1 Sleep thread
            QThread::msleep(std::floor(emuTimeDead * 1000));

            // 2 Kill remaining time
            m_emuTimeFrameImmediate = QDateTime::currentMSecsSinceEpoch() - m_emuTimePrevious;
            while (EmuSettings::emuTimeFramePeriod - m_emuTimeFrameImmediate > 0)
                m_emuTimeFrameImmediate = QDateTime::currentMSecsSinceEpoch() - m_emuTimePrevious;
        }
    }
    else
        m_emuTimeFrameImmediate = QDateTime::currentMSecsSinceEpoch() - m_emuTimePrevious;
    m_emuTimePrevious = QDateTime::currentMSecsSinceEpoch();
    */
}

void NesEmulator::emuClockComponents()
{
    m_ppu.clock();
    m_interrupts.pollStatus();
    m_ppu.clock();
    m_ppu.clock();
    m_apu.clock();
    m_dma.clock();
    m_memory.board()->onCpuClock();
}

void NesEmulator::writeState(QDataStream &dataStream) const
{
    m_apu.writeState(dataStream);
    m_cpu.writeState(dataStream);
    m_dma.writeState(dataStream);
    m_interrupts.writeState(dataStream);
    m_memory.writeState(dataStream);
    m_ports.portWriteState(dataStream);
    m_ppu.writeState(dataStream);
}

void NesEmulator::readState(QDataStream &dataStream)
{
    m_apu.readState(dataStream);
    m_cpu.readState(dataStream);
    m_dma.readState(dataStream);
    m_interrupts.readState(dataStream);
    m_memory.readState(dataStream);
    m_ports.portReadState(dataStream);
    m_ppu.readState(dataStream);
}

Apu &NesEmulator::apu()
{
    return m_apu;
}

const Apu &NesEmulator::apu() const
{
    return m_apu;
}

Cpu &NesEmulator::cpu()
{
    return m_cpu;
}

const Cpu &NesEmulator::cpu() const
{
    return m_cpu;
}

Dma &NesEmulator::dma()
{
    return m_dma;
}

const Dma &NesEmulator::dma() const
{
    return m_dma;
}

Interrupts &NesEmulator::interrupts()
{
    return m_interrupts;
}

const Interrupts &NesEmulator::interrupts() const
{
    return m_interrupts;
}

Memory &NesEmulator::memory()
{
    return m_memory;
}

const Memory &NesEmulator::memory() const
{
    return m_memory;
}

Ports &NesEmulator::ports()
{
    return m_ports;
}

const Ports &NesEmulator::ports() const
{
    return m_ports;
}

Ppu &NesEmulator::ppu()
{
    return m_ppu;
}

const Ppu &NesEmulator::ppu() const
{
    return m_ppu;
}

void NesEmulator::frameFinished()
{
    m_apu.flush();
    m_frameFinished = true;
}
