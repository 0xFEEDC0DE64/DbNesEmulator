#include "board.h"

// system includes
#include <algorithm>

// local includes
#include "rom.h"

Board::Board(NesEmulator &emu, const Rom &rom) :
    m_emu(emu),
    m_rom(rom)
{
    // PRG RAM
    m_sramSaveRequired = false;

    /* foreach from db */
    {
        const int SIZE = prgRam8KbDefaultBlkCount() * 8;
        const bool BATTERY = true;

        if (BATTERY)
            m_sramSaveRequired = true;

        if (SIZE > 0)
        {
            int kb4_count = SIZE / 2;
            for (int i = 0; i < kb4_count; i++)
                m_prgRam.append({
                    /* .ram = */ {},
                    /* .enabled = */ true,
                    /* .writeable = */ true,
                    /* .battery = */ BATTERY
                });
        }
    }

    // TRAINER, it should be copied into the RAM blk at 0x7000. In this case, it should be copied into ram blk 3
    if (rom.hasTrainer)
        std::copy(std::begin(rom.trainer), std::end(rom.trainer), std::begin(m_prgRam[3].ram));

    // CHR RAM
    // Map 8 Kb for now
    const int chr_ram_banks_1k = chrRom1KbDefaultBlkCount(); // from db
    m_chrRam.reserve(chr_ram_banks_1k);
    for (int i = 0; i < chr_ram_banks_1k; i++)
        m_chrRam.append({
            /* .ram = */ {},
            /* .enabled = */ true,
            /* .writeable = */ true,
            /* .battery = */ false
        });

    // 6 Nametables
}

Board::~Board()
{
}

QString Board::name() const
{
    throw std::runtime_error("has not been implemented");
}

quint8 Board::mapper() const
{
    throw std::runtime_error("has not been implemented");
}

void Board::hardReset()
{
    // Use the configuration of mapper 0 (NRAM).
    // PRG Switching
    // Toggle ram/rom
    // Switch 16KB ram, from 0x4000 into 0x7000
    toggle16kPrgRam(true, PRGArea::Area4000);
    switch16kPrg(0, PRGArea::Area4000);
    // Switch 32KB rom, from 0x8000 into 0xF000
    toggle32kPrgRam(false, PRGArea::Area8000);
    switch32kPrg(0, PRGArea::Area8000);

    // CHR Switching
    // Pattern tables
    toggle8kChrRam((chrRom1KbCount() == 0));
    switch8kChr(0);

    // Nametables
    switch1kNmt(m_rom.mirroring);
}

void Board::softReset()
{
}

quint8 Board::_readEx(quint16 address)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();
        if (m_prgRam[prgTmpIndex].enabled)
            return m_prgRam[prgTmpIndex].ram[address & 0xFFF];
        else
            return 0;
    }
    else
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRom4KbMask();
        return m_rom.prg[prgTmpIndex][address & 0xFFF];
    }
}

quint8 Board::readEx(quint16 address)
{
    auto result = _readEx(address);
    return result;
}

void Board::writeEx(quint16 address, quint8 value)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();

        if (m_prgRam[prgTmpIndex].enabled)
            if (m_prgRam[prgTmpIndex].writeable)
                m_prgRam[prgTmpIndex].ram[address & 0xFFF] = value;
    }
}

quint8 Board::_readSrm(quint16 address)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();
        if (m_prgRam[prgTmpIndex].enabled)
            return m_prgRam[prgTmpIndex].ram[address & 0xFFF];
        else
            return 0;
    }
    else
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRom4KbMask();
        return m_rom.prg[prgTmpIndex][address & 0xFFF];
    }
}

quint8 Board::readSrm(quint16 address)
{
    auto result = _readSrm(address);
    return result;
}

void Board::writeSrm(quint16 address, quint8 value)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();

        if (m_prgRam[prgTmpIndex].enabled)
            if (m_prgRam[prgTmpIndex].writeable)
                m_prgRam[prgTmpIndex].ram[address & 0xFFF] = value;
    }
}

