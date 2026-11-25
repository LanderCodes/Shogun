#include "effects.h"

#include "layout.h"

static const Color WHITE = {255, 255, 255};

static int shogun_image(Side side)
{
    return side == SIDE_WHITE ? 3 : 2;
}

void effects_draw_check(Art *art, const Game *game, int portrait)
{
    if (game->check_timer <= 0) return;
    if (game->check_cell.row < 0 || game->check_cell.row >= BOARD_SIZE ||
        game->check_cell.col < 0 || game->check_cell.col >= BOARD_SIZE) {
        return;
    }

    float x;
    float y;
    layout_cell_center(portrait, game->check_cell.row,
                       game->check_cell.col, &x, &y);
    float bob = (game->tick & 4) ? -2.0f : 2.0f;

    art->alpha = 0.95f;
    art->color = WHITE;
    art_image(art, shogun_image(game->turn), 1,
              x + 18.0f, y - 18.0f + bob, 0.5f, 0.5f);
    art->alpha = 1.0f;
}
