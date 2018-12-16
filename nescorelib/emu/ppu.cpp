#include "ppu.h"

// Qt includes
#include <QDataStream>

// dbcorelib includes
#include "utils/datastreamutils.h"

// local includes
#include "nesemulator.h"
#include "emusettings.h"

Ppu::Ppu(NesEmulator &emu) :
    QObject(&emu),
    m_emu(emu)
{
}

void Ppu::hardReset()
{
    m_ppuReg2001Grayscale = 0xF3;

    // oam
    m_ppuOamBank.fill(0);
    m_ppuOamBankSecondary.fill(0);
    oamReset();

    // pallettes
    m_ppuPaletteBank = {
        0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C, // Bkg palette
        0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08  // Spr palette
    };
    //ppu_palette = PaletteFileWrapper.LoadFile("");

    m_ppuColorAnd = m_ppuReg2001Grayscale | m_ppuReg2001Emphasis;

    m_ppuRegIoDb = 0;
    m_ppuRegIoAddr = 0;
    m_ppuRegAccessHappened = false;
    m_ppuRegAccessW = false;

    m_ppuReg2000VramAddressIncreament = 1;
    m_ppuReg2000SpritePatternTableAddressFor8x8Sprites = 0;
    m_ppuReg2000BackgroundPatternTableAddress = 0;
    m_ppuReg2000SpriteSize = 0;
    m_ppuReg2000Vbi = false;

    m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen = false;
    m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen = false;
    m_ppuReg2001ShowBackground = false;
    m_ppuReg2001ShowSprites = false;
    m_ppuReg2001Grayscale = 0;
    m_ppuReg2001Emphasis = 0;

    m_ppuReg2002SpriteOverflow = false;
    m_ppuReg2002Sprite0Hit = false;
    m_ppuReg2002VblankStartedFlag = false;

    m_ppuReg2003OamAddr = 0;

    m_ppuIsSprfetch = false;
}

void Ppu::clock()
{
    static constexpr std::array<void (Ppu::*)(), 320> ppuVClocks = []() {
        std::array<void (Ppu::*)(), 320> ppuVClocks {};

        for(std::size_t i = 0; i < SCREEN_HEIGHT; i++)
            ppuVClocks[i] = &Ppu::scanlineRender;

        ppuVClocks[SCREEN_HEIGHT] = &Ppu::scanlineVBlank;

        if(EmuSettings::region == EmuRegion::DENDY)
            for(std::size_t i = 241; i <= 290; i++)
                ppuVClocks[i] = &Ppu::scanlineVBlank;

        ppuVClocks[EmuSettings::ppuClockVBlankStart] = &Ppu::scanlineVBlankStart;

        for(std::size_t i = EmuSettings::ppuClockVBlankStart + 1; i <= EmuSettings::ppuClockVBlankEnd - 1; i++)
            ppuVClocks[i] = &Ppu::scanlineVBlank;

        ppuVClocks[EmuSettings::ppuClockVBlankEnd] = &Ppu::scanlineVBlankEnd;

        return ppuVClocks;
    }();

    static constexpr std::array<void (Ppu::*)(), 8> ppuRegUpdateFuncs {
        &Ppu::onRegister2000, &Ppu::onRegister2001, &Ppu::onRegister2002, &Ppu::onRegister2003,
        &Ppu::onRegister2004, &Ppu::onRegister2005, &Ppu::onRegister2006, &Ppu::onRegister2007
    };

    m_emu.memory().board()->onPpuClock();

    // Clock a scanline
    const auto callback = ppuVClocks[m_ppuClockV];
    (this->*callback)();

    // Advance
    if(m_ppuClockH == 340)
    {
        m_emu.memory().board()->onPpuScanlineTick();

        // Advance scanline ...
        if(m_ppuClockV == EmuSettings::ppuClockVBlankEnd)
        {
            m_ppuClockV = 0;

            Q_EMIT frameFinished(m_ppuScreenPixels);
        }
        else
            m_ppuClockV++;
        m_ppuClockH = -1;
    }
    m_ppuClockH++;

    /* THEORY:
     * After read/write (io access at 0x200x), the registers take effect at the end of the ppu cycle.
     * That's why we have the vbl and nmi effects on writing at 0x2000 and reading from 0x2002.
     * Same forother registers. First, when cpu access the IO at 0x200x, a data register sets from cpu written value.
     * At the end of the next ppu clock, ppu check out what happened in IO, then updates flags and other stuff.
     * forwrites, first cpu sets the data bus, then AT THE END OF PPU CYCLE, update flags and data from the data bus to use at the next cycle.
     * forreads, first update the data bus with flags and data, then return the value to cpu.
     * After that, AT THE END OF PPU CYCLE, ppu acknowledge the read: flags get reset, data updated ...etc
     */
    if(m_ppuRegAccessHappened)
    {
        m_ppuRegAccessHappened = false;
        (this->*ppuRegUpdateFuncs[m_ppuRegIoAddr])();
    }
}

