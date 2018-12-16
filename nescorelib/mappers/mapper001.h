#pragma once

#include "nescorelib_global.h"
#include "boards/board.h"

class NESCORELIB_EXPORT Mapper001 : public Board
{
public:
    using Board::Board;

    QString name() const Q_DECL_OVERRIDE;
    quint8 mapper() const Q_DECL_OVERRIDE;

    void hardReset() Q_DECL_OVERRIDE;
    void writePrg(quint16 address, quint8 value) Q_DECL_OVERRIDE;
    void onCpuClock() Q_DECL_OVERRIDE;
    void readState(QDataStream &dataStream) Q_DECL_OVERRIDE;
    void writeState(QDataStream &dataStream) const Q_DECL_OVERRIDE;

protected:
    int prgRam8KbDefaultBlkCount() const Q_DECL_OVERRIDE;
    int chrRom1KbDefaultBlkCount() const Q_DECL_OVERRIDE;

private:
    void updateCHR();
    void updatePRG();

    int m_addressReg;
    std::array<quint8, 4> m_reg;
    quint8 m_shift {};
    quint8 m_buffer {};
    bool m_flagP;
    bool m_flagC;
    bool m_flagS;
    bool m_enableWramEnable;
    int m_prgHijackedBit;
    bool m_useHijacked;
    bool m_useSramSwitch;
    int m_sramSwitchMask;
    int m_cpuCycles;
};
