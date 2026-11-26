#include "render/render.h"

#include "core/board.h"
#include "core/rules.h"
#include "render/effects.h"
#include "render/font.h"
#include "render/layout.h"
#include "ui/menu.h"

#include <stdio.h>
#include <string.h>

/* Colours copied from the original game's P_color calls. */
static const Color WHITE = {255, 255, 255};
static const Color BLACK = {0, 0, 0};
static const Color BACKDROP = {16, 16, 16};
static const Color BLUE = {0, 0, 64};
static const Color TILE_LIGHT = {72, 72, 72};
static const Color TILE_DARK = {24, 24, 24};
static const Color BOARD_FRAME = {90, 90, 120};
static const Color GOLD = {255, 255, 0};
static const Color SETTING_BOX_ON = {0, 95, 0};
static const Color SETTING_BOX_ON_LIGHT = {0, 155, 0};
static const Color SETTING_BOX_OFF = {115, 0, 0};
static const Color SETTING_BOX_OFF_LIGHT = {185, 0, 0};
static const Color HOVER = {80, 255, 80};
static const Color OLIVE = {128, 128, 0};
static const Color OLIVE_LIGHT = {159, 159, 63};
static const Color TEAL = {0, 128, 128};
static const Color MAROON = {128, 0, 0};
static const Color MAROON_LIGHT = {159, 63, 63};
static const Color PURPLE = {80, 0, 128};
static const Color RED_TEXT = {255, 90, 90};
static const Color PALE_TEXT = {230, 230, 230};
static const Color GRAY_TEXT = {160, 160, 160};
static const Color FOOTER = {200, 200, 255};
static const Color GREEN_BUTTON = {63, 138, 63};
static const Color DARK_GREEN = {0, 70, 0};
static const Color HINT = {170, 170, 170};
static const Color GUIDE = {255, 160, 0};
static const Color CHECK = {255, 0, 0};
static const Color CAPTURED = {128, 128, 128};

static void label(Art *art, const char *text, float x, float y,
                  float scale, Color color, int centered)
{
    art->color = color;
    font_text(art, text, x, y, scale, centered);
    art->color = WHITE;
}

/* Only the title and menu screens use the original checkerboard of 60px
   tiles. The sprite art is dark, so a neutral square keeps it visible. */
static void draw_tiles(Art *art, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    for (int row = 0; row * 60 < h; ++row) {
        for (int col = 0; col * 60 < w; ++col) {
            int dark = (row + col) & 1;
            art->alpha = 1.0f;
            art->color = dark ? TILE_DARK : TILE_LIGHT;
            art_rect(art, (float)col * 60.0f, (float)row * 60.0f,
                     60.0f, 60.0f);
            art->alpha = 0.35f;
            art->color = WHITE;
            art_image(art, 1, dark, (float)col * 60.0f + 30.0f,
                      (float)row * 60.0f + 30.0f, 1.0f, 1.0f);
        }
    }
    art->alpha = 1.0f;
    art->color = WHITE;
}

static void draw_logo(Art *art, int portrait)
{
    float cx = portrait ? 240.0f : 480.0f;
    float cy = portrait ? 80.0f : 82.0f;
    art->alpha = 0.75f;
    art->color = BLACK;
    art_image(art, 0, 0, cx + 10.0f, cy + 10.0f, 1.5f, 1.5f);
    art->alpha = 1.0f;
    art->color = WHITE;
    art_image(art, 0, 0, cx, cy, 1.5f, 1.5f);
}

static Color box_color(int portrait, int index)
{
    if (portrait) {
        switch (index) {
        case 0: case 1: return OLIVE;
        case 2: return TEAL;
        case 3: case 4: return MAROON;
        default: return PURPLE;
        }
    }
    switch (index) {
    case 0: case 1: return OLIVE;
    case 2: return TEAL;
    case 3: case 4: return MAROON;
    default: return PURPLE;
    }
}

static Color box_highlight(Color base)
{
    if (base.r == 128 && base.g == 0) return MAROON_LIGHT;
    if (base.r == 128) return OLIVE_LIGHT;
    if (base.g == 128 && base.b == 128) return (Color){63, 159, 159};
    if (base.g == 128) return (Color){63, 159, 63};
    return (Color){159, 63, 191};
}

