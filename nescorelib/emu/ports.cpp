#include "ports.h"

// Qt includes
#include <QDataStream>

Ports::Ports(NesEmulator &emu) :
    m_emu(emu)
{
}

void Ports::update()
{
    for(auto &input : m_inputs)
        if(input)
            input->update();
}

void Ports::updatePorts()
{
    const auto getData = [this](int index){ return m_inputs[index] ? m_inputs[index]->getData() : 0; };
    m_port0 = getData(2) << 8 | getData(0) | 0x01010000;
    m_port1 = getData(3) << 8 | getData(1) | 0x02020000;
}

void Ports::portWriteState(QDataStream &dataStream) const
{
    dataStream << m_port0 << m_port1;
}

void Ports::portReadState(QDataStream &dataStream)
{
    dataStream >> m_port0 >> m_port1;
}

quint32 Ports::port0() const
{
    return m_port0;
}

void Ports::setPort0(quint32 port0)
{
    m_port0 = port0;
}

quint32 Ports::port1() const
{
    return m_port1;
}

void Ports::setPort1(quint32 port1)
{
    m_port1 = port1;
}