void Ppu::scanlineRender()
{
    static constexpr std::array<void (Ppu::*)(), 8> ppuBkgFetches {
        &Ppu::bkgFetch0, &Ppu::bkgFetch1, &Ppu::bkgFetch2, &Ppu::bkgFetch3,
        &Ppu::bkgFetch4, &Ppu::bkgFetch5, &Ppu::bkgFetch6, &Ppu::bkgFetch7
    };

    static constexpr std::array<void (Ppu::*)(), 8> ppuSprFetches {
        &Ppu::bkgFetch0, &Ppu::bkgFetch1, &Ppu::bkgFetch2, &Ppu::bkgFetch3,
        &Ppu::sprFetch0, &Ppu::sprFetch1, &Ppu::sprFetch2, &Ppu::sprFetch3
    };

    static constexpr std::array<void (Ppu::*)(), 9> ppuOamPhases {
        &Ppu::oamPhase0, &Ppu::oamPhase1, &Ppu::oamPhase2, &Ppu::oamPhase3, &Ppu::oamPhase4,
        &Ppu::oamPhase5, &Ppu::oamPhase6, &Ppu::oamPhase7, &Ppu::oamPhase8
    };

    // 0 - 239 scanlines and pre-render scanline 261
    if(m_ppuClockH > 0)
    {
        if(m_ppuReg2001ShowBackground || m_ppuReg2001ShowSprites)
        {
            if(m_ppuClockH < SCREEN_WIDTH + 1)
            {
                // H clocks 1 - 256
                // OAM evaluation doesn't occur on pre-render scanline.
                if(m_ppuClockV != EmuSettings::ppuClockVBlankEnd)
                {
                    // Sprite evaluation
                    if(m_ppuClockH < 65)
                    {
                        m_ppuOamBankSecondary[(m_ppuClockH - 1) & 0x1F] = 0xFF;
                    }
                    else
                    {
                        if(m_ppuClockH == 65)
                            oamReset();

                        if((m_ppuClockH & 1) == 1)
                            oamEvFetch();
                        else
                            (this->*ppuOamPhases[m_ppuPhaseIndex])();

                        if(m_ppuClockH == SCREEN_WIDTH)
                            oamClear();
                    }
                }

                // BKG fetches
                (this->*ppuBkgFetches[(m_ppuClockH - 1) & 7])();

                if(m_ppuClockH < SCREEN_WIDTH + 1)
                    renderPixel();
            }
            else if(m_ppuClockH < 321)
            {
                // H clocks 256 - 320
                // BKG garbage fetches and sprite fetches
                (this->*ppuSprFetches[(m_ppuClockH - 1) & 7])();

                if(m_ppuClockH == SCREEN_WIDTH + 1)
                    m_ppuVramAddr = (m_ppuVramAddr & 0x7BE0) | (m_ppuVramAddrTemp & 0x041F);

                if(m_ppuClockV == EmuSettings::ppuClockVBlankEnd && m_ppuClockH >= 280 && m_ppuClockH <= 304)
                    m_ppuVramAddr = (m_ppuVramAddr & 0x041F) | (m_ppuVramAddrTemp & 0x7BE0);
            }
            else
            {
                // 321 - 340
                // BKG dummy fetch
                (this->*ppuBkgFetches[(m_ppuClockH - 1) & 7])();
            }
        }
        else if(m_ppuClockV < SCREEN_HEIGHT && m_ppuClockH < SCREEN_WIDTH + 1)
        {
            // Rendering is off, draw color at vram address ifit in range 0x3F00 - 0x3FFF
            if((m_ppuVramAddr & 0x3F00) == 0x3F00)
            {
                if((m_ppuVramAddr & 0x03) == 0)
                {
                    const auto index1 = m_ppuClockH - 1 + (m_ppuClockV * SCREEN_WIDTH);
                    const auto index2 = m_ppuPaletteBank[m_ppuVramAddr & 0x0C] & m_ppuColorAnd;
                    const auto value = EmuSettings::Video::palette[index2];
                    m_ppuScreenPixels[index1] = value;
                }
                else
                {
                    const auto index1 = m_ppuClockH - 1 + (m_ppuClockV * SCREEN_WIDTH);
                    const auto index2 = m_ppuPaletteBank[m_ppuVramAddr & 0x1F] & m_ppuColorAnd;
                    const auto value = EmuSettings::Video::palette[index2];
                    m_ppuScreenPixels[index1] = value;
                }
            }
            else
            {
                const auto index1 = m_ppuClockH - 1 + (m_ppuClockV * SCREEN_WIDTH);
                const auto index2 = m_ppuPaletteBank[0] & m_ppuColorAnd;
                const auto value = EmuSettings::Video::palette[index2];
                m_ppuScreenPixels[index1] = value;
            }
        }
    }// else is the idle clock
}

void Ppu::scanlineVBlankStart()
{
    // This is scanline 241
    m_ppuIsNmiTime = m_ppuClockH >= 1 && m_ppuClockH <= 4;
    if(m_ppuIsNmiTime)// 0 - 3
    {
        if(m_ppuClockH == 1)
            m_ppuReg2002VblankStartedFlag = true;

        m_emu.interrupts().setNmiCurrent(m_ppuReg2002VblankStartedFlag & m_ppuReg2000Vbi);
    }
}