static void menu_box_colors(int portrait, int index, const Settings *settings,
                            Color *base, Color *highlight)
{
    if (index == 3 || index == 4) {
        int on = index == 3 ? settings->show_moves : settings->show_terrain;
        if (on) {
            *base = SETTING_BOX_ON;
            *highlight = SETTING_BOX_ON_LIGHT;
        } else {
            *base = SETTING_BOX_OFF;
            *highlight = SETTING_BOX_OFF_LIGHT;
        }
        return;
    }
    *base = box_color(portrait, index);
    *highlight = box_highlight(*base);
}

/* The setting boxes carry the red/green state, so labels stay high-contrast. */
static Color menu_text_color(int index, const Settings *settings)
{
    (void)index;
    (void)settings;
    return WHITE;
}

static void draw_menu(Art *art, const Game *game, int portrait)
{
    art_clear(art, BACKDROP);
    draw_tiles(art, portrait);
    draw_logo(art, portrait);

    /* The original paints every box before it paints any text. */
    for (int i = 0; i < MENU_ITEMS; ++i) {
        Rect box;
        layout_menu_item(portrait, i, &box);
        Color base;
        Color highlight;
        int active = i == game->menu_cursor;
        menu_box_colors(portrait, i, &game->settings, &base, &highlight);

        art->alpha = active ? 0.70f : 1.0f;
        art->color = active ? GOLD : WHITE;
        art_rect(art, box.x, box.y, box.w, box.h);
        art->alpha = active ? 0.80f : 1.0f;
        art->color = active ? highlight : base;
        art_rect(art, box.x + 2.0f, box.y + 2.0f, box.w - 4.0f, box.h - 4.0f);
        art->alpha = 1.0f;
        art->color = WHITE;
    }

    for (int i = 0; i < MENU_ITEMS; ++i) {
        Rect box;
        layout_menu_item(portrait, i, &box);
        char status[32];
        menu_status(i, &game->settings, status, sizeof(status));
        float cx = box.x + box.w / 2.0f;
        Color text_color = menu_text_color(i, &game->settings);

        if (portrait) {
            if (i < 3) {
                static const float offsets[3] = {47.0f, 47.0f, 42.0f};
                label(art, menu_name_portrait(i), cx, box.y + offsets[i],
                      2.0f, text_color, 1);
            } else {
                char line[64];
                snprintf(line, sizeof(line), "%s %s",
                         menu_name_portrait(i), status);
                label(art, line, cx, box.y + 27.0f, 1.0f, text_color, 1);
            }
        } else if (i < 3) {
            label(art, menu_name(i), cx, box.y + 34.0f, 2.0f, text_color, 1);
        } else {
            label(art, menu_name(i), cx, box.y + 18.0f, 1.0f, text_color, 1);
            label(art, status, cx, box.y + 44.0f, 2.0f, text_color, 1);
        }
    }

    if (!portrait) {
        label(art, "MOUSE, KEYBOARD OR CONTROLLER", 480.0f, 440.0f,
              1.0f, FOOTER, 1);
    }
    label(art, "Made by LanderCodes", portrait ? 240.0f : 480.0f,
          portrait ? 740.0f : 500.0f, 1.0f, HINT, 1);
}

static void draw_mode_button(Art *art, const Game *game, int portrait,
                             int index, const char *text, Color fill)
{
    Rect outer;
    Rect inner;
    layout_mode_button(portrait, index, &outer, &inner);
    int active = game->mode_cursor == index || game->hover_item == index;
    art->color = active ? GOLD : WHITE;
    art_rect(art, outer.x, outer.y, outer.w, outer.h);
    art->color = fill;
    art_rect(art, inner.x, inner.y, inner.w, inner.h);
    label(art, text, inner.x + inner.w / 2.0f, inner.y + 16.0f,
          1.5f, WHITE, 1);
}

static void draw_play_mode(Art *art, const Game *game, int portrait)
{
    art_clear(art, BACKDROP);
    draw_tiles(art, portrait);
    draw_logo(art, portrait);
    label(art, "TWO PLAYERS", portrait ? 240.0f : 480.0f,
          portrait ? 150.0f : 150.0f, 2.0f, WHITE, 1);
    draw_mode_button(art, game, portrait, 0, "LOCAL GAME", GREEN_BUTTON);
    draw_mode_button(art, game, portrait, 1, "ONLINE GAME", TEAL);
    draw_mode_button(art, game, portrait, 2, "BACK", MAROON);
}

