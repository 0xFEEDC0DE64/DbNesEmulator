#pragma once

#include <QtGlobal>

#include <array>
#include <cmath>

#include "enums/emuregion.h"

namespace EmuSettings
{
    constexpr EmuRegion region = EmuRegion::PALB;

    constexpr double emuTimeTargetFps = region == EmuRegion::NTSC ? 60.0988 : 50.;
    constexpr double emuTimeFramePeriod = 1000. / emuTimeTargetFps;

    constexpr quint16 ppuClockVBlankStart = region == EmuRegion::DENDY ? 291 : 241;
    constexpr quint16 ppuClockVBlankEnd = region == EmuRegion::NTSC ? 261 : 311;
    constexpr bool ppuUseOddCycle = region == EmuRegion::NTSC;

    constexpr bool frameLimiterEnabled = false;

    namespace Video
    {
        constexpr float saturation = 2.0f;
        constexpr float hue_tweak = 0.0f;
        constexpr float contrast = 1.4f;
        constexpr float brightness = 1.070f;
        constexpr float gamma = 2.0f;

        // Voltage levels, relative to synch voltage
        constexpr float black = 0.518f;
        constexpr float white = 1.962f;
        constexpr float attenuation = 0.746f;

        constexpr std::array<float, 8> levels
        {
            0.350f, 0.518f, 0.962f, 1.550f, // Signal low
            1.094f, 1.506f, 1.962f, 1.962f  // Signal high
        };

        constexpr qint32 makeRGBcolor(qint32 pixel)
        {
            // The input value is a NES color index (with de-emphasis bits).
            // We need RGB values. Convert the index into RGB.
            // For most part, this process is described at:
            //    http://wiki.nesdev.com/w/index.php/NTSC_video

            // Decode the color index
            const qint32 color = (pixel & 0x0F);
            const qint32 level = color < 0xE ? (pixel >> 4) & 3 : 1;

            const float lo = levels[level + ((color == 0x0) ? 4 : 0)];
            const float hi = levels[level + ((color <= 0xC) ? 4 : 0)];

            // Calculate the luma and chroma by emulating the relevant circuits:
            float y = 0.0f;
            float i = 0.0f;
            float q = 0.0f;

            for (int p = 0; p < 12; p++) // 12 clock cycles per pixel.
            {
                const auto wave = [](const int p, const int color) {
                    return (color + p + 8) % 12 < 6;
                };

                // NES NTSC modulator (square wave between two voltage levels):
                auto spot = wave(p, color) ? hi : lo;

                // De-emphasis bits attenuate a part of the signal:
                if (((pixel & 0x040) && wave(p, 0xC)) ||
                    ((pixel & 0x080) && wave(p, 0x4)) ||
                    ((pixel & 0x100) && wave(p, 0x8)))
                    spot *= attenuation;

                // Normalize:
                float v = (spot - black) / (white - black);

                // Ideal TV NTSC demodulator:
                // Apply contrast/brightness
                v = (v - 0.5F) * contrast + 0.5F;
                v *= brightness / 12.0F;

                y += v;
//                    if constexpr (EmuSettings::region == EmuRegion::NTSC)
//                    {
                    i += v * std::cos((M_PI / 6.0) * (p + hue_tweak));
                    q += v * std::sin((M_PI / 6.0) * (p + hue_tweak));
//                    }
//                    else
//                    {
//                        // PAL hue is rotated by 15° from NTSC
//                        i += v * std::cos((M_PI / 6.0) * (p + 0.5F + hue_tweak));
//                        q += v * std::sin((M_PI / 6.0) * (p + 0.5F + hue_tweak));
//                    }
            }

            i *= saturation;
            q *= saturation;

            const auto gammafix = [](const float f, const float gamma){
                return f < 0.0f ? 0.0f : std::pow(f, 2.2f / gamma);
            };

            const auto clamp = [](const float v)->int {
                return v < 0 ? 0 : v > 255 ? 255 : v;
            };

            // Convert YIQ into RGB according to FCC-sanctioned conversion matrix.
            return
                0x10000 * clamp(255 * gammafix(y + 0.946882F * i + 0.623557F * q, gamma)) +
                0x00100 * clamp(255 * gammafix(y - 0.274788F * i - 0.635691F * q, gamma)) +
                0x00001 * clamp(255 * gammafix(y - 1.108545F * i + 1.709007F * q, gamma));
        }

        constexpr std::array<qint32, 512> palette = [](){
            std::array<qint32, 512> pal {};

            for (int i = 0; i < 512; i++)
                pal[i] = makeRGBcolor(i) | (0xFF << 24);

            return pal;
        }();
    }

    namespace Audio
    {
        constexpr int volume = 100;
        constexpr int internalAmplitude = 285;
        constexpr int internalPeekLimit = 124;

        namespace ChannelEnabled
        {
            constexpr bool SQ1 = true;
            constexpr bool SQ2 = true;
            constexpr bool NOZ = true;
            constexpr bool TRL = true;
            constexpr bool DMC = true;
            constexpr bool MMC5_SQ1 = true;
            constexpr bool MMC5_SQ2 = true;
            constexpr bool MMC5_PCM = true;
            constexpr bool VRC6_SQ1 = true;
            constexpr bool VRC6_SQ2 = true;
            constexpr bool VRC6_SAW = true;
            constexpr bool SUN1 = true;
            constexpr bool SUN2 = true;
            constexpr bool SUN3 = true;
            constexpr bool NMT1 = true;
            constexpr bool NMT2 = true;
            constexpr bool NMT3 = true;
            constexpr bool NMT4 = true;
            constexpr bool NMT5 = true;
            constexpr bool NMT6 = true;
            constexpr bool NMT7 = true;
            constexpr bool NMT8 = true;
        }
    }
}
