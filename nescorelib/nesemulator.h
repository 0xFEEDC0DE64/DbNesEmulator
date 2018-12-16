#pragma once

#include "nescorelib_global.h"
#include <QObject>

// system includes
#include <array>
#include <memory>

// local includes
#include "emu/apu.h"
#include "emu/cpu.h"
#include "emu/dma.h"
#include "emu/interrupts.h"
#include "emu/memory.h"
#include "emu/ports.h"
#include "emu/ppu.h"

// forward declarations
class QDataStream;

struct Rom;

class NESCORELIB_EXPORT NesEmulator : public QObject
{
    Q_OBJECT

public:
    explicit NesEmulator();

    void load(const Rom &rom);

    void hardReset();
    void softReset();

    void emuClockFrame();
    void emuClockComponents();

    void writeState(QDataStream &dataStream) const;
    void readState(QDataStream &dataStream);

    Apu &apu();
    const Apu &apu() const;
    Cpu &cpu();
    const Cpu &cpu() const;
    Dma &dma();
    const Dma &dma() const;
    Interrupts &interrupts();
    const Interrupts &interrupts() const;
    Memory &memory();
    const Memory &memory() const;
    Ports &ports();
    const Ports &ports() const;
    Ppu &ppu();
    const Ppu &ppu() const;

private Q_SLOTS:
    void frameFinished();

private:
    Apu m_apu;
    Cpu m_cpu;
    Dma m_dma;
    Interrupts m_interrupts;
    Memory m_memory;
    Ports m_ports;
    Ppu m_ppu;

    bool m_frameFinished;
};
