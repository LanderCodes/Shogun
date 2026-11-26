#include "render/glyph.h"

/* The zxfont sheet is a 32x3 grid of 16x16 frames. This is the character to
   frame order visible in the sheet: space, punctuation, digits, punctuation,
   @, A-Z, brackets, lowercase a-z and braces. The original WASM table was
   stored glyph-to-character and was copied here in the wrong direction, so
   most punctuation and all letters used to draw unrelated glyphs. */
static const unsigned char GLYPHS[96] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     7, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 31,
};

int glyph_frame(unsigned char c)
{
    if (c < 32 || c >= 127) return 31; /* '?' for unsupported input */
    return GLYPHS[c - 32];
}
