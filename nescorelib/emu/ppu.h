#pragma once

#include "nescorelib_global.h"
#include <QObject>

// Qt includes
#include <QtGlobal>

// system includes
#include <array>

// forward declarations
class NesEmulator;
class QDataStream;

class NESCORELIB_EXPORT Ppu : public QObject
{
    Q_OBJECT

public:
    static constexpr quint32 SCREEN_WIDTH = 256;
    static constexpr quint32 SCREEN_HEIGHT = 240;

    explicit Ppu(NesEmulator &emu);

    void hardReset();
    void clock();

    // scanlines
    void scanlineRender();
    void scanlineVBlankStart();
    void scanlineVBlankEnd();
    void scanlineVBlank();

    // bkg fetches
    void bkgFetch0();
    void bkgFetch1();
    void bkgFetch2();
    void bkgFetch3();
    void bkgFetch4();
    void bkgFetch5();
    void bkgFetch6();
    void bkgFetch7();

    // spr fetches
    void sprFetch0();
    void sprFetch1();
    void sprFetch2();
    void sprFetch3();

    // oam evaluation
    void oamReset();
    void oamClear();
    void oamEvFetch();
    void oamPhase0();
    void oamPhase1();
    void oamPhase2();
    void oamPhase3();
    void oamPhase4();
    void oamPhase5();
    void oamPhase6();
    void oamPhase7();
    void oamPhase8();

    void renderPixel();

    // io
    quint8 _ioRead(const quint16 address);
    quint8 ioRead(const quint16 address);
    void ioWrite(const quint16 address, const quint8 value);

    void onRegister2000();
    void onRegister2001();
    void onRegister2002();
    void onRegister2003();
    void onRegister2004();
    void onRegister2005();
    void onRegister2006();
    void onRegister2007();
    void read2000();
    void read2001();
    void read2002();
    void read2003();
    void read2004();
    void read2005();
    void read2006();
    void read2007();

    bool isRenderingOn() const;
    bool isInRender() const;

    void readState(QDataStream &dataStream);
    void writeState(QDataStream &dataStream) const;

    const std::array<qint32, SCREEN_WIDTH*SCREEN_HEIGHT> &screenPixels() const;

Q_SIGNALS:
    void frameFinished(const std::array<qint32, SCREEN_WIDTH*SCREEN_HEIGHT> &frame);

private:
    NesEmulator &m_emu;

    std::array<quint8, 512> m_ppuBkgPixels {};
    std::array<qint32, SCREEN_WIDTH> m_ppuSprPixels {};
    std::array<qint32, SCREEN_WIDTH*SCREEN_HEIGHT> m_ppuScreenPixels {};

    // Clocks
    qint32 m_ppuClockH {};
    quint16 m_ppuClockV {};
    bool m_ppuUseOddSwap {};
    bool m_ppuIsNmiTime {};

    // Memory
    std::array<quint8, SCREEN_WIDTH> m_ppuOamBank {};
    std::array<quint8, 32> m_ppuOamBankSecondary {};
    std::array<quint8, 32> m_ppuPaletteBank {};

    // Data Reg
    quint8 m_ppuRegIoDb {}; //The data bus
    quint8 m_ppuRegIoAddr {}; //The address bus (only first 3 bits are used, will be ranged 0-7)
    bool m_ppuRegAccessHappened {}; //Triggers when cpu accesses ppu bus.
    bool m_ppuRegAccessW {}; //True= write access, False= Read access.

    // 0x2000 register values
    quint8 m_ppuReg2000VramAddressIncreament {};
    quint16 m_ppuReg2000SpritePatternTableAddressFor8x8Sprites {};
    quint16 m_ppuReg2000BackgroundPatternTableAddress {};
    quint8 m_ppuReg2000SpriteSize {};
    bool m_ppuReg2000Vbi {};

    // 0x2001 register values
    bool m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen {};
    bool m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen {};
    bool m_ppuReg2001ShowBackground {};
    bool m_ppuReg2001ShowSprites {};
    qint32 m_ppuReg2001Grayscale {};
    qint32 m_ppuReg2001Emphasis {};

    // 0x2002 register values.
    bool m_ppuReg2002SpriteOverflow {};
    bool m_ppuReg2002Sprite0Hit {};
    bool m_ppuReg2002VblankStartedFlag {};

    // 0x2003 register values.
    quint8 m_ppuReg2003OamAddr {};

    // VRAM
    quint16 m_ppuVramAddr {};
    quint8 m_ppuVramData {};
    quint16 m_ppuVramAddrTemp {};
    quint16 m_ppuVramAddrAccessTemp {};
    bool m_ppuVramFlipFlop {};
    quint8 m_ppuVramFinex {};

    // Fetches
    quint16 m_ppuBkgfetchNtAddr {};
    quint8 m_ppuBkgfetchNtData {};
    quint16 m_ppuBkgfetchAtAddr {};
    quint8 m_ppuBkgfetchAtData {};
    quint16 m_ppuBkgfetchLbAddr {};
    quint8 m_ppuBkgfetchLbData {};
    quint16 m_ppuBkgfetchHbAddr {};
    quint8 m_ppuBkgfetchHbData {};

    qint32 m_ppuSprfetchSlot {};
    quint8 m_ppuSprfetchYData {};
    quint8 m_ppuSprfetchTData {};
    quint8 m_ppuSprfetchAtData {};
    quint8 m_ppuSprfetchXData {};
    quint16 m_ppuSprfetchLbAddr {};
    quint8 m_ppuSprfetchLbData {};
    quint16 m_ppuSprfetchHbAddr {};
    quint8 m_ppuSprfetchHbData {};

    bool m_ppuIsSprfetch {};

    // Renderer helper
    qint32 m_ppuColorAnd {};
    // OAM
    quint8 m_ppuOamEvN {};
    quint8 m_ppuOamEvM {};
    bool m_ppuOamevCompare {};
    quint8 m_ppuOamevSlot {};
    quint8 m_ppuFetchData {};
    quint8 m_ppuPhaseIndex {};
    bool m_ppuSprite0ShouldHit {};
};
