#include "mapper004.h"

// local includes
#include "nesemulator.h"
#include "utils/datastreamutils.h"

QString Mapper004::name() const
{
    return QStringLiteral("MMC3");
}

quint8 Mapper004::mapper() const
{
    return 4;
}

void Mapper004::hardReset()
{
    Board::hardReset();

    // Flags
    m_flagC = false;
    m_flagP = false;
    m_address8001 = 0;

    m_prgReg[0] = 0;
    m_prgReg[1] = 1;
    m_prgReg[2] = prgRom8KbMask()-1;
    m_prgReg[3] = prgRom8KbMask();

    setupPRG();

    // CHR
    for (int i = 0; i < 6; i++)
        m_chrReg[i] = 0;

    // IRQ
    m_irqEnabled = false;
    m_irqCounter = 0;
    m_irqReload = 0xFF;
    m_oldIrqCounter = 0;
    m_mmc3AltBehavior = false;
    m_irqClear = false;

//    switch (GameCartInfo.chip_type[0].ToLower())
//    {
//    case "mmc3a": mmc3_alt_behavior = true; break;
//    case "mmc3b": mmc3_alt_behavior = false; break;
//    case "mmc3c": mmc3_alt_behavior = false; break;
//    }
}

void Mapper004::writePrg(quint16 address, quint8 value)
{
    switch (address & 0xE001)
    {
    case 0x8000:
        m_address8001 = value & 0x7;
        m_flagC = (value & 0x80) != 0;
        m_flagP = (value & 0x40) != 0;
        setupCHR();
        setupPRG(); break;
    case 0x8001:
        switch (m_address8001)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            m_chrReg[m_address8001] = value; setupCHR();
            break;
        case 6:
        case 7:
            m_prgReg[m_address8001 - 6] = value & prgRom8KbMask();
            setupPRG();
            break;
        }
        break;
    case 0xA000:
        if (m_rom.mirroring != Mirroring::Full)
            switch1kNmt((value & 1) == 1 ? Mirroring::Horizontal : Mirroring::Vertical);
        break;
    case 0xA001:
        togglePrgRamEnable(value & 0x80);
        togglePrgRamWritableEnable((value & 0x40) == 0);
        break;
    case 0xC000:
        m_irqReload = value;
        break;
    case 0xC001:
        if (m_mmc3AltBehavior)
            m_irqClear = true;
        m_irqCounter = 0;
        break;
    case 0xE000:
        m_irqEnabled = false;
        m_emu.interrupts().removeFlag(Interrupts::IRQ_BOARD);
        break;
    case 0xE001:
        m_irqEnabled = true;
        break;
    }
}

void Mapper004::onPpuA12RaisingEdge()
{
    m_oldIrqCounter = m_irqCounter;

    if (m_irqCounter == 0 || m_irqClear)
        m_irqCounter = m_irqReload;
    else
        m_irqCounter = m_irqCounter - 1;

    if ((!m_mmc3AltBehavior || m_oldIrqCounter != 0 || m_irqClear) && m_irqCounter == 0 && m_irqEnabled)
        m_emu.interrupts().addFlag(Interrupts::IRQ_BOARD);

    m_irqClear = false;
}

void Mapper004::readState(QDataStream &dataStream)
{
    Board::readState(dataStream);

    dataStream >> m_flagC >> m_flagP >> m_address8001 >> m_chrReg >> m_prgReg
    // IRQ
    >> m_irqEnabled >> m_irqCounter >> m_oldIrqCounter >> m_irqReload >> m_irqClear >> m_mmc3AltBehavior;
}

void Mapper004::writeState(QDataStream &dataStream) const
{
    Board::writeState(dataStream);

    dataStream << m_flagC << m_flagP << m_address8001 << m_chrReg << m_prgReg
    // IRQ
               << m_irqEnabled << m_irqCounter << m_oldIrqCounter << m_irqReload << m_irqClear << m_mmc3AltBehavior;
}

bool Mapper004::ppuA12ToggleTimerEnabled() const
{
    return true;
}

bool Mapper004::ppuA12TogglesOnRaisingEdge() const
{
    return true;
}

void Mapper004::setupCHR()
{
    if (!m_flagC)
    {
        switch2kChr(m_chrReg[0] >> 1, CHRArea::Area0000);
        switch2kChr(m_chrReg[1] >> 1, CHRArea::Area0800);
        switch1kChr(m_chrReg[2], CHRArea::Area1000);
        switch1kChr(m_chrReg[3], CHRArea::Area1400);
        switch1kChr(m_chrReg[4], CHRArea::Area1800);
        switch1kChr(m_chrReg[5], CHRArea::Area1C00);
    }
    else
    {
        switch2kChr(m_chrReg[0] >> 1, CHRArea::Area1000);
        switch2kChr(m_chrReg[1] >> 1, CHRArea::Area1800);
        switch1kChr(m_chrReg[2], CHRArea::Area0000);
        switch1kChr(m_chrReg[3], CHRArea::Area0400);
        switch1kChr(m_chrReg[4], CHRArea::Area0800);
        switch1kChr(m_chrReg[5], CHRArea::Area0C00);
    }
}

void Mapper004::setupPRG()
{
    switch8kPrg(m_prgReg[m_flagP ? 2 : 0], PRGArea::Area8000);
    switch8kPrg(m_prgReg[1], PRGArea::AreaA000);
    switch8kPrg(m_prgReg[m_flagP ? 0 : 2], PRGArea::AreaC000);
    switch8kPrg(m_prgReg[3], PRGArea::AreaE000);
}
