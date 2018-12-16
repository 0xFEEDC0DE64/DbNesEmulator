#include "ffe.h"

// Qt includes
#include <QDataStream>

// local includes
#include "nesemulator.h"

Ffe::Ffe(NesEmulator &emu, const Rom &rom) :
    Board(emu, rom)
{
}

Ffe::~Ffe()
{
}

void Ffe::writeEx(quint16 address, quint8 value)
{
    switch (address)
    {
    case 0x4501:
        m_irqEnable = false;
        m_emu.interrupts().removeFlag(Interrupts::IRQ_BOARD);
        break;
    case 0x4502:
        m_irqCounter = (m_irqCounter & 0xFF00) | value;
        break;
    case 0x4503:
        m_irqEnable = true;
        m_irqCounter = (m_irqCounter & 0x00FF) | (value << 8);
        break;
    }
}

void Ffe::onCpuClock()
{
    if (m_irqEnable)
    {
        m_irqCounter++;
        if (m_irqCounter >= 0xFFFF)
        {
            m_irqCounter = 0;
            m_emu.interrupts().addFlag(Interrupts::IRQ_BOARD);
        }
    }
}

void Ffe::readState(QDataStream &dataStream)
{
    Board::readState(dataStream);
    dataStream >> m_irqEnable >> m_irqCounter;
}

void Ffe::writeState(QDataStream &dataStream) const
{
    Board::writeState(dataStream);
    dataStream << m_irqEnable << m_irqCounter;
}