void Ppu::scanlineVBlankEnd()
{
    // This is scanline 261, also called pre-render line
    m_ppuIsNmiTime = m_ppuClockH >= 1 && m_ppuClockH <= 4;

    // Pre-render line is here !
    if(m_ppuClockH > 0)
    {
        // Do a pre-render
        scanlineRender();

        if(m_ppuClockH == 1)
        {
            // Clear vbl flag
            m_ppuReg2002Sprite0Hit = false;
            m_ppuReg2002VblankStartedFlag = false;
            m_ppuReg2002SpriteOverflow = false;
        }

        if(EmuSettings::ppuUseOddCycle)
        {
            if(m_ppuClockH == 339)
            {
                // ODD Cycle
                m_ppuUseOddSwap = !m_ppuUseOddSwap;
                if(!m_ppuUseOddSwap & (m_ppuReg2001ShowBackground || m_ppuReg2001ShowSprites))
                    m_ppuClockH++;
            }
        }
    }
}

void Ppu::scanlineVBlank()
{
}

void Ppu::bkgFetch0()
{
    // Calculate NT address
    m_ppuBkgfetchNtAddr = 0x2000 | (m_ppuVramAddr & 0x0FFF);
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuBkgfetchNtAddr);
}

void Ppu::bkgFetch1()
{
    // Fetch NT data
    m_ppuBkgfetchNtData = m_emu.memory().board()->readNmt(m_ppuBkgfetchNtAddr);
}

void Ppu::bkgFetch2()
{
    // Calculate AT address
    m_ppuBkgfetchAtAddr = 0x23C0 | (m_ppuVramAddr & 0xC00) | ((m_ppuVramAddr >> 4) & 0x38) | ((m_ppuVramAddr >> 2) & 0x7);
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuBkgfetchAtAddr);
}

void Ppu::bkgFetch3()
{
    // Fetch AT data
    m_ppuBkgfetchAtData = m_emu.memory().board()->readNmt(m_ppuBkgfetchAtAddr);
    m_ppuBkgfetchAtData = m_ppuBkgfetchAtData >> ((m_ppuVramAddr >> 4 & 0x04) | (m_ppuVramAddr & 0x02));
}

void Ppu::bkgFetch4()
{
    // Calculate tile low-bit address
    m_ppuBkgfetchLbAddr = m_ppuReg2000BackgroundPatternTableAddress | (m_ppuBkgfetchNtData << 4) | (m_ppuVramAddr >> 12 & 7);
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuBkgfetchLbAddr);
}

void Ppu::bkgFetch5()
{
    // Fetch tile low-bit data
    m_ppuBkgfetchLbData = m_emu.memory().board()->readChr(m_ppuBkgfetchLbAddr);
}

void Ppu::bkgFetch6()
{
    // Calculate tile high-bit address
    m_ppuBkgfetchHbAddr = m_ppuReg2000BackgroundPatternTableAddress | (m_ppuBkgfetchNtData << 4) | 8 | (m_ppuVramAddr >> 12 & 7);
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuBkgfetchHbAddr);
}

void Ppu::bkgFetch7()
{
    // Fetch tile high-bit data
    m_ppuBkgfetchHbData = m_emu.memory().board()->readChr(m_ppuBkgfetchHbAddr);

    const auto ppuBkgRenderPos = m_ppuClockH > 320 ? m_ppuClockH - 327 : m_ppuClockH + 9;

    // Rendering background pixel
    for(auto i = 0; i < 8; i++)
    {
        const auto temp = ((m_ppuBkgfetchAtData << 2) & 0xC) | (m_ppuBkgfetchLbData >> 7 & 1) | (m_ppuBkgfetchHbData >> 6 & 2);
        if((temp & 3) != 0)
            m_ppuBkgPixels[i + ppuBkgRenderPos] = temp;
        else
            m_ppuBkgPixels[i + ppuBkgRenderPos] = 0;
        m_ppuBkgfetchLbData <<= 1;
        m_ppuBkgfetchHbData <<= 1;
    }

    // Increments
    if(m_ppuClockH == SCREEN_WIDTH)
    {
        // Increment Y
        if((m_ppuVramAddr & 0x7000) != 0x7000)
            m_ppuVramAddr += 0x1000;
        else
        {
            m_ppuVramAddr ^= 0x7000;

            switch (m_ppuVramAddr & 0x3E0)
            {
                case 0x3A0: m_ppuVramAddr ^= 0xBA0; break;
                case 0x3E0: m_ppuVramAddr ^= 0x3E0; break;
                default: m_ppuVramAddr += 0x20; break;
            }
        }
    }
    else
    {
        // Increment X
        if((m_ppuVramAddr & 0x001F) == 0x001F)
            m_ppuVramAddr ^= 0x041F;
        else
            m_ppuVramAddr++;
    }
}