static void draw_online_setup(Art *art, const Game *game, int portrait,
                              uint32_t now)
{
    art_clear(art, BACKDROP);
    float cx = portrait ? 240.0f : 480.0f;
    label(art, "ONLINE SETUP", cx, portrait ? 160.0f : 70.0f,
          2.0f, WHITE, 1);
    label(art, game->online_message, cx, portrait ? 190.0f : 112.0f,
          1.0f, GOLD, 1);

    for (int i = 0; i < 2; ++i) {
        Rect box;
        layout_setup_field(portrait, i, &box);
        int active = game->setup_cursor == i;
        art->alpha = active ? 0.75f : 1.0f;
        art->color = active ? GOLD : WHITE;
        art_rect(art, box.x, box.y, box.w, box.h);
        art->alpha = 1.0f;
        art->color = BLACK;
        art_rect(art, box.x + 2.0f, box.y + 2.0f, box.w - 4.0f, box.h - 4.0f);

        char entry[128];
        if (i == 0) {
            size_t n = strlen(game->token);
            if (n > 16) n = 16;
            memset(entry, '*', n);
            entry[n] = '\0';
            if (!n) snprintf(entry, sizeof(entry), "NONE");
        } else {
            snprintf(entry, sizeof(entry), "%s",
                     game->server_entry[0] ? game->server_entry : "AUTO");
        }
        if (active && game->setup_typing && (now / 400) % 2) {
            size_t n = strlen(entry);
            if (n + 1 < sizeof(entry)) {
                entry[n] = '_';
                entry[n + 1] = '\0';
            }
        }
        label(art, i == 0 ? "ACCESS TOKEN" : "SERVER",
              box.x + 12.0f, box.y - 22.0f, 1.0f, FOOTER, 0);
        label(art, entry, box.x + 12.0f, box.y + 24.0f,
              1.5f, active ? GOLD : PALE_TEXT, 0);
    }

    for (int i = 0; i < 2; ++i) {
        Rect outer;
        Rect inner;
        layout_setup_button(portrait, i, &outer, &inner);
        int active = game->setup_cursor == i + 2 || game->hover_item == i + 2;
        art->color = active ? GOLD : WHITE;
        art_rect(art, outer.x, outer.y, outer.w, outer.h);
        art->color = i == 0 ? TEAL : MAROON;
        art_rect(art, inner.x, inner.y, inner.w, inner.h);
        label(art, i == 0 ? "NEXT" : "BACK", inner.x + inner.w / 2.0f,
              inner.y + 16.0f, 1.5f, WHITE, 1);
    }

    label(art, "SELECT A FIELD, TYPE, THEN PRESS NEXT", cx,
          portrait ? 680.0f : 498.0f, 1.0f, HINT, 1);
}

static void draw_online_name(Art *art, const Game *game, int portrait,
                              uint32_t now)

{
    art_clear(art, BACKDROP);
    float cx = portrait ? 240.0f : 480.0f;
    label(art, "ONLINE GAME", cx, portrait ? 200.0f : 80.0f,
          2.0f, WHITE, 1);
    label(art, "ENTER YOUR NAME", cx, portrait ? 250.0f : 150.0f,
          1.5f, FOOTER, 1);

    Rect box;
    layout_name_box(portrait, &box);
    art->color = WHITE;
    art_rect(art, box.x, box.y, box.w, box.h);
    art->color = BLACK;
    art_rect(art, box.x + 2.0f, box.y + 2.0f, box.w - 4.0f, box.h - 4.0f);
    char entry[ONLINE_NAME_SIZE + 2];
    snprintf(entry, sizeof(entry), "%s%s", game->name,
             (now / 400) % 2 ? "_" : " ");
    label(art, entry, cx, box.y + 28.0f, 2.0f, GOLD, 1);

    label(art, "TYPE A NAME - ENTER: CONNECT - ESC: BACK", cx,
          portrait ? 430.0f : 330.0f, 1.0f, HINT, 1);
    draw_mode_button(art, game, portrait, 2, "BACK", MAROON);
}

