#include "effects.h"

#include "layout.h"

static const Color WHITE = {255, 255, 255};
static const Color GOLD = {255, 255, 0};

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

static void draw_big_shogun(Art *art, const Game *game, int portrait)
{
    float cx = portrait ? 240.0f : 360.0f;
    float cy = portrait ? 330.0f : 330.0f;
    float pulse = (float)(game->win_timer % 12) / 12.0f;
    float scale = (portrait ? 2.2f : 2.0f) + pulse * 0.35f;
    float rot = (float)((game->win_timer % 24) - 12) * 1.5f;
    float bob = (float)((game->win_timer % 8) - 4) * 1.5f;

    art->alpha = 0.90f;
    art->color = WHITE;
    art_image_rot(art, shogun_image((Side)game->winner), 1,
                  cx, cy + bob, rot, scale, scale);
    art->alpha = 1.0f;
}

static void draw_ring(Art *art, float cx, float cy, float hw, float hh)
{
    art_rect(art, cx - hw, cy - hh, hw * 2.0f, 4.0f);
    art_rect(art, cx - hw, cy + hh - 4.0f, hw * 2.0f, 4.0f);
    art_rect(art, cx - hw, cy - hh, 4.0f, hh * 2.0f);
    art_rect(art, cx + hw - 4.0f, cy - hh, 4.0f, hh * 2.0f);
}

/* Logo slam and expanding rings. Deliberately no words are drawn here. */
static void draw_logo_impact(Art *art, const Game *game, int portrait)
{
    int t = game->win_timer;
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    float cx = portrait ? 240.0f : 360.0f;
    float cy = portrait ? 240.0f : 210.0f;

    if (t < 8) {
        art->alpha = 0.75f * (1.0f - (float)t / 8.0f);
        art->color = WHITE;
        art_rect(art, 0.0f, 0.0f, (float)w, (float)h);
    }

    if (t < 20) {
        float fade = 1.0f - (float)t / 20.0f;
        float hw = 150.0f + (float)t * 22.0f;
        float hh = 45.0f + (float)t * 14.0f;
        art->alpha = 0.70f * fade;
        art->color = GOLD;
        draw_ring(art, cx, cy, hw, hh);
        art->alpha = 0.45f * fade;
        art->color = WHITE;
        draw_ring(art, cx, cy, hw - 12.0f, hh - 8.0f);
    }

    float scale;
    float y;
    float rot;
    if (t < 12) {
        float p = (float)t / 12.0f;
        scale = 0.3f + 2.0f * p;
        y = cy - 120.0f + 10.0f * (float)t;
        rot = (float)(12 - t) * 2.5f;
    } else if (t < 20) {
        float p = (float)(t - 12) / 8.0f;
        scale = 2.3f - 0.7f * p;
        y = cy + 2.0f * (float)(20 - t);
        rot = 0.0f;
    } else {
        scale = 1.6f + ((t & 8) ? 0.02f : 0.0f);
        y = cy + ((t & 8) ? -3.0f : 3.0f);
        rot = 0.0f;
    }

    art->alpha = 1.0f;
    art->color = WHITE;
    art_image_rot(art, 0, 0, cx, y, rot, scale, scale);
}

void effects_draw_win(Art *art, const Game *game, int portrait)
{
    if (game->winner < 0) return;
    draw_big_shogun(art, game, portrait);
    draw_logo_impact(art, game, portrait);
}