void Ppu::sprFetch0()
{
    m_ppuSprfetchSlot = (((m_ppuClockH - 1) >> 3) & 7);
    m_ppuSprfetchYData = m_ppuOamBankSecondary[(m_ppuSprfetchSlot * 4)];
    m_ppuSprfetchTData = m_ppuOamBankSecondary[(m_ppuSprfetchSlot * 4) + 1];
    m_ppuSprfetchAtData = m_ppuOamBankSecondary[(m_ppuSprfetchSlot * 4) + 2];
    m_ppuSprfetchXData = m_ppuOamBankSecondary[(m_ppuSprfetchSlot * 4) + 3];

    const auto pputempcomparator = (m_ppuClockV - m_ppuSprfetchYData) ^ ((m_ppuSprfetchAtData & 0x80) != 0 ? 0x0F : 0x00);
    if(m_ppuReg2000SpriteSize == 0x10)
        m_ppuSprfetchLbAddr = (m_ppuSprfetchTData << 0x0C & 0x1000) | (m_ppuSprfetchTData << 0x04 & 0x0FE0) |
                              (pputempcomparator << 0x01 & 0x0010) | (pputempcomparator & 0x0007);
    else
        m_ppuSprfetchLbAddr = m_ppuReg2000SpritePatternTableAddressFor8x8Sprites | (m_ppuSprfetchTData << 0x04) | (pputempcomparator & 0x0007);

    m_emu.memory().board()->onPpuAddressUpdate(m_ppuSprfetchLbAddr);
}

void Ppu::sprFetch1()
{
    // Fetch tile low-bit data
    m_ppuIsSprfetch = true;
    m_ppuSprfetchLbData = m_emu.memory().board()->readChr(m_ppuSprfetchLbAddr);
    m_ppuIsSprfetch = false;
}

void Ppu::sprFetch2()
{
    m_ppuSprfetchHbAddr = m_ppuSprfetchLbAddr | 0x08;
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuSprfetchHbAddr);
}

void Ppu::sprFetch3()
{
    if(m_ppuSprfetchXData == 255)
        return;

    // Fetch tile high-bit data
    m_ppuIsSprfetch = true;
    m_ppuSprfetchHbData = m_emu.memory().board()->readChr(m_ppuSprfetchHbAddr);
    m_ppuIsSprfetch = false;

    // Render the sprite
    int ppuBkgRenderPos = m_ppuSprfetchXData;

    if((m_ppuSprfetchAtData & 0x40) == 0)
    {
        // Rendering sprite pixel
        for(auto i = 0; i < 8; i++)
        {
            if(ppuBkgRenderPos < 255)
            {
                const auto temp = ((m_ppuSprfetchAtData << 2) & 0xC) | (m_ppuSprfetchLbData >> 7 & 1) | (m_ppuSprfetchHbData >> 6 & 2);
                if((temp & 3) != 0 && (m_ppuSprPixels[ppuBkgRenderPos] & 3) == 0)
                    m_ppuSprPixels[ppuBkgRenderPos] = temp;

                if(m_ppuSprfetchSlot == 0 && m_ppuSprite0ShouldHit)
                {
                    //m_ppuSprite0ShouldHit = false;
                    m_ppuSprPixels[ppuBkgRenderPos] |= 0x4000;// Sprite 0
                }

                if((m_ppuSprfetchAtData & 0x20) == 0)
                    m_ppuSprPixels[ppuBkgRenderPos] |= 0x8000;

                m_ppuSprfetchLbData <<= 1;
                m_ppuSprfetchHbData <<= 1;

                ppuBkgRenderPos++;
            }
        }
    }
    else
    {
        // H flip
        for(auto i = 0; i < 8; i++)
        {
            if(ppuBkgRenderPos < 255)
            {
                const auto temp = ((m_ppuSprfetchAtData << 2) & 0xC) | (m_ppuSprfetchLbData & 1) | (m_ppuSprfetchHbData << 1 & 2);
                if((temp & 3) && (m_ppuSprPixels[ppuBkgRenderPos] & 3) == 0)
                    m_ppuSprPixels[ppuBkgRenderPos] = temp;

                if(m_ppuSprfetchSlot == 0 && m_ppuSprite0ShouldHit)
                {
                    //m_ppuSprite0ShouldHit = false;
                    m_ppuSprPixels[ppuBkgRenderPos] |= 0x4000;// Sprite 0
                }

                if((m_ppuSprfetchAtData & 0x20) == 0)
                    m_ppuSprPixels[ppuBkgRenderPos] |= 0x8000;

                m_ppuSprfetchLbData >>= 1;
                m_ppuSprfetchHbData >>= 1;

                ppuBkgRenderPos++;
            }
        }
    }
}

void Ppu::oamReset()
{
    m_ppuOamEvN = 0;
    m_ppuOamEvM = 0;
    m_ppuOamevSlot = 0;
    m_ppuPhaseIndex = 0;
    m_ppuSprite0ShouldHit = false;
}

void Ppu::oamClear()
{
    m_ppuSprPixels.fill(0);
}

void Ppu::oamEvFetch()
{
    m_ppuFetchData = m_ppuOamBank[(m_ppuOamEvN * 4) + m_ppuOamEvM];
}