static void draw_online_wait(Art *art, const Game *game, int portrait)
{
    art_clear(art, BACKDROP);
    float cx = portrait ? 240.0f : 480.0f;
    label(art, "ONLINE GAME", cx, portrait ? 200.0f : 80.0f,
          2.0f, WHITE, 1);
    char player[ONLINE_NAME_SIZE + 16];
    snprintf(player, sizeof(player), "NAME: %s", game->name);
    label(art, player, cx, portrait ? 250.0f : 145.0f, 1.5f, FOOTER, 1);
    label(art, game->online_message, cx, portrait ? 320.0f : 215.0f,
          1.0f, GOLD, 1);

    char server[112];
    snprintf(server, sizeof(server), "SERVER: %s", game->server);
    label(art, server, cx, portrait ? 360.0f : 260.0f, 1.0f, HINT, 1);

    if (game->opponent[0]) {
        char opponent[ONLINE_OPPONENT_SIZE + 16];
        snprintf(opponent, sizeof(opponent), "OPPONENT: %s", game->opponent);
        label(art, opponent, cx, portrait ? 400.0f : 295.0f,
              1.0f, PALE_TEXT, 1);
    }

    if (game->online_phase != ONLINE_WAITING &&
        game->online_phase != ONLINE_CONNECTING) {
        draw_mode_button(art, game, portrait, 1, "RETRY", TEAL);
    }
    draw_mode_button(art, game, portrait, 2, "BACK", MAROON);
}

static void draw_online_match(Art *art, const Game *game, int portrait)
{
    art_clear(art, BACKDROP);
    float cx = portrait ? 240.0f : 735.0f;
    label(art, "MATCH FOUND", cx, portrait ? 180.0f : 120.0f,
          2.0f, WHITE, 1);
    label(art, "WANTS TO PLAY", cx, portrait ? 250.0f : 170.0f,
          1.0f, FOOTER, 1);
    label(art, game->opponent, cx, portrait ? 300.0f : 215.0f,
          2.0f, GOLD, 1);

    for (int i = 0; i < 2; ++i) {
        Rect outer;
        Rect inner;
        layout_winner_button(portrait, i, &outer, &inner);
        int active = game->challenge_cursor == i || game->hover_item == i;
        art->color = active ? GOLD : WHITE;
        art_rect(art, outer.x, outer.y, outer.w, outer.h);
        art->color = i == 0 ? GREEN_BUTTON : MAROON;
        art_rect(art, inner.x, inner.y, inner.w, inner.h);
        label(art, i == 0 ? "ACCEPT" : "DECLINE",
              inner.x + inner.w / 2.0f, inner.y + 14.0f,
              1.5f, WHITE, 1);
    }
}

static int is_move_target(const Game *game, int row, int col)
{
    if (!game->settings.show_moves || !game->has_selection) return 0;
    for (int i = 0; i < game->move_count; ++i) {
        const Move *move = &game->moves[i];
        if (move->to.row == row && move->to.col == col &&
            !(move->to.row == game->selected.row &&
              move->to.col == game->selected.col)) return 1;
    }
    return 0;
}

static void draw_guide(Art *art, int portrait, int row, int col)
{
    float x;
    float y;
    layout_cell_center(portrait, row, col, &x, &y);
    x -= 30.0f;
    y -= 30.0f;

    art->alpha = 0.55f;
    art->color = GUIDE;
    art_rect(art, x + 2.0f, y + 2.0f, 56.0f, 2.0f);
    art_rect(art, x + 2.0f, y + 56.0f, 56.0f, 2.0f);
    art_rect(art, x + 2.0f, y + 2.0f, 2.0f, 56.0f);
    art_rect(art, x + 56.0f, y + 2.0f, 2.0f, 56.0f);
    art->alpha = 1.0f;
    art->color = WHITE;
}

