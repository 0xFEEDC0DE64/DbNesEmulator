#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>
#include <QString>
#include <QVector>

// system includes
#include <array>

// local includes
#include "rom.h"
#include "enums/prgarea.h"
#include "enums/chrarea.h"

// forward declarations
class QDataStream;

class NesEmulator;

class NESCORELIB_EXPORT Board
{
    Q_DISABLE_COPY(Board)

public:
    explicit Board(NesEmulator &emu, const Rom &rom);
    virtual ~Board();

    virtual QString name() const;
    virtual quint8 mapper() const;

    virtual void hardReset();
    virtual void softReset();

    virtual quint8 _readEx(quint16 address);
    virtual quint8 readEx(quint16 address);
    virtual void writeEx(quint16 address, quint8 value);
    virtual quint8 _readSrm(quint16 address);
    virtual quint8 readSrm(quint16 address);
    virtual void writeSrm(quint16 address, quint8 value);
    virtual quint8 _readPrg(quint16 address);
    virtual quint8 readPrg(quint16 address);
    virtual void writePrg(quint16 address, quint8 value);
    virtual quint8 _readChr(quint16 address);
    virtual quint8 readChr(quint16 address);
    virtual void writeChr(quint16 address, quint8 value);
    virtual quint8 _readNmt(quint16 address);
    virtual quint8 readNmt(quint16 address);
    virtual void writeNmt(quint16 address, quint8 value);

    virtual void onPpuAddressUpdate(quint16 address);
    virtual void onCpuClock();
    virtual void onPpuClock();
    virtual void onPpuA12RaisingEdge();
    virtual void onPpuScanlineTick();
    virtual void onApuClockDuration();
    virtual void onApuClockEnvelope();
    virtual void onApuClockSingle();
    virtual void onApuClock();
    virtual double apuGetSample() const;
    virtual void apuApplyChannelsSettings();

    virtual void readState(QDataStream &dataStream);
    virtual void writeState(QDataStream &dataStream) const;

    virtual bool enableExternalSound() const;

protected:
    virtual int prgRam8KbDefaultBlkCount() const;
    virtual int chrRom1KbDefaultBlkCount() const;
    virtual bool ppuA12ToggleTimerEnabled() const;
    virtual bool ppuA12TogglesOnRaisingEdge() const;

    void switch4kPrg(int index, PRGArea area);
    void switch8kPrg(int index, PRGArea area);
    void switch16kPrg(int index, PRGArea area);
    void switch32kPrg(int index, PRGArea area);
    void toggle4kPrgRam(bool ram, PRGArea area);
    void toggle8kPrgRam(bool ram, PRGArea area);
    void toggle16kPrgRam(bool ram, PRGArea area);
    void toggle32kPrgRam(bool ram, PRGArea area);
    void togglePrgRamEnable(bool enable);
    void togglePrgRamWritableEnable(bool enable);
    void toggle4kPrgRamEnabled(bool enable, int index);
    void toggle4kPrgRamWritable(bool enable, int index);
    void toggle4kPrgRamBattery(bool enable, int index);
    void switch1kChr(int index, CHRArea area);
    void switch2kChr(int index, CHRArea area);
    void switch4kChr(int index, CHRArea area);
    void switch8kChr(int index);
    void toggle1kChrRam(bool ram, CHRArea area);
    void toggle2kChrRam(bool ram, CHRArea area);
    void toggle4kChrRam(bool ram, CHRArea area);
    void toggle8kChrRam(bool ram);
    void toggle1kChrRamEnabled(bool enable, int index);
    void toggle1kChrRamWritable(bool enable, int index);
    void toggleChrRamWritableEnable(bool enable);
    void toggle1kChrRamBattery(bool enable, int index);
    void switch1kNmt(int index, quint8 area);
    void switch1kNmt(Mirroring mirroring);

