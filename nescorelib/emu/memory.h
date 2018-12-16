#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// system includes
#include <memory>

// local includes
#include "boards/board.h"

// forward declarations
class QDataStream;
class NesEmulator;
class Board;
struct Rom;

class NESCORELIB_EXPORT Memory
{
public:
    explicit Memory(NesEmulator &emu);

    std::unique_ptr<Board> getBoard(const Rom &rom);

    void initialize(const Rom &rom);
    void hardReset();

    void loadSram();
    void reloadGameGenieCodes();

    quint8 _readWRam(const quint16 address);
    quint8 readWRam(const quint16 address);
    void writeWRam(const quint16 address, const quint8 value);

    quint8 _read(quint16 address);
    quint8 read(quint16 address);
    void write(quint16 address, quint8 value);

    quint8 readEx(const quint16 address);
    void writeEx(const quint16 address, const quint8 value);
    quint8 readSrm(const quint16 address);
    void writeSrm(const quint16 address, const quint8 value);
    quint8 readPrg(const quint16 address);
    void writePrg(const quint16 address, const quint8 value);

    void readState(QDataStream &dataStream);
    void writeState(QDataStream &dataStream) const;

    Board *board();
    const Board *board() const;

    bool busRw() const;
    quint16 busAddress() const;

    static constexpr std::size_t wramAddressToIndex(quint16 address) { return address & 0x7FF; }
    const std::array<quint8, 0x0800> &wram() const;

private:
    NesEmulator &m_emu;

    std::array<quint8, 0x0800> m_wram {};
    std::unique_ptr<Board> m_board {};

    bool m_busRw {};
    quint16 m_busAddress {};
};