void Ppu::oamPhase0()
{
    m_ppuOamevCompare = m_ppuClockV >= m_ppuFetchData && m_ppuClockV < m_ppuFetchData + m_ppuReg2000SpriteSize;

    // Check ifread data in range
    if(m_ppuOamevCompare)
    {
        m_ppuOamBankSecondary[(m_ppuOamevSlot * 4)] = m_ppuFetchData;
        m_ppuOamEvM = 1;
        m_ppuPhaseIndex++;
        if(m_ppuOamEvN == 0)
            m_ppuSprite0ShouldHit = true;
    }
    else
    {
        m_ppuOamEvM = 0;
        m_ppuOamEvN++;
        if(m_ppuOamEvN == 64)
        {
            m_ppuOamEvN = 0;
            m_ppuPhaseIndex = 8;
        }
    }
}

void Ppu::oamPhase1()
{
    m_ppuOamBankSecondary[(m_ppuOamevSlot * 4) + m_ppuOamEvM] = m_ppuFetchData;
    m_ppuOamEvM = 2;
    m_ppuPhaseIndex++;
}

void Ppu::oamPhase2()
{
    m_ppuOamBankSecondary[(m_ppuOamevSlot * 4) + m_ppuOamEvM] = m_ppuFetchData;
    m_ppuOamEvM = 3;
    m_ppuPhaseIndex++;
}

void Ppu::oamPhase3()
{
    m_ppuOamBankSecondary[(m_ppuOamevSlot * 4) + m_ppuOamEvM] = m_ppuFetchData;
    m_ppuOamEvM = 0;
    m_ppuOamEvN++;
    m_ppuOamevSlot++;

    if(m_ppuOamEvN == 64)
    {
        m_ppuOamEvN = 0;
        m_ppuPhaseIndex = 8;
    }
    else if(m_ppuOamevSlot < 8)
        m_ppuPhaseIndex = 0;
    else if(m_ppuOamevSlot == 8)
        m_ppuPhaseIndex = 4;
}

void Ppu::oamPhase4()
{
    m_ppuOamevCompare = m_ppuClockV >= m_ppuFetchData &&
                        m_ppuClockV < m_ppuFetchData + m_ppuReg2000SpriteSize;

    // Check ifread data in range
    if(m_ppuOamevCompare)
    {
        m_ppuOamEvM = 1;
        m_ppuPhaseIndex++;
        m_ppuReg2002SpriteOverflow = true;
    }
    else
    {
        m_ppuOamEvM++;

        if(m_ppuOamEvM == 4)
            m_ppuOamEvM = 0;

        m_ppuOamEvN++;

        if(m_ppuOamEvN == 64)
        {
            m_ppuOamEvN = 0;
            m_ppuPhaseIndex = 8;
        }
        else
            m_ppuPhaseIndex = 4;
    }
}

void Ppu::oamPhase5()
{
    m_ppuOamEvM = 2;
    m_ppuPhaseIndex++;
}

void Ppu::oamPhase6()
{
    m_ppuOamEvM = 3;
    m_ppuPhaseIndex++;
}

void Ppu::oamPhase7()
{
    m_ppuOamEvM = 0;
    m_ppuOamEvN++;
    if(m_ppuOamEvN == 64)
    {
        m_ppuOamEvN = 0;
        m_ppuPhaseIndex = 8;
    }
}

void Ppu::oamPhase8()
{
    m_ppuOamEvN++;
    if(m_ppuOamEvN >= 64)
        m_ppuOamEvN = 0;
}

void Ppu::renderPixel()
{
    if(m_ppuClockV == EmuSettings::ppuClockVBlankEnd)
        return;

    const auto ppuRenderX = m_ppuClockH - 1;

    int ppuBkgCurrentPixel;
    int ppuSprCurrentPixel;

    // Get the pixels.
    if(ppuRenderX < 8)
    {
        // This area is not rendered
        if(m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen)
            ppuBkgCurrentPixel = 0x3F00 | m_ppuBkgPixels[ppuRenderX + m_ppuVramFinex + 1];
        else
            ppuBkgCurrentPixel = 0x3F00;

        if(m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen && m_ppuClockV != 0)
            ppuSprCurrentPixel = 0x3F10 | (m_ppuSprPixels[ppuRenderX] & 0xFF);
        else
            ppuSprCurrentPixel = 0x3F10;
    }
    else
    {
        if(!m_ppuReg2001ShowBackground)
            ppuBkgCurrentPixel = 0x3F00;
        else
            ppuBkgCurrentPixel = 0x3F00 | m_ppuBkgPixels[ppuRenderX + m_ppuVramFinex + 1];

        if(!m_ppuReg2001ShowSprites || m_ppuClockV == 0)
            ppuSprCurrentPixel = 0x3F10;
        else
            ppuSprCurrentPixel = 0x3F10 | (m_ppuSprPixels[ppuRenderX] & 0xFF);

    }

    int ppuCurrentPixel;

    if((ppuBkgCurrentPixel & 3) == 0)
    {
        ppuCurrentPixel = ppuSprCurrentPixel;
        goto render;
    }

    if((ppuSprCurrentPixel & 3) == 0)
    {
        ppuCurrentPixel = ppuBkgCurrentPixel;
        goto render;
    }

    // Use priority
    if(m_ppuSprPixels[ppuRenderX] & 0x8000)
        ppuCurrentPixel = ppuSprCurrentPixel;
    else
        ppuCurrentPixel = ppuBkgCurrentPixel;

    // Sprite 0 Hit
    if(m_ppuSprPixels[ppuRenderX] & 0x4000)
        m_ppuReg2002Sprite0Hit = true;

render:
    if((ppuCurrentPixel & 0x03) == 0)
    {
        const auto index1 = ppuRenderX + (m_ppuClockV * SCREEN_WIDTH);
        const auto index2 = m_ppuPaletteBank[ppuCurrentPixel & 0x0C] & m_ppuColorAnd;
        const auto value = EmuSettings::Video::palette[index2];
        m_ppuScreenPixels[index1] = value;
    }
    else
    {
        const auto index1 = ppuRenderX + (m_ppuClockV * SCREEN_WIDTH);
        const auto index2 = m_ppuPaletteBank[ppuCurrentPixel & 0x1F] & m_ppuColorAnd;
        const auto value = EmuSettings::Video::palette[index2];
        m_ppuScreenPixels[index1] = value;
    }
}

