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

void effects_draw_win(Art *art, const Game *game, int portrait)
{
    if (game->winner < 0) return;

    float cx = portrait ? 240.0f : 360.0f;
    float cy = portrait ? 300.0f : 270.0f;
    float pulse = (float)(game->tick % 12) / 12.0f;
    float scale = (portrait ? 2.2f : 2.0f) + pulse * 0.35f;
    float rot = (float)((game->tick % 24) - 12) * 1.5f;
    float bob = (float)((game->tick % 8) - 4) * 1.5f;

    art->alpha = 0.90f;
    art->color = WHITE;
    art_image_rot(art, shogun_image((Side)game->winner), 1,
                  cx, cy + bob, rot, scale, scale);
    art->alpha = 1.0f;
}