static void draw_board(Art *art, const Game *game, int portrait)
{
    art_clear(art, BLUE);

    if (!portrait) {
        art->color = BOARD_FRAME;
        art_rect(art, 27.0f, 27.0f, 486.0f, 486.0f);
    }

    /* The original walks columns on the outside and rows on the inside, and
       draws a cell's tile and contents together. */
    for (int col = 0; col < BOARD_SIZE; ++col) {
        for (int row = 0; row < BOARD_SIZE; ++row) {
            float x;
            float y;
            layout_cell_center(portrait, row, col, &x, &y);
            float rot = row < 4 ? 180.0f : 0.0f;

            art->color = WHITE;
            art_image(art, 1, (row + col) & 1, x, y, 1.0f, 1.0f);

            const Piece *piece = &game->board.piece[row][col];
            if (piece->type != PIECE_NONE) {
                int id = piece->side == SIDE_WHITE ? 3 : 2;
                int frame = piece->type == PIECE_SHOGUN ? 1 : 0;
                art_image(art, id, frame, x, y, 1.0f, 1.0f);
                int number = piece->value > 2 ? 1 : piece->value - 1;
                if (piece->type == PIECE_SHOGUN) {
                    art_image_rot(art, 4, number, x, y,
                                  rot, 1.0f, 1.0f);
                } else {
                    art_image_rot(art, 4, piece->value - 1, x, y,
                                  rot, 1.0f, 1.0f);
                }
            } else if (game->settings.show_terrain) {
                int value = game->board.terrain[row][col].value;
                art_image_rot(art, 4, value - 1, x,
                              y + (row < 4 ? -10.0f : 10.0f),
                              rot, 1.0f, 1.0f);
            }

            if (is_move_target(game, row, col)) {
                art->alpha = 0.40f;
                art->color = GOLD;
                art_rect(art, x - 30.0f, y - 30.0f, 60.0f, 60.0f);
                art->alpha = 1.0f;
                art->color = WHITE;
            }
        }
    }

    /* The original frames the last move with a thin orange outline, even
       after the game has finished. */
    if (game->has_guide) {
        draw_guide(art, portrait, game->guide_from.row,
                   game->guide_from.col);
        draw_guide(art, portrait, game->guide_to.row,
                   game->guide_to.col);
    }

    if (game->check_timer > 0) {
        float x;
        float y;
        layout_cell_center(portrait, game->check_cell.row,
                           game->check_cell.col, &x, &y);
        art->alpha = 0.50f;
        art->color = CHECK;
        art_rect(art, x - 30.0f, y - 30.0f, 60.0f, 60.0f);
        art->alpha = 1.0f;
        art->color = WHITE;
    }

    if (game->has_hover) {
        float x;
        float y;
        layout_cell_center(portrait, game->hover_cell.row,
                           game->hover_cell.col, &x, &y);
        x -= 30.0f;
        y -= 30.0f;
        art->alpha = 0.45f;
        art->color = HOVER;
        art_rect(art, x + 2.0f, y + 2.0f, 56.0f, 2.0f);
        art_rect(art, x + 2.0f, y + 56.0f, 56.0f, 2.0f);
        art_rect(art, x + 2.0f, y + 2.0f, 2.0f, 56.0f);
        art_rect(art, x + 56.0f, y + 2.0f, 2.0f, 56.0f);
        art->alpha = 1.0f;
        art->color = WHITE;
    }
}

static void draw_pause_control(Art *art, int portrait)
{
    Rect control;
    layout_pause_control(portrait, &control);
    art->color = WHITE;
    art_rect(art, control.x, control.y, control.w, control.h);
    art->color = DARK_GREEN;
    art_rect(art, control.x + 2.0f, control.y + 2.0f,
             control.w - 4.0f, control.h - 4.0f);

    if (portrait) {
        art->color = WHITE;
        art_rect(art, 440.0f, 17.0f, 6.0f, 22.0f);
        art_rect(art, 452.0f, 17.0f, 6.0f, 22.0f);
    } else {
        label(art, "PAUSE / MENU  (ESC)", 591.0f, 498.0f,
              1.0f, WHITE, 0);
    }
}

static void status_label(Art *art, const char *text, float y, Color color)
{
    label(art, text, 735.0f, y, 1.0f, color, 1);
}

static void draw_turn_indicator(Art *art, const Game *game, int portrait)
{
    const char *text = game->turn == SIDE_RED ? "RED TO MOVE" : "WHITE TO MOVE";
    Color color = game->turn == SIDE_RED ? RED_TEXT : PALE_TEXT;
    float cx = portrait ? 240.0f : 735.0f;
    float cy = portrait ? 122.0f : 152.0f;
    float w = portrait ? 250.0f : 300.0f;
    float h = 36.0f;

    art->alpha = 0.85f;
    art->color = BLACK;
    art_rect(art, cx - w / 2.0f, cy - h / 2.0f, w, h);
    art->alpha = 1.0f;
    art->color = color;
    art_rect(art, cx - w / 2.0f, cy - h / 2.0f, 4.0f, h);
    label(art, text, cx, cy - 8.0f, 1.0f, color, 1);
}