    int prgRom4KbCount() const { return m_rom.prg.size(); }
    int prgRom4KbMask() const { return prgRom4KbCount() - 1; }
    int prgRom8KbCount() const { return prgRom4KbCount() / 2; }
    int prgRom8KbMask() const { return prgRom8KbCount() - 1; }
    int prgRom16KbCount() const { return prgRom4KbCount() / 4; }
    int prgRom16KbMask() const { return prgRom16KbCount() - 1; }
    int prgRom32KbCount() const { return prgRom4KbCount() / 8; }
    int prgRom32KbMask() const { return prgRom32KbCount() - 1; }
    int prgRam4KbCount() const { return m_prgRam.size(); }
    int prgRam4KbMask() const { return prgRam4KbCount() - 1; }
    int prgRam8KbCount() const { return prgRam4KbCount() / 2; }
    int prgRam8KbMask() const { return prgRam8KbCount() - 1; }
    int prgRam16KbCount() const { return prgRam4KbCount() / 4; }
    int prgRam16KbMask() const { return prgRam16KbCount() - 1; }
    int prgRam32KbCount() const { return prgRam4KbCount() / 8; }
    int prgRam32KbMask() const { return prgRam32KbCount() - 1; }
    int chrRom1KbCount() const { return m_rom.chr.size(); }
    int chrRom1KbMask() const { return chrRom1KbCount() - 1; }
    int chrRom2KbCount() const { return chrRom1KbCount() / 2; }
    int chrRom2KbMask() const { return chrRom2KbCount() - 1; }
    int chrRom4KbCount() const { return chrRom1KbCount() / 4; }
    int chrRom4KbMask() const { return chrRom4KbCount() - 1; }
    int chrRom8KbCount() const { return chrRom1KbCount() / 8; }
    int chrRom8KbMask() const { return chrRom8KbCount() - 1; }
    int chrRam1KbCount() const { return m_chrRam.size(); }
    int chrRam1KbMask() const { return chrRam1KbCount() - 1; }
    int chrRam2KbCount() const { return chrRam1KbCount() / 2; }
    int chrRam2KbMask() const { return chrRam2KbCount() - 1; }
    int chrRam4KbCount() const { return chrRam1KbCount() / 4; }
    int chrRam4KbMask() const { return chrRam4KbCount() - 1; }
    int chrRam8KbCount() const { return chrRam1KbCount() / 8; }
    int chrRam8KbMask() const { return chrRam8KbCount() - 1; }

    template<std::size_t L>
    struct RamPage {
        std::array<quint8, L> ram {}; // The prg RAM blocks, 4KB (0x1000) each.
        bool enabled {}; // Indicates if a block is enabled (disabled ram blocks cannot be accessed, either read nor write)
        bool writeable {}; // Indicates if a block is writable (false means writes are not accepted even if this block is RAM)
        bool battery {}; // Indicates if a block is battery (RAM block battery will be saved to file on emu shutdown)
    };

    struct AreaBlk {
        bool ram {}; // Indicates if a blk is RAM (true) or ROM (false)
        int index {}; // The index of RAM/ROM block in the area
    };

    struct NmtRam {
        std::array<quint8, 0x400> ram {};
        int index {}; // The index of NMT RAM block in the area
    };

    QVector<RamPage<0x1000> > m_prgRam {};
    std::array<AreaBlk, 16> m_prgAreaBlk {}; // Starting from 4xxx to Fxxx, each entry here configure a 8kb block area

    QVector<RamPage<0x400> > m_chrRam {};
    std::array<AreaBlk, 8> m_chrAreaBlk {}; // Starting from 0000 to F3FF, each entry here configure a 4kb block area

    std::array<NmtRam, 4> m_nmtRam {};

    int m_oldVramAddress {};
    int m_newVramAddress {};
    int m_ppuCyclesTimer {};

protected:
    NesEmulator &m_emu;
    const Rom m_rom;

private:

    bool m_sramSaveRequired {};
};
