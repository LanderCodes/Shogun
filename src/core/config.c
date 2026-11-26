#include "core/config.h"

void settings_init(Settings *settings)
{
    /* Settings are deliberately session-only: closing the game forgets them. */
    settings->show_moves = 0;
    settings->show_terrain = 0;
    settings->improved_ai = 0;
}
