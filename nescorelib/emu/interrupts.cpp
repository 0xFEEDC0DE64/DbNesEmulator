#include "interrupts.h"

// Qt includes
#include <QDataStream>

// local includes
#include "nesemulator.h"

Interrupts::Interrupts(NesEmulator &emu) :
    m_emu(emu)
{
}

void Interrupts::pollStatus()
{
    if(!m_emu.cpu().suspendNmi())
    {
        //The edge detector, see ifnmi occurred.
        if(m_ppuNmiCurrent && !m_ppuNmiOld) //Raising edge, set nmi request
            m_emu.cpu().setNmiPin(true);
        m_ppuNmiCurrent = false; //NMI detected or not, low both lines forthis form ___|-|__
        m_ppuNmiOld = false;
    }

    if(!m_emu.cpu().suspendIrq())
        m_emu.cpu().setIrqPin(!m_emu.cpu().flagI() && m_flags); // irq level detector

    m_vector = m_emu.cpu().nmiPin() ? 0xFFFA : 0xFFFE;
}

void Interrupts::writeState(QDataStream &dataStream) const
{
    dataStream << m_flags << m_ppuNmiCurrent << m_ppuNmiOld << m_vector;
}

void Interrupts::readState(QDataStream &dataStream)
{
    dataStream >> m_flags >> m_ppuNmiCurrent >> m_ppuNmiOld >> m_vector;
}

qint32 Interrupts::flags() const
{
    return m_flags;
}

void Interrupts::addFlag(IrqFlag flag)
{
    m_flags |= flag;
}

void Interrupts::removeFlag(IrqFlag flag)
{
    m_flags &= ~flag;
}

void Interrupts::setFlags(qint32 flags)
{
    m_flags = flags;
}

quint16 Interrupts::vector() const
{
    return m_vector;
}

void Interrupts::setNmiCurrent(bool nmiCurrent)
{
    m_ppuNmiCurrent = nmiCurrent;
}