quint8 Ppu::_ioRead(const quint16 address)
{
    static constexpr std::array<void (Ppu::*)(), 8> ppuRegReadFuncs {
        &Ppu::read2000, &Ppu::read2001, &Ppu::read2002, &Ppu::read2003,
        &Ppu::read2004, &Ppu::read2005, &Ppu::read2006, &Ppu::read2007
    };

    // PPU IO Registers. Emulating bus here.
    m_ppuRegIoAddr = address & 0x7;
    m_ppuRegAccessHappened = true;
    m_ppuRegAccessW = false;

    (this->*ppuRegReadFuncs[m_ppuRegIoAddr])();

    return m_ppuRegIoDb;
}

quint8 Ppu::ioRead(const quint16 address)
{
    auto result = _ioRead(address);
    return result;
}

void Ppu::ioWrite(const quint16 address, const quint8 value)
{
    // PPU IO Registers. Emulating bus here.
    m_ppuRegIoAddr = address & 0x7;
    m_ppuRegIoDb = value;
    m_ppuRegAccessW = true;
    m_ppuRegAccessHappened = true;
}

void Ppu::onRegister2000()
{
    // Only writes accepted
    if(!m_ppuRegAccessW)
        return;

    // Update vram address
    m_ppuVramAddrTemp = (m_ppuVramAddrTemp & 0x73FF) | ((m_ppuRegIoDb & 0x3) << 10);

    if((m_ppuRegIoDb & 4) != 0)
        m_ppuReg2000VramAddressIncreament = 32;
    else
        m_ppuReg2000VramAddressIncreament = 1;

    if((m_ppuRegIoDb & 0x8) != 0)
        m_ppuReg2000SpritePatternTableAddressFor8x8Sprites = 0x1000;
    else
        m_ppuReg2000SpritePatternTableAddressFor8x8Sprites = 0x0000;

    if((m_ppuRegIoDb & 0x10) != 0)
        m_ppuReg2000BackgroundPatternTableAddress = 0x1000;
    else
        m_ppuReg2000BackgroundPatternTableAddress = 0x0000;

    if((m_ppuRegIoDb & 0x20) != 0)
        m_ppuReg2000SpriteSize = 0x0010;
    else
        m_ppuReg2000SpriteSize = 0x0008;

    if(!m_ppuReg2000Vbi && ((m_ppuRegIoDb & 0x80) != 0))
    {
        if(m_ppuReg2002VblankStartedFlag)// Special case ! NMI can be enabled anytime ifvbl already set
            m_emu.interrupts().setNmiCurrent(true);
    }

    m_ppuReg2000Vbi = m_ppuRegIoDb & 0x80;

    if(!m_ppuReg2000Vbi)// NMI disable effect only at vbl set period (HClock between 1 and 3)
        if(m_ppuIsNmiTime)// 0 - 3
            m_emu.interrupts().setNmiCurrent(false);
}

void Ppu::onRegister2001()
{
    // Only writes accepted
    if(!m_ppuRegAccessW)
        return;

    m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen = m_ppuRegIoDb & 0x2;
    m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen = m_ppuRegIoDb & 0x4;
    m_ppuReg2001ShowBackground = m_ppuRegIoDb & 0x8;
    m_ppuReg2001ShowSprites = m_ppuRegIoDb & 0x10;

    m_ppuReg2001Grayscale = m_ppuRegIoDb & 0x01 ? 0x30 : 0x3F;
    m_ppuReg2001Emphasis = (m_ppuRegIoDb & 0xE0) << 1;

    m_ppuColorAnd = m_ppuReg2001Grayscale | m_ppuReg2001Emphasis;
}

void Ppu::onRegister2002()
{
    // Only reads accepted
    if(m_ppuRegAccessW)
        return;

    m_ppuVramFlipFlop = false;
    m_ppuReg2002VblankStartedFlag = false;

    if(m_ppuClockV == EmuSettings::ppuClockVBlankStart)
        m_emu.interrupts().setNmiCurrent(m_ppuReg2002VblankStartedFlag & m_ppuReg2000Vbi);
}

void Ppu::onRegister2003()
{
    // Only writes accepted
    if(!m_ppuRegAccessW)
        return;

    m_ppuReg2003OamAddr = m_ppuRegIoDb;
}