quint8 Board::_readPrg(quint16 address)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        const int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();
        const int temp = address & 0xFFF;
        if (m_prgRam[prgTmpIndex].enabled)
            return m_prgRam[prgTmpIndex].ram[temp];
        else
            return 0;
    }
    else
    {
        const int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRom4KbMask();
        const int temp = address & 0xFFF;
        return m_rom.prg[prgTmpIndex][temp];
    }

    /*
    if (IsGameGenieActive)
    {
        foreach (GameGenieCode code in GameGenieCodes)
        {
            if (!code.Enabled)
                continue;

            if (code.Address != addr)
                continue;

            if (!code.IsCompare || code.Compare == return_value)
                return code.Value;
        }
    }
    */
}

quint8 Board::readPrg(quint16 address)
{
    auto result = _readPrg(address);
    return result;
}

void Board::writePrg(quint16 address, quint8 value)
{
    int prgTmpArea = address >> 12 & 0xF;
    if (m_prgAreaBlk[prgTmpArea].ram)
    {
        int prgTmpIndex = m_prgAreaBlk[prgTmpArea].index & prgRam4KbMask();

        if (m_prgRam[prgTmpIndex].enabled)
            if (m_prgRam[prgTmpIndex].writeable)
                m_prgRam[prgTmpIndex].ram[address & 0xFFF] = value;
    }
}

quint8 Board::_readChr(quint16 address)
{
    // 00-07 means patterntables
    // 08-11 means nametables, should not included
    // 12-15 nametables mirrors, should not included as well
    int chrTmpArea = (address >> 10) & 0x7;// 0x0000 - 0x1FFF, 0-7.
    int chrTmpIndex = m_chrAreaBlk[chrTmpArea].index;
    if (m_chrAreaBlk[chrTmpArea].ram)
    {
        chrTmpIndex &= chrRam1KbMask();
        if (m_chrRam[chrTmpIndex].enabled)
            return m_chrRam[chrTmpIndex].ram[address & 0x3FF];
        else
            return 0;
    }
    else
    {
        chrTmpIndex &= chrRom1KbMask();
        return m_rom.chr[chrTmpIndex][address & 0x3FF];
    }
}

quint8 Board::readChr(quint16 address)
{
    auto result = _readChr(address);
    return result;
}

void Board::writeChr(quint16 address, quint8 value)
{
    // 00-07 means patterntables
    // 08-11 means nametables, should not included
    // 12-15 nametables mirrors, should not included as well
    int chrTmpArea = (address >> 10) & 0x7;// 0x0000 - 0x1FFF, 0-7.

    if (m_chrAreaBlk[chrTmpArea].ram)
    {
        int chrTmpIndex = m_chrAreaBlk[chrTmpArea].index & chrRam1KbMask();
        if (m_chrRam[chrTmpIndex].enabled)
            if (m_chrRam[chrTmpIndex].writeable)
                m_chrRam[chrTmpIndex].ram[address & 0x3FF] = value;
    }
}

quint8 Board::_readNmt(quint16 address)
{
    int nmtTmpArea = (address >> 10) & 0x3;// 0x2000 - 0x2C00, 0-3.
    int nmtTmpIndex = m_nmtRam[nmtTmpArea].index;

    return m_nmtRam[nmtTmpIndex].ram[address & 0x3FF];
}

quint8 Board::readNmt(quint16 address)
{
    auto result = _readNmt(address);
    return result;
}

void Board::writeNmt(quint16 address, quint8 value)
{
    int nmtTmpArea = (address >> 10) & 0x3;// 0x2000 - 0x2C00, 0-3.
    int nmtTmpIndex = m_nmtRam[nmtTmpArea].index;

    m_nmtRam[nmtTmpIndex].ram[address & 0x3FF] = value;
}

