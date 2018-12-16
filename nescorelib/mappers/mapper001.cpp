#include "mapper001.h"

#include "utils/datastreamutils.h"

QString Mapper001::name() const
{
    return QStringLiteral("MMC1");
}

quint8 Mapper001::mapper() const
{
    return 1;
}


void Mapper001::hardReset()
{
    Board::hardReset();

    m_cpuCycles = 0;

    // Registers
    m_addressReg = 0;
    m_reg = { 0x0C, 0, 0, 0 };
    m_reg[0] = 0x0C;
    m_flagC = false;
    m_flagS = true;
    m_flagP = true;
    m_prgHijackedBit = 0;

    // Buffers
    m_buffer = 0;
    m_shift = 0;

    //if (Chips.Contains("MMC1B") || Chips.Contains("MMC1B2"))
    //    TogglePRGRAMEnable(false);

    m_enableWramEnable = true; // !Chips.Contains("MMC1A");

    //  use hijacked
    m_useHijacked = (prgRom16KbMask() & 0x10) == 0x10;
    if (m_useHijacked)
        m_prgHijackedBit = 0x10;

    // SRAM Switch ?
    m_useSramSwitch = false;
    if (prgRam8KbCount() > 0)
    {
        m_useSramSwitch = true;
        m_sramSwitchMask = m_useHijacked ? 0x08 : 0x18;
        m_sramSwitchMask &= prgRam8KbMask() << 3;

        if (m_sramSwitchMask == 0)
            m_useSramSwitch = false;
    }

    switch16kPrg(0xF | m_prgHijackedBit, PRGArea::AreaC000);
}

void Mapper001::writePrg(quint16 address, quint8 value)
{
    // Too close writes ignored !
    if (m_cpuCycles > 0)
        return;

    m_cpuCycles = 3;// Make save cycles ...
    //Temporary reg port ($8000-FFFF):
    //[r... ...d]
    //r = reset flag
    //d = data bit

    //r is set
    if ((value & 0x80) == 0x80)
    {
        m_reg[0] |= 0x0C;//bits 2,3 of reg $8000 are set (16k PRG mode, $8000 swappable)
        m_flagS = true;
        m_flagP = true;
        m_shift = 0;
        m_buffer = 0;//hidden temporary reg is reset
        return;
    }
    //d is set
    if ((value & 0x01) == 0x01)
        m_buffer |= 1 << m_shift; //'d' proceeds as the next bit written in the 5-bit sequence
    if (++m_shift < 5)
        return;
    // If this completes the 5-bit sequence:
    // - temporary reg is copied to actual internal reg (which reg depends on the last address written to)
    m_addressReg = (address & 0x7FFF) >> 13;
    m_reg[m_addressReg] = m_buffer;

    // - temporary reg is reset (so that next write is the "first" write)
    m_shift = 0;
    m_buffer = 0;

    // Update internal registers ...
    switch (m_addressReg)
    {
    case 0:// $8000-9FFF [Flags and mirroring]
        // Flags
        m_flagC = m_reg[0] & 0x10;
        m_flagP = m_reg[0] & 0x08;
        m_flagS = m_reg[0] & 0x04;
        updatePRG();
        updateCHR();
        // Mirroring
        switch (m_reg[0] & 3)
        {
        case 0: switch1kNmt(Mirroring::OneScA); break;
        case 1: switch1kNmt(Mirroring::OneScB); break;
        case 2: switch1kNmt(Mirroring::Vertical); break;
        case 3: switch1kNmt(Mirroring::Horizontal); break;
        }
        break;
    case 1:// $A000-BFFF [CHR REG 0]
        // CHR
        if (!m_flagC)
            switch8kChr(m_reg[1] >> 1);
        else
            switch4kChr(m_reg[1], CHRArea::Area0000);
        // SRAM
        if (m_useSramSwitch)
            switch8kPrg((m_reg[1] & m_sramSwitchMask) >> 3, PRGArea::Area6000);
        // PRG hijack
        if (m_useHijacked)
        {
            m_prgHijackedBit = m_reg[1] & 0x10;
            updatePRG();
        }
        break;
    case 2:// $C000-DFFF [CHR REG 1]
        // CHR
        if (m_flagC)
            switch4kChr(m_reg[2], CHRArea::Area1000);
        // SRAM
        if (m_useSramSwitch)
            switch8kPrg((m_reg[2] & m_sramSwitchMask) >> 3, PRGArea::Area6000);
        // PRG hijack
        if (m_useHijacked)
        {
            m_prgHijackedBit = m_reg[2] & 0x10;
            updatePRG();
        }
        break;
    case 3:// $E000-FFFF [PRG REG]
        if (m_enableWramEnable)
            togglePrgRamEnable((m_reg[3] & 0x10) == 0);
        updatePRG();
        break;
    }
}

void Mapper001::onCpuClock()
{
    if (m_cpuCycles > 0)
        m_cpuCycles--;
}

void Mapper001::readState(QDataStream &dataStream)
{
    Board::readState(dataStream);
    dataStream >> m_reg >> m_shift >> m_buffer >> m_flagP >> m_flagC >> m_flagS >> m_enableWramEnable
               >> m_prgHijackedBit >> m_useHijacked >> m_useSramSwitch >> m_cpuCycles;
}

void Mapper001::writeState(QDataStream &dataStream) const
{
    Board::writeState(dataStream);
    dataStream << m_reg << m_shift << m_buffer << m_flagP << m_flagC << m_flagS << m_enableWramEnable
               << m_prgHijackedBit << m_useHijacked << m_useSramSwitch << m_cpuCycles;
}

int Mapper001::prgRam8KbDefaultBlkCount() const
{
    return 4;
}

int Mapper001::chrRom1KbDefaultBlkCount() const
{
    return 64;
}

void Mapper001::updateCHR()
{
    if (!m_flagC)
        switch8kChr(m_reg[1] >> 1);
    else
    {
        switch4kChr(m_reg[1], CHRArea::Area0000);
        switch4kChr(m_reg[2], CHRArea::Area1000);
    }
    // SRAM
    if (m_useSramSwitch)
        switch8kPrg((m_reg[1] & m_sramSwitchMask) >> 3, PRGArea::Area6000);
}

void Mapper001::updatePRG()
{
    if (!m_flagP)
    {
        switch32kPrg(((m_reg[3] & 0xF) | m_prgHijackedBit) >> 1, PRGArea::Area8000);
    }
    else
    {
        if (m_flagS)
        {
            switch16kPrg((m_reg[3] & 0xF) | m_prgHijackedBit, PRGArea::Area8000);
            switch16kPrg(0xF | m_prgHijackedBit, PRGArea::AreaC000);
        }
        else
        {
            switch16kPrg(m_prgHijackedBit, PRGArea::Area8000);
            switch16kPrg((m_reg[3] & 0xF) | m_prgHijackedBit, PRGArea::AreaC000);
        }
    }
}