void Ppu::onRegister2004()
{
    if(m_ppuRegAccessW)
    {
        // ON Writes
        m_ppuOamBank[m_ppuReg2003OamAddr] = m_ppuRegIoDb;
        m_ppuReg2003OamAddr = (m_ppuReg2003OamAddr + 1) & 0xFF;
    }
    // Nothing happens on reads
}

void Ppu::onRegister2005()
{
    // Only writes accepted
    if(!m_ppuRegAccessW)
        return;

    if(!m_ppuVramFlipFlop)
    {
        m_ppuVramAddrTemp = (m_ppuVramAddrTemp & 0x7FE0) | ((m_ppuRegIoDb & 0xF8) >> 3);
        m_ppuVramFinex = m_ppuRegIoDb & 0x07;
    }
    else
        m_ppuVramAddrTemp = (m_ppuVramAddrTemp & 0x0C1F) | ((m_ppuRegIoDb & 0x7) << 12) | ((m_ppuRegIoDb & 0xF8) << 2);

    m_ppuVramFlipFlop = !m_ppuVramFlipFlop;
}

void Ppu::onRegister2006()
{
    // Only writes accepted
    if(!m_ppuRegAccessW)
        return;

    if(!m_ppuVramFlipFlop)
        m_ppuVramAddrTemp = (m_ppuVramAddrTemp & 0x00FF) | ((m_ppuRegIoDb & 0x3F) << 8);
    else
    {
        m_ppuVramAddrTemp = (m_ppuVramAddrTemp & 0x7F00) | m_ppuRegIoDb;
        m_ppuVramAddr = m_ppuVramAddrTemp;
        m_emu.memory().board()->onPpuAddressUpdate(m_ppuVramAddr);
    }

    m_ppuVramFlipFlop = !m_ppuVramFlipFlop;
}

void Ppu::onRegister2007()
{
    if(m_ppuRegAccessW)
    {
        m_ppuVramAddrAccessTemp = m_ppuVramAddr & 0x3FFF;

        // ON Writes
        if(m_ppuVramAddrAccessTemp < 0x2000)
            m_emu.memory().board()->writeChr(m_ppuVramAddrAccessTemp, m_ppuRegIoDb);
        else if(m_ppuVramAddrAccessTemp < 0x3F00)
            m_emu.memory().board()->writeNmt(m_ppuVramAddrAccessTemp, m_ppuRegIoDb);
        else
        {
            if(m_ppuVramAddrAccessTemp & 3)
                m_ppuPaletteBank[m_ppuVramAddrAccessTemp & 0x1F] = m_ppuRegIoDb;
            else
                m_ppuPaletteBank[m_ppuVramAddrAccessTemp & 0x0C] = m_ppuRegIoDb;
        }
    }
    else
    {
        // ON Reads
        if((m_ppuVramAddr & 0x3F00) == 0x3F00)
            m_ppuVramAddrAccessTemp = m_ppuVramAddr & 0x2FFF;
        else
            m_ppuVramAddrAccessTemp = m_ppuVramAddr & 0x3FFF;

        // Update the vram data bus
        if(m_ppuVramAddrAccessTemp < 0x2000)
            m_ppuVramData = m_emu.memory().board()->readChr(m_ppuVramAddrAccessTemp);
        else if(m_ppuVramAddrAccessTemp < 0x3F00)
            m_ppuVramData = m_emu.memory().board()->readNmt(m_ppuVramAddrAccessTemp);
    }

    m_ppuVramAddr = (m_ppuVramAddr + m_ppuReg2000VramAddressIncreament) & 0x7FFF;
    m_emu.memory().board()->onPpuAddressUpdate(m_ppuVramAddr);
}

void Ppu::read2000()
{
}

void Ppu::read2001()
{
}

void Ppu::read2002()
{
    m_ppuRegIoDb = (m_ppuRegIoDb & 0xDF) | (m_ppuReg2002SpriteOverflow ? 0x20 : 0x0);
    m_ppuRegIoDb = (m_ppuRegIoDb & 0xBF) | (m_ppuReg2002Sprite0Hit ? 0x40 : 0x0);
    m_ppuRegIoDb = (m_ppuRegIoDb & 0x7F) | (m_ppuReg2002VblankStartedFlag ? 0x80 : 0x0);
}

void Ppu::read2003()
{
}

void Ppu::read2004()
{
    m_ppuRegIoDb = m_ppuOamBank[m_ppuReg2003OamAddr];
}

void Ppu::read2005()
{
}

void Ppu::read2006()
{
}

void Ppu::read2007()
{
    m_ppuVramAddrAccessTemp = m_ppuVramAddr & 0x3FFF;
    if(m_ppuVramAddrAccessTemp < 0x3F00)
        // Reading will put the vram data bus into the io bus,
        // then later it will transfer the data from vram datas bus into io data bus.
        // This causes the 0x2007 dummy reads effect.
        m_ppuRegIoDb = m_ppuVramData;
    else
    {
        // Reading from palettes puts the value in the io bus immediately
        if(m_ppuVramAddrAccessTemp & 3)
            m_ppuRegIoDb = m_ppuPaletteBank[m_ppuVramAddrAccessTemp & 0x1F];
        else
            m_ppuRegIoDb = m_ppuPaletteBank[m_ppuVramAddrAccessTemp & 0x0C];
    }
}

