#include "effects.h"

#include "layout.h"

static const Color WHITE = {255, 255, 255};
static const Color GOLD = {255, 255, 0};
static const Color GUIDE = {255, 160, 0};

#define SPIN_TICKS 24

static int shogun_image(Side side)
{
    return side == SIDE_WHITE ? 3 : 2;
}

static float ease_out(float p)
{
    float q = 1.0f - p;
    return 1.0f - q * q * q;
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

static void winner_target(int portrait, float *cx, float *cy)
{
    /* These are the original no-sound winner-screen shogun spots. */
    if (portrait) {
        *cx = 240.0f;
        *cy = 700.0f;
    } else {
        *cx = 300.0f;
        *cy = 480.0f;
    }
}

static void draw_big_shogun(Art *art, const Game *game, int portrait)
{
    int t = game->win_timer;
    int id = shogun_image((Side)game->winner);
    float cx;
    float cy;
    float x;
    float y;
    float scale;
    float rot;
    float alpha;
    float base = portrait ? 1.5f : 1.6f;

    winner_target(portrait, &cx, &cy);

    if (t < SPIN_TICKS) {
        float p = ease_out((float)t / (float)SPIN_TICKS);
        float fly = 1.0f - p;
        x = cx + fly * (portrait ? 180.0f : 300.0f);
        y = cy - fly * (portrait ? 220.0f : 180.0f);
        scale = base * (1.0f + 0.5f * fly);
        rot = fly * 720.0f;
        if (game->winner == SIDE_RED) rot = -rot;
        alpha = 0.55f + 0.45f * p;

        /* Two faint copies make the spin-in read as motion, not a teleport. */
        for (int i = 1; i <= 2; ++i) {
            float trail_p = ease_out((float)(t > i ? t - i : 0) /
                                     (float)SPIN_TICKS);
            float trail_fly = 1.0f - trail_p;
            art->alpha = i == 1 ? 0.18f : 0.10f;
            art->color = WHITE;
            art_image_rot(art, id, 1,
                          cx + trail_fly * (portrait ? 180.0f : 300.0f),
                          cy - trail_fly * (portrait ? 220.0f : 180.0f),
                          trail_fly * (game->winner == SIDE_RED ? -720.0f : 720.0f),
                          base * (1.0f + 0.5f * trail_fly),
                          base * (1.0f + 0.5f * trail_fly));
        }
    } else {
        int q = t - SPIN_TICKS;
        float pulse = (float)(q % 16) / 16.0f;
        x = cx;
        y = cy + (float)((q % 8) - 4) * 1.5f;
        scale = base * (1.0f + 0.08f * pulse);
        rot = (float)((q % 8) - 4) * 2.0f;
        alpha = 0.92f;
    }

    art->alpha = alpha;
    art->color = WHITE;
    art_image_rot(art, id, 1, x, y, rot, scale, scale);
    art->alpha = 1.0f;
}

static void draw_target_brackets(Art *art, int portrait, int t)
{
    float cx;
    float cy;
    float size;
    float half;
    float arm;
    float alpha;

    winner_target(portrait, &cx, &cy);

    if (t < SPIN_TICKS) {
        float p = ease_out((float)t / (float)SPIN_TICKS);
        size = 110.0f - 54.0f * p;
        alpha = 0.35f + 0.20f * p;
    } else {
        size = 56.0f + ((t & 8) ? 2.0f : -2.0f);
        alpha = 0.55f;
    }
    half = size / 2.0f;
    arm = 12.0f;

    art->alpha = alpha;
    art->color = GUIDE;
    art_rect(art, cx - half, cy - half, arm, 2.0f);
    art_rect(art, cx - half, cy - half, 2.0f, arm);
    art_rect(art, cx + half - arm, cy - half, arm, 2.0f);
    art_rect(art, cx + half - 2.0f, cy - half, 2.0f, arm);
    art_rect(art, cx - half, cy + half - 2.0f, arm, 2.0f);
    art_rect(art, cx - half, cy + half - arm, 2.0f, arm);
    art_rect(art, cx + half - arm, cy + half - 2.0f, arm, 2.0f);
    art_rect(art, cx + half - 2.0f, cy + half - arm, 2.0f, arm);
    art->alpha = 1.0f;
    art->color = WHITE;
}

static void draw_ring(Art *art, float cx, float cy, float hw, float hh)
{
    art_rect(art, cx - hw, cy - hh, hw * 2.0f, 2.0f);
    art_rect(art, cx - hw, cy + hh - 2.0f, hw * 2.0f, 2.0f);
    art_rect(art, cx - hw, cy - hh, 2.0f, hh * 2.0f);
    art_rect(art, cx + hw - 2.0f, cy - hh, 2.0f, hh * 2.0f);
}

static void draw_logo_impact(Art *art, const Game *game, int portrait)
{
    int t = game->win_timer;
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    float cx = portrait ? 240.0f : 735.0f;
    float cy = portrait ? 96.0f : 230.0f;
    float scale;
    float y;
    float rot;

    if (t < 8) {
        art->alpha = 0.22f * (1.0f - (float)t / 8.0f);
        art->color = WHITE;
        art_rect(art, 0.0f, 0.0f, (float)w, (float)h);
    }

    if (t < 20) {
        float fade = 1.0f - (float)t / 20.0f;
        float hw = 70.0f + (float)t * 9.0f;
        float hh = 20.0f + (float)t * 4.0f;
        art->alpha = 0.60f * fade;
        art->color = GOLD;
        draw_ring(art, cx, cy, hw, hh);
        art->alpha = 0.35f * fade;
        art->color = WHITE;
        draw_ring(art, cx, cy, hw - 8.0f, hh - 5.0f);
    }

    if (t < 10) {
        float p = ease_out((float)t / 10.0f);
        scale = 0.18f + 0.55f * p;
        y = cy - 70.0f * (1.0f - p);
        rot = (1.0f - p) * 180.0f;
    } else if (t < 18) {
        float p = (float)(t - 10) / 8.0f;
        scale = 0.73f - 0.08f * p;
        y = cy + 5.0f * p;
        rot = 0.0f;
    } else {
        scale = 0.68f + ((t & 8) ? 0.01f : 0.0f);
        y = cy + ((t & 8) ? -2.0f : 2.0f);
        rot = 0.0f;
    }

    art->alpha = 1.0f;
    art->color = WHITE;
    art_image_rot(art, 0, 0, cx, y, rot, scale, scale);
    art->alpha = 1.0f;
}

void effects_draw_win(Art *art, const Game *game, int portrait)
{
    if (game->winner < 0) return;
    draw_logo_impact(art, game, portrait);
    draw_target_brackets(art, portrait, game->win_timer);
    draw_big_shogun(art, game, portrait);
}
