#pragma once

#include "nescorelib_global.h"
#include "boards/board.h"

class NESCORELIB_EXPORT Mapper004 : public Board
{
public:
    using Board::Board;

    QString name() const Q_DECL_OVERRIDE;
    quint8 mapper() const Q_DECL_OVERRIDE;

    void hardReset() Q_DECL_OVERRIDE;

    void writePrg(quint16 address, quint8 value) Q_DECL_OVERRIDE;

    void onPpuA12RaisingEdge() Q_DECL_OVERRIDE;

    void readState(QDataStream &dataStream) Q_DECL_OVERRIDE;
    void writeState(QDataStream &dataStream) const Q_DECL_OVERRIDE;

protected:
    bool ppuA12ToggleTimerEnabled() const Q_DECL_OVERRIDE;
    bool ppuA12TogglesOnRaisingEdge() const Q_DECL_OVERRIDE;

private:
    void setupCHR();
    void setupPRG();

    bool m_flagC;
    bool m_flagP;
    int m_address8001;
    std::array<int, 6> m_chrReg;
    std::array<int, 4> m_prgReg;

    // IRQ
    bool m_irqEnabled;
    quint8 m_irqCounter;
    int m_oldIrqCounter;
    quint8 m_irqReload;
    bool m_irqClear;
    bool m_mmc3AltBehavior;
};
