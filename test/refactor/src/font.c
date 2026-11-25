#include "font.h"

#include <string.h>

/* ASCII 32..127 mapped to zxfont sheet frames. */
static const uint8_t GLYPHS[96] = {
     32,  33, 126,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    163,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 169,
};

static int glyph_frame(char c)
{
    unsigned char ch = (unsigned char)c;
    if (ch < 32 || ch > 127) return 0;
    return GLYPHS[ch - 32];
}

void font_text(Art *art, const char *text, float x, float y, float scale, int centered)
{
    if (!text) return;
    int length = (int)strlen(text);
    float step = 16.0f * scale;
    float start = centered ? x - (float)(length - 1) * step / 2.0f : x;

    for (int i = 0; i < length; ++i) {
        if (text[i] != ' ') {
            art_image(art, 7, glyph_frame(text[i]), start + (float)i * step, y,
                      scale, scale);
        }
    }
}
