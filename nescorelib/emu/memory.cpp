#include "memory.h"

// Qt includes
#include <QDataStream>

// dbcorelib includes
#include "utils/datastreamutils.h"

// local includes
#include "nesemulator.h"
#include "rom.h"
#include "mappers/mapper000.h"
#include "mappers/mapper001.h"
#include "mappers/mapper002.h"
#include "mappers/mapper003.h"
#include "mappers/mapper004.h"

Memory::Memory(NesEmulator &emu) :
    m_emu(emu)
{
}

std::unique_ptr<Board> Memory::getBoard(const Rom &rom)
{
    switch(rom.mapperNumber)
    {
    case 0: return std::make_unique<Mapper000>(m_emu, rom);
    case 1: return std::make_unique<Mapper001>(m_emu, rom);
    case 2: return std::make_unique<Mapper002>(m_emu, rom);
    case 3: return std::make_unique<Mapper003>(m_emu, rom);
    case 4: return std::make_unique<Mapper004>(m_emu, rom);
    }

    throw std::runtime_error(QString("unknown mapper %0").arg(rom.mapperNumber).toStdString());
}

void Memory::initialize(const Rom &rom)
{
    m_board = getBoard(rom);
    m_board->mapper();
}

void Memory::hardReset()
{
    m_wram.fill(0);
    m_wram[0x08] = 0xF7;
    m_wram[0x09] = 0xEF;
    m_wram[0x0A] = 0xDF;
    m_wram[0x0F] = 0xBF;

    loadSram();

    reloadGameGenieCodes();

    m_board->hardReset();
}

void Memory::loadSram()
{
    //TODO
}

void Memory::reloadGameGenieCodes()
{
    //TODO
}

quint8 Memory::_readWRam(const quint16 address)
{
    return m_wram[wramAddressToIndex(address & 0x7FF)];
}

quint8 Memory::readWRam(const quint16 address)
{
    auto result = _readWRam(address);
    return result;
}

void Memory::writeWRam(const quint16 address, const quint8 value)
{
    m_wram[address & 0x7FF] = value;
}

quint8 Memory::_read(quint16 address)
{
    m_busRw = true;
    m_busAddress = address;
    m_emu.emuClockComponents();

    const auto roundAddress = [](quint16 address) { return (address & 0xF000) >> 12; };
    switch(roundAddress(address))
    {
    case roundAddress(0x0000):
    case roundAddress(0x1000):
        return readWRam(address);
    case roundAddress(0x2000):
    case roundAddress(0x3000):
        return m_emu.ppu().ioRead(address);
    case roundAddress(0x4000):
        return m_emu.apu().ioRead(address);
    case roundAddress(0x5000):
        return readEx(address);
    case roundAddress(0x6000):
    case roundAddress(0x7000):
        return readSrm(address);
    case roundAddress(0x8000):
    case roundAddress(0x9000):
    case roundAddress(0xA000):
    case roundAddress(0xB000):
    case roundAddress(0xC000):
    case roundAddress(0xD000):
    case roundAddress(0xE000):
    case roundAddress(0xF000):
        return readPrg(address);
    }

    qFatal("undefined read");
    return 0;
}

quint8 Memory::read(quint16 address)
{
    auto result = _read(address);
    return result;
}

void Memory::write(quint16 address, quint8 value)
{
    m_busRw = false;
    m_busAddress = address;
    m_emu.emuClockComponents();

    const auto roundAddress = [](quint16 address) { return (address & 0xF000) >> 12; };
    switch(roundAddress(address))
    {
    case roundAddress(0x0000):
    case roundAddress(0x1000):
        writeWRam(address, value);
        break;
    case roundAddress(0x2000):
    case roundAddress(0x3000):
        m_emu.ppu().ioWrite(address, value);
        break;
    case roundAddress(0x4000):
        m_emu.apu().ioWrite(address, value);
        break;
    case roundAddress(0x5000):
        writeEx(address, value);
        break;
    case roundAddress(0x6000):
    case roundAddress(0x7000):
        writeSrm(address, value);
        break;
    case roundAddress(0x8000):
    case roundAddress(0x9000):
    case roundAddress(0xA000):
    case roundAddress(0xB000):
    case roundAddress(0xC000):
    case roundAddress(0xD000):
    case roundAddress(0xE000):
    case roundAddress(0xF000):
        writePrg(address, value);
        break;
    default:
        qFatal("undefined write");
    }
}

quint8 Memory::readEx(const quint16 address)
{
    return m_board->readEx(address);
}

void Memory::writeEx(const quint16 address, const quint8 value)
{
    m_board->writeEx(address, value);
}

quint8 Memory::readSrm(const quint16 address)
{
    return m_board->readSrm(address);
}

void Memory::writeSrm(const quint16 address, const quint8 value)
{
    m_board->writeSrm(address, value);
}

quint8 Memory::readPrg(const quint16 address)
{
    return m_board->readPrg(address);
}

void Memory::writePrg(const quint16 address, const quint8 value)
{
    m_board->writePrg(address, value);
}

void Memory::readState(QDataStream &dataStream)
{
    dataStream >> m_wram >> m_busRw >> m_busAddress;
    m_board->readState(dataStream);
}

void Memory::writeState(QDataStream &dataStream) const
{
    dataStream << m_wram << m_busRw << m_busAddress;
    m_board->writeState(dataStream);
}

Board *Memory::board()
{
    return m_board.get();
}

const Board *Memory::board() const
{
    return m_board.get();
}

bool Memory::busRw() const
{
    return m_busRw;
}

quint16 Memory::busAddress() const
{
    return m_busAddress;
}

const std::array<quint8, 0x0800> &Memory::wram() const
{
    return m_wram;
}
