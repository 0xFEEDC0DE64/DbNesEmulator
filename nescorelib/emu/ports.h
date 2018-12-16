#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// system includes
#include <memory>

// local includes
#include "inputprovider.h"

// forward declarations
class NesEmulator;
class QDataStream;

class NESCORELIB_EXPORT Ports
{
public:
    explicit Ports(NesEmulator &emu);

    void update();
    void updatePorts();

    void portWriteState(QDataStream &dataStream) const;
    void portReadState(QDataStream &dataStream);

    quint32 port0() const;
    void setPort0(quint32 port0);

    quint32 port1() const;
    void setPort1(quint32 port1);

private:
    NesEmulator &m_emu;

    std::array<std::unique_ptr<InputProvider>, 4> m_inputs {};

    quint32 m_port0 {};
    quint32 m_port1 {};
};