static void draw_captured(Art *art, int image, int pawns, int shogun, float y)
{
    float x = 735.0f - (float)(pawns + (shogun ? 1 : 0) - 1) * 20.0f;
    art->color = CAPTURED;
    for (int i = 0; i < pawns; ++i) {
        art_image(art, image, 0, x + (float)i * 40.0f, y, 1.0f, 1.0f);
    }
    if (shogun) art_image(art, image, 1, x + (float)pawns * 40.0f, y,
                          1.0f, 1.0f);
    art->color = WHITE;
}

static void draw_status(Art *art, const Game *game, int portrait)
{
    if (!portrait) {
        char red_name[ONLINE_OPPONENT_SIZE + 12];
        char white_name[ONLINE_OPPONENT_SIZE + 12];
        if (game->local_side >= 0) {
            if (game->local_side == SIDE_RED) {
                snprintf(red_name, sizeof(red_name), "%s (RED)", game->name);
                snprintf(white_name, sizeof(white_name), "%s (WHITE)",
                         game->opponent);
            } else {
                snprintf(red_name, sizeof(red_name), "%s (RED)",
                         game->opponent);
                snprintf(white_name, sizeof(white_name), "%s (WHITE)",
                         game->name);
            }
        } else {
            snprintf(red_name, sizeof(red_name), "%s",
                     game->two_player ? "RED PLAYER" : "RED - COMPUTER");
            snprintf(white_name, sizeof(white_name), "%s",
                     game->two_player ? "WHITE PLAYER" : "WHITE - YOU");
        }
        const char *red = red_name;
        const char *white = white_name;
        status_label(art, red, 40.0f, RED_TEXT);
        draw_captured(art, 3, 7 - board_count_pawns(&game->board, SIDE_WHITE),
                      !board_has_shogun(&game->board, SIDE_WHITE), 92.0f);
        status_label(art, white, 380.0f, PALE_TEXT);
        draw_captured(art, 2, 7 - board_count_pawns(&game->board, SIDE_RED),
                      !board_has_shogun(&game->board, SIDE_RED), 432.0f);
    }

    if (game->screen == SCREEN_WINNER) return;

    draw_turn_indicator(art, game, portrait);

    if (game->thinking) {
        art->color = WHITE;
        if (portrait) art_image(art, 5, 1, 240.0f, 700.0f, 1.0f, 1.0f);
        else {
            art_image(art, 5, 1, 735.0f, 210.0f, 1.0f, 1.0f);
            status_label(art, "THINKING...", 254.0f, GRAY_TEXT);
        }
    }

    draw_pause_control(art, portrait);
}

static void draw_pause(Art *art, const Game *game, int portrait)
{
    draw_board(art, game, portrait);
    effects_draw_check(art, game, portrait);
    draw_status(art, game, portrait);
    if (portrait) return;

    int w = layout_width(portrait);
    int h = layout_height(portrait);
    art->alpha = 0.60f;
    art->color = BLACK;
    art_rect(art, 0.0f, 0.0f, (float)w, (float)h);
    art->alpha = 1.0f;

    art->color = WHITE;
    art_rect(art, 58.0f, 138.0f, 424.0f, 244.0f);
    art->color = BLACK;
    art_rect(art, 60.0f, 140.0f, 420.0f, 240.0f);
    label(art, "*PAUSED*", 270.0f, 180.0f, 2.0f, WHITE, 1);

    Rect outer;
    Rect inner;
    layout_pause_button(portrait, 0, &outer, &inner);
    art->color = GOLD;
    art_rect(art, outer.x, outer.y, outer.w, outer.h);
    art->color = GREEN_BUTTON;
    art_rect(art, inner.x, inner.y, inner.w, inner.h);
    label(art, "CONTINUE", 270.0f, 255.0f, 2.0f, WHITE, 1);

    layout_pause_button(portrait, 1, &outer, &inner);
    art->color = WHITE;
    art_rect(art, outer.x, outer.y, outer.w, outer.h);
    art->color = MAROON;
    art_rect(art, inner.x, inner.y, inner.w, inner.h);
    label(art, "MAIN MENU", 270.0f, 321.0f, 2.0f, WHITE, 1);

    label(art, "ESC AGAIN FOR MENU", 270.0f, 362.0f, 1.0f, HINT, 1);
}