void Board::onPpuAddressUpdate(quint16 address)
{
    if (ppuA12ToggleTimerEnabled())
    {
        m_oldVramAddress = m_newVramAddress;
        m_newVramAddress = address & 0x1000;
        if (ppuA12TogglesOnRaisingEdge())
        {
            if (m_oldVramAddress < m_newVramAddress)
            {
                if (m_ppuCyclesTimer > 8)
                    onPpuA12RaisingEdge();

                m_ppuCyclesTimer = 0;
            }
        }
        else
        {
            if (m_oldVramAddress > m_newVramAddress)
            {
                if (m_ppuCyclesTimer > 8)
                    onPpuA12RaisingEdge();

                m_ppuCyclesTimer = 0;
            }
        }
    }
}

void Board::onCpuClock()
{
}

void Board::onPpuClock()
{
    if (ppuA12ToggleTimerEnabled())
        m_ppuCyclesTimer++;
}

void Board::onPpuA12RaisingEdge()
{
}

void Board::onPpuScanlineTick()
{
}

void Board::onApuClockDuration()
{
}

void Board::onApuClockEnvelope()
{
}

void Board::onApuClockSingle()
{
}

void Board::onApuClock()
{
}

double Board::apuGetSample() const
{
    return 0.;
}

void Board::apuApplyChannelsSettings()
{
}

void Board::readState(QDataStream &dataStream)
{
    Q_UNUSED(dataStream)
}

void Board::writeState(QDataStream &dataStream) const
{
    Q_UNUSED(dataStream)
}

int Board::prgRam8KbDefaultBlkCount() const
{
    return 1;
}

int Board::chrRom1KbDefaultBlkCount() const
{
    return 8;
}

bool Board::ppuA12ToggleTimerEnabled() const
{
    return false;
}

bool Board::ppuA12TogglesOnRaisingEdge() const
{
    return false;
}

void Board::switch4kPrg(int index, PRGArea area)
{
    m_prgAreaBlk[int(area)].index = index;
}

void Board::switch8kPrg(int index, PRGArea area)
{
    index *= 2;
    m_prgAreaBlk[int(area)].index = index;
    m_prgAreaBlk[int(area) + 1].index = index + 1;
}

void Board::switch16kPrg(int index, PRGArea area)
{
    index *= 4;
    m_prgAreaBlk[int(area)].index = index;
    m_prgAreaBlk[int(area) + 1].index = index + 1;
    m_prgAreaBlk[int(area) + 2].index = index + 2;
    m_prgAreaBlk[int(area) + 3].index = index + 3;
}

void Board::switch32kPrg(int index, PRGArea area)
{
    index *= 8;
    m_prgAreaBlk[int(area)].index = index;
    m_prgAreaBlk[int(area) + 1].index = index + 1;
    m_prgAreaBlk[int(area) + 2].index = index + 2;
    m_prgAreaBlk[int(area) + 3].index = index + 3;
    m_prgAreaBlk[int(area) + 4].index = index + 4;
    m_prgAreaBlk[int(area) + 5].index = index + 5;
    m_prgAreaBlk[int(area) + 6].index = index + 6;
    m_prgAreaBlk[int(area) + 7].index = index + 7;
}

void Board::toggle4kPrgRam(bool ram, PRGArea area)
{
    m_prgAreaBlk[int(area)].ram = ram;
}

void Board::toggle8kPrgRam(bool ram, PRGArea area)
{
    m_prgAreaBlk[int(area)].ram = ram;
    m_prgAreaBlk[int(area) + 1].ram = ram;
}

void Board::toggle16kPrgRam(bool ram, PRGArea area)
{
    m_prgAreaBlk[int(area)].ram = ram;
    m_prgAreaBlk[int(area) + 1].ram = ram;
    m_prgAreaBlk[int(area) + 2].ram = ram;
    m_prgAreaBlk[int(area) + 3].ram = ram;
}

