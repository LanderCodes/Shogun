#include "render/font.h"

#include "render/glyph.h"

#include <string.h>

void font_text(Art *art, const char *text, float x, float y, float scale, int centered)
{
    if (!text) return;
    int length = (int)strlen(text);
    float step = 16.0f * scale;
    float start = centered ? x - (float)(length - 1) * step / 2.0f : x;

    for (int i = 0; i < length; ++i) {
        if (text[i] != ' ') {
            art_image(art, 7, glyph_frame((unsigned char)text[i]),
                      start + (float)i * step, y, scale, scale);
        }
    }
}