typedef struct {
    float y;
    const char *text;
} HelpLine;

static void draw_help_lines(Art *art, const HelpLine *lines, int count)
{
    for (int i = 0; i < count; ++i) {
        if (lines[i].text[0]) {
            label(art, lines[i].text, 32.0f, lines[i].y,
                  1.0f, WHITE, 0);
        }
    }
}

static void draw_help(Art *art, const Game *game, int portrait)
{
    (void)portrait;
    art_clear(art, (Color){0, 0, 50});

    static const HelpLine page1[] = {
        {82.0f, "SHOGUN is a one or two"},
        {102.0f, "player game where you have"},
        {122.0f, "to capture the opponent"},
        {142.0f, "SHOGUN, or leave him with"},
        {162.0f, "only 1 pawn remaining as"},
        {182.0f, "well as his SHOGUN piece."},
        {216.0f, "The board has hidden terrain"},
        {236.0f, "values that determine how"},
        {256.0f, "far a piece moves in a turn."},
        {276.0f, "This is exactly 1 to 4"},
        {296.0f, "squares for a pawn, but only"},
        {316.0f, "1 or 2 for the SHOGUN."},
        {350.0f, "Pieces cannot move"},
        {370.0f, "diagonally, jump over any"},
        {390.0f, "other pieces, double back,"},
        {410.0f, "or make more than one turn"},
        {430.0f, "in a move. Land on an enemy"},
        {450.0f, "piece to capture it. See the"},
        {470.0f, "diagram."}
    };
    static const HelpLine page2[] = {
        {82.0f, "On your turn click the piece you wish to move, then click"},
        {102.0f, "where to move it to. Click the piece again to cancel."},
        {134.0f, "With a keyboard or controller, move the cursor with the"},
        {154.0f, "arrow keys or d-pad and select with ENTER, SPACE or (A)."},
        {174.0f, "ESC or START pauses the game."},
        {206.0f, "In a one player game you play WHITE against the computer."},
        {226.0f, "In a two player game share the mouse and take turns -"},
        {246.0f, "white plays up the board, red plays down it."},
        {278.0f, "Press F to switch fullscreen on and off."},
        {374.0f, "CLASSIC looks a move or two ahead and plays steadily."},
        {394.0f, "IMPROVED searches further, weighs material and shogun"},
        {414.0f, "safety, and is much harder to beat. Choose it on the"},
        {434.0f, "main menu."}
    };
    static const HelpLine page3[] = {
        {82.0f, "Aim to corner and capture the enemy SHOGUN while"},
        {102.0f, "protecting your own pawns and SHOGUN from capture by"},
        {122.0f, "enemy forces."},
        {154.0f, "If a move places the opponent SHOGUN in danger of capture"},
        {174.0f, "a 'check' warning will appear on the screen briefly. This"},
        {194.0f, "will not happen if you place your own SHOGUN in danger!"},
        {226.0f, "Although the terrain will be different in every game, try"},
        {246.0f, "remembering the terrain values so you can build up a"},
        {266.0f, "mental map of the best routes across the board."},
        {298.0f, "The main menu options that let you see the full board"},
        {318.0f, "terrain values, and that let you see the valid moves a"},
        {338.0f, "piece can make, are only there for new players - having"},
        {358.0f, "them turned *off* is recommended once you are used to the"},
        {378.0f, "game rules and play."}
    };

    switch (game->help_page) {
    case 0:
        label(art, "ABOUT THE GAME", 40.0f, 36.0f, 2.0f, WHITE, 0);
        draw_help_lines(art, page1, (int)(sizeof(page1) / sizeof(page1[0])));
        label(art, "PAGE 1/3 - CLICK OR ENTER: NEXT    ESC: MENU",
              480.0f, 522.0f, 1.0f, WHITE, 1);
        break;
    case 1:
        label(art, "CONTROLS/PLAY", 40.0f, 36.0f, 2.0f, WHITE, 0);
        draw_help_lines(art, page2, (int)(sizeof(page2) / sizeof(page2[0])));
        label(art, "THE COMPUTER", 40.0f, 330.0f, 2.0f, WHITE, 0);
        label(art, "PAGE 2/3 - CLICK OR ENTER: NEXT    ESC: MENU",
              480.0f, 522.0f, 1.0f, WHITE, 1);
        break;
    default:
        label(art, "GAME STRATEGY", 40.0f, 36.0f, 2.0f, WHITE, 0);
        draw_help_lines(art, page3, (int)(sizeof(page3) / sizeof(page3[0])));
        label(art, "*THANK YOU*", 480.0f, 436.0f, 2.0f, WHITE, 1);
        label(art, "ENJOY THE GAME", 480.0f, 476.0f, 2.0f, WHITE, 1);
        label(art, "PAGE 3/3 - CLICK OR ENTER: MENU",
              480.0f, 522.0f, 1.0f, WHITE, 1);
        break;
    }
}