void Board::toggle32kPrgRam(bool ram, PRGArea area)
{
    m_prgAreaBlk[int(area)].ram = ram;
    m_prgAreaBlk[int(area) + 1].ram = ram;
    m_prgAreaBlk[int(area) + 2].ram = ram;
    m_prgAreaBlk[int(area) + 3].ram = ram;
    m_prgAreaBlk[int(area) + 4].ram = ram;
    m_prgAreaBlk[int(area) + 5].ram = ram;
    m_prgAreaBlk[int(area) + 6].ram = ram;
    m_prgAreaBlk[int(area) + 7].ram = ram;
}

void Board::togglePrgRamEnable(bool enable)
{
    for(auto &page : m_prgRam)
        page.enabled = enable;
}

void Board::togglePrgRamWritableEnable(bool enable)
{
    for(auto &page : m_prgRam)
        page.writeable = enable;
}

void Board::toggle4kPrgRamEnabled(bool enable, int index)
{
    m_prgRam[index].enabled = enable;
}

void Board::toggle4kPrgRamWritable(bool enable, int index)
{
    m_prgRam[index].writeable = enable;
}

void Board::toggle4kPrgRamBattery(bool enable, int index)
{
    m_prgRam[index].battery = enable;
}

void Board::switch1kChr(int index, CHRArea area)
{
    m_chrAreaBlk[int(area)].index = index;
}

void Board::switch2kChr(int index, CHRArea area)
{
    index *= 2;
    m_chrAreaBlk[int(area)].index = index;
    m_chrAreaBlk[int(area) + 1].index = index + 1;
}

void Board::switch4kChr(int index, CHRArea area)
{
    index *= 4;
    m_chrAreaBlk[int(area)].index = index;
    m_chrAreaBlk[int(area) + 1].index = index + 1;
    m_chrAreaBlk[int(area) + 2].index = index + 2;
    m_chrAreaBlk[int(area) + 3].index = index + 3;
}

void Board::switch8kChr(int index)
{
    index *= 8;
    for(int i = 0; i < 8; i++)
        m_chrAreaBlk[i].index = index + i;
}

void Board::toggle1kChrRam(bool ram, CHRArea area)
{
    m_chrAreaBlk[int(area)].ram = ram;
}

void Board::toggle2kChrRam(bool ram, CHRArea area)
{
    m_chrAreaBlk[int(area)].ram = ram;
    m_chrAreaBlk[int(area) + 1].ram = ram;
}

void Board::toggle4kChrRam(bool ram, CHRArea area)
{
    m_chrAreaBlk[int(area)].ram = ram;
    m_chrAreaBlk[int(area) + 1].ram = ram;
    m_chrAreaBlk[int(area) + 2].ram = ram;
    m_chrAreaBlk[int(area) + 3].ram = ram;
}

void Board::toggle8kChrRam(bool ram)
{
    for(auto &areaBlk : m_chrAreaBlk)
        areaBlk.ram = ram;
}

void Board::toggle1kChrRamEnabled(bool enable, int index)
{
    m_chrRam[index].enabled = enable;
}

void Board::toggle1kChrRamWritable(bool enable, int index)
{
    m_chrRam[index].writeable = enable;
}

void Board::toggleChrRamWritableEnable(bool enable)
{
    for (auto &ramPage : m_chrRam)
        ramPage.writeable = enable;
}

void Board::toggle1kChrRamBattery(bool enable, int index)
{
    m_chrRam[index].battery = enable;
}

void Board::switch1kNmt(int index, quint8 area)
{
    m_nmtRam[area].index = index;
}

void Board::switch1kNmt(Mirroring mirroring)
{
    // Mirroring value:
    // 0000 0000
    // ddcc bbaa
    // aa: index for area 0x2000
    // bb: index for area 0x2400
    // cc: index for area 0x2800
    // dd: index for area 0x2C00
    m_nmtRam[0].index = int(mirroring) & 0x3;
    m_nmtRam[1].index = (int(mirroring) >> 2) & 0x3;
    m_nmtRam[2].index = (int(mirroring) >> 4) & 0x3;
    m_nmtRam[3].index = (int(mirroring) >> 6) & 0x3;
}

bool Board::enableExternalSound() const
{
    return false;
}
