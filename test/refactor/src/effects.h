#ifndef SHOGUN_EFFECTS_H
#define SHOGUN_EFFECTS_H

#include "art.h"
#include "game.h"

/* Small shogun badge shown while a check warning is active. */
void effects_draw_check(Art *art, const Game *game, int portrait);

/* Original-style winner screen: drifting banner is in render.c, while this
   draws the spin-in shogun, targeting brackets and small logo impact. */
void effects_draw_win(Art *art, const Game *game, int portrait);

#endif