static void draw_winner_menu(Art *art, const Game *game, int portrait)
{
    for (int i = 0; i < 2; ++i) {
        Rect outer;
        Rect inner;
        layout_winner_button(portrait, i, &outer, &inner);
        int active = game->winner_cursor == i || game->hover_item == i;
        Color edge = active ? GOLD : WHITE;
        Color fill = i == 0 ? GREEN_BUTTON : MAROON;

        art->alpha = 1.0f;
        art->color = edge;
        art_rect(art, outer.x, outer.y, outer.w, outer.h);
        art->color = fill;
        art_rect(art, inner.x, inner.y, inner.w, inner.h);

        label(art, i == 0 ? "PLAY AGAIN" : "MAIN MENU",
              inner.x + inner.w / 2.0f, inner.y + 14.0f,
              1.5f, WHITE, 1);
    }

    label(art, "ENTER: SELECT    UP/DOWN: CHOOSE",
          portrait ? 240.0f : 735.0f,
          portrait ? 766.0f : 422.0f, 1.0f, HINT, 1);
}

/* The original banner drifts right and back by a few pixels. */
static float winner_banner_drift(const Game *game)
{
    int phase = game->win_timer % 80;
    return (float)(phase < 40 ? phase : 80 - phase) / 5.0f;
}

static void draw_winner(Art *art, const Game *game, int portrait)
{
    /* Keep the no-sound winner board, brackets and drifting banner, then add
       the wordless logo impact and a small rematch menu. */
    draw_board(art, game, portrait);
    effects_draw_check(art, game, portrait);
    draw_status(art, game, portrait);
    effects_draw_win(art, game, portrait);

    float drift = winner_banner_drift(game);
    art->color = WHITE;
    art_image(art, 5, game->winner == SIDE_WHITE ? 4 : 3,
              (portrait ? 240.0f : 758.0f) + (portrait ? drift * 0.5f : drift),
              portrait ? 430.0f : 230.0f, 0.9f, 0.9f);
    draw_winner_menu(art, game, portrait);
}

void render_screen(Art *art, const Game *game, int portrait, uint32_t now)
{
    switch (game->screen) {
    case SCREEN_TITLE:
    case SCREEN_MENU:
        draw_menu(art, game, portrait);
        break;
    case SCREEN_PLAY:
        draw_board(art, game, portrait);
        effects_draw_check(art, game, portrait);
        draw_status(art, game, portrait);
        break;
    case SCREEN_PAUSE:
        draw_pause(art, game, portrait);
        break;
    case SCREEN_HELP:
        draw_help(art, game, portrait);
        break;
    case SCREEN_WINNER:
        draw_winner(art, game, portrait);
        break;
    case SCREEN_PLAY_MODE:
        draw_play_mode(art, game, portrait);
        break;
    case SCREEN_ONLINE_SETUP:
        draw_online_setup(art, game, portrait, now);
        break;
    case SCREEN_ONLINE_NAME:
        draw_online_name(art, game, portrait, now);
        break;
    case SCREEN_ONLINE_WAIT:
        draw_online_wait(art, game, portrait);
        break;
    case SCREEN_ONLINE_MATCH:
        draw_online_match(art, game, portrait);
        break;
    }
}
