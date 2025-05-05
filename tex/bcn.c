#include "bcn.h"

#include <string.h>

static void _RGB565_RGB888(u16 rgb565, u8 rgb888[3]) {
    rgb888[0] = ((rgb565 >> 11) & 0x1F) * 255 / 31;
    rgb888[1] = ((rgb565 >> 5) & 0x3F) * 255 / 63;
    rgb888[2] = (rgb565 & 0x1F) * 255 / 31;
}

static void _InterpolateColors(u16 c0, u16 c1, u8 colors[4][3]) {
    _RGB565_RGB888(c0, colors[0]);
    _RGB565_RGB888(c1, colors[1]);

    if (c0 > c1) {
        for (unsigned i = 0; i < 3; i++) {
            colors[2][i] = (2 * colors[0][i] + colors[1][i]) / 3;
            colors[3][i] = (colors[0][i] + 2 * colors[1][i]) / 3;
        }
    }
    else {
        for (unsigned i = 0; i < 3; i++) {
            colors[2][i] = (colors[0][i] + colors[1][i]) / 2;
            colors[3][i] = 0;
        }
    }
}

static void _BuildAlphaTable(u8 a0, u8 a1, u8 table[8]) {
    table[0] = a0;
    table[1] = a1;
    if (a0 > a1) {
        for (unsigned i = 1; i <= 6; i++)
            table[i + 1] = ((7 - i) * a0 + i * a1) / 7;
    }
    else {
        for (unsigned i = 1; i <= 4; i++)
            table[i + 1] = ((5 - i) * a0 + i * a1) / 5;
        table[6] = 0;
        table[7] = 255;
    }
}

void BCNDecode_BC3(const u8 block[16], u8 rgba[4][4][4]) {
    // Alpha
    const u8 a0 = block[0];
    const u8 a1 = block[1];

    u64 alphaBits = 0;
    for (unsigned i = 0; i < 6; i++)
        alphaBits |= ((u64)block[2 + i]) << (8 * i);

    u8 alphaTable[8];
    _BuildAlphaTable(a0, a1, alphaTable);

    // Colors
    const u16 c0 = block[8] | (block[9] << 8);
    const u16 c1 = block[10] | (block[11] << 8);

    u8 colorTable[4][3];
    _InterpolateColors(c0, c1, colorTable);

    const u32 colorIndices = block[12] | (block[13] << 8) | (block[14] << 16) | (block[15] << 24);

    for (unsigned i = 0; i < 16; i++) {
        const unsigned x = i % 4;
        const unsigned y = i / 4;

        const u64 alphaIndex = (alphaBits >> (i * 3)) & 0x07;
        const u32 colorIndex = (colorIndices >> (i * 2)) & 0x03;

        rgba[y][x][0] = colorTable[colorIndex][0];
        rgba[y][x][1] = colorTable[colorIndex][1];
        rgba[y][x][2] = colorTable[colorIndex][2];
        rgba[y][x][3] = alphaTable[alphaIndex];
    }
}
