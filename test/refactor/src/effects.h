#ifndef SHOGUN_EFFECTS_H
#define SHOGUN_EFFECTS_H

#include "art.h"
#include "game.h"

/* Small shogun badge shown while a check warning is active. */
void effects_draw_check(Art *art, const Game *game, int portrait);

/* Large animated shogun shown on the winner screen. */
void effects_draw_win(Art *art, const Game *game, int portrait);

#endif