bool Ppu::isRenderingOn() const
{
    return m_ppuReg2001ShowBackground || m_ppuReg2001ShowSprites;
}

bool Ppu::isInRender() const
{
    return m_ppuClockV < SCREEN_HEIGHT || m_ppuClockV == EmuSettings::ppuClockVBlankEnd;
}

void Ppu::readState(QDataStream &dataStream)
{
    dataStream >> m_ppuClockH >> m_ppuClockV >> m_ppuUseOddSwap >> m_ppuIsNmiTime >> m_ppuOamBank >> m_ppuOamBankSecondary >> m_ppuPaletteBank >> m_ppuRegIoDb
               >> m_ppuRegIoAddr >> m_ppuRegAccessHappened >> m_ppuRegAccessW >> m_ppuReg2000VramAddressIncreament >> m_ppuReg2000SpritePatternTableAddressFor8x8Sprites
               >> m_ppuReg2000BackgroundPatternTableAddress >> m_ppuReg2000SpriteSize >> m_ppuReg2000Vbi >> m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen
               >> m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen >> m_ppuReg2001ShowBackground >> m_ppuReg2001ShowSprites >> m_ppuReg2001Grayscale >> m_ppuReg2001Emphasis
               >> m_ppuReg2002SpriteOverflow >> m_ppuReg2002Sprite0Hit >> m_ppuReg2002VblankStartedFlag >> m_ppuReg2003OamAddr >> m_ppuVramAddr >> m_ppuVramData >> m_ppuVramAddrTemp
               >> m_ppuVramAddrAccessTemp >> m_ppuVramFlipFlop >> m_ppuVramFinex >> m_ppuBkgfetchNtAddr >> m_ppuBkgfetchNtData >> m_ppuBkgfetchAtAddr >> m_ppuBkgfetchAtData
               >> m_ppuBkgfetchLbAddr >> m_ppuBkgfetchLbData >> m_ppuBkgfetchHbAddr >> m_ppuBkgfetchHbData >> m_ppuSprfetchSlot >> m_ppuSprfetchYData >> m_ppuSprfetchTData
               >> m_ppuSprfetchAtData >> m_ppuSprfetchXData >> m_ppuSprfetchLbAddr >> m_ppuSprfetchLbData >> m_ppuSprfetchHbAddr >> m_ppuSprfetchHbData >> m_ppuColorAnd
               >> m_ppuOamEvN >> m_ppuOamEvM >> m_ppuOamevCompare >> m_ppuOamevSlot >> m_ppuFetchData >> m_ppuPhaseIndex >> m_ppuSprite0ShouldHit;
}

void Ppu::writeState(QDataStream &dataStream) const
{
    dataStream << m_ppuClockH << m_ppuClockV << m_ppuUseOddSwap << m_ppuIsNmiTime << m_ppuOamBank << m_ppuOamBankSecondary << m_ppuPaletteBank << m_ppuRegIoDb
               << m_ppuRegIoAddr << m_ppuRegAccessHappened << m_ppuRegAccessW << m_ppuReg2000VramAddressIncreament << m_ppuReg2000SpritePatternTableAddressFor8x8Sprites
               << m_ppuReg2000BackgroundPatternTableAddress << m_ppuReg2000SpriteSize << m_ppuReg2000Vbi << m_ppuReg2001ShowBackgroundInLeftmost8PixelsOfScreen
               << m_ppuReg2001ShowSpritesInLeftmost8PixelsOfScreen << m_ppuReg2001ShowBackground << m_ppuReg2001ShowSprites << m_ppuReg2001Grayscale << m_ppuReg2001Emphasis
               << m_ppuReg2002SpriteOverflow << m_ppuReg2002Sprite0Hit << m_ppuReg2002VblankStartedFlag << m_ppuReg2003OamAddr << m_ppuVramAddr << m_ppuVramData << m_ppuVramAddrTemp
               << m_ppuVramAddrAccessTemp << m_ppuVramFlipFlop << m_ppuVramFinex << m_ppuBkgfetchNtAddr << m_ppuBkgfetchNtData << m_ppuBkgfetchAtAddr << m_ppuBkgfetchAtData
               << m_ppuBkgfetchLbAddr << m_ppuBkgfetchLbData << m_ppuBkgfetchHbAddr << m_ppuBkgfetchHbData << m_ppuSprfetchSlot << m_ppuSprfetchYData << m_ppuSprfetchTData
               << m_ppuSprfetchAtData << m_ppuSprfetchXData << m_ppuSprfetchLbAddr << m_ppuSprfetchLbData << m_ppuSprfetchHbAddr << m_ppuSprfetchHbData << m_ppuColorAnd
               << m_ppuOamEvN << m_ppuOamEvM << m_ppuOamevCompare << m_ppuOamevSlot << m_ppuFetchData << m_ppuPhaseIndex << m_ppuSprite0ShouldHit;
}

const std::array<qint32, Ppu::SCREEN_WIDTH*Ppu::SCREEN_HEIGHT> &Ppu::screenPixels() const
{
    return m_ppuScreenPixels;
}
