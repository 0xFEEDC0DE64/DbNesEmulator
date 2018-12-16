#pragma once

enum class Mirroring
{
    // Mirroring value:
    // 0000 0000
    // ddcc bbaa
    // aa: index for area 0x2000
    // bb: index for area 0x2400
    // cc: index for area 0x2800
    // dd: index for area 0x2C00
    /*
           (  D  C  B  A)
Horz:  $50  (%01 01 00 00)
Vert:  $44  (%01 00 01 00)
1ScA:  $00  (%00 00 00 00)
1ScB:  $55  (%01 01 01 01)
Full:  $E4  (%11 10 01 00)
    */
    Horizontal = 0x50,
    Vertical = 0x44,
    OneScA = 0x00,
    OneScB = 0x55,
    Full = 0xE4
};
