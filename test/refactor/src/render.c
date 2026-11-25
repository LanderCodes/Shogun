#include "render.h"

#include "font.h"
#include "layout.h"
#include "menu.h"
#include "rules.h"

#include <stdio.h>

static const Color WHITE = {255, 255, 255};
static const Color BLACK = {0, 0, 0};
static const Color YELLOW = {255, 232, 96};
static const Color RED = {255, 96, 96};

static void label(Art *art, const char *text, float x, float y,
                  float scale, Color color, int centered)
{
    art->color = color;
    font_text(art, text, x, y, scale, centered);
    art->color = WHITE;
}

static void background(Art *art, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    float cell = layout_cell(portrait);
    art_clear(art, BLACK);
    for (int row = 0; row * cell < h; ++row) {
        for (int col = 0; col * cell < w; ++col) {
            art_image(art, 1, (row + col) & 1,
                      (float)col * cell + cell / 2.0f,
                      (float)row * cell + cell / 2.0f, 1.0f, 1.0f);
        }
    }
}

static void draw_menu(Art *art, const Game *game, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    art->color = WHITE;
    art_image(art, 0, 0, (float)w / 2.0f, portrait ? 110.0f : 82.0f,
              portrait ? 1.25f : 1.5f, portrait ? 1.25f : 1.5f);

    for (int i = 0; i < MENU_ITEMS; ++i) {
        Rect box;
        layout_menu_item(portrait, i, &box);
        int active = i == game->menu_cursor;
        art->color = active ? YELLOW : WHITE;
        art_rect(art, box.x - 2.0f, box.y - 2.0f, box.w + 4.0f, box.h + 4.0f);
        art->color = BLACK;
        art_rect(art, box.x, box.y, box.w, box.h);
        art->color = WHITE;
        char status[48];
        menu_status(i, &game->settings, status, sizeof(status));
        label(art, menu_name(i), box.x + box.w / 2.0f, box.y + 20.0f,
              portrait ? 1.0f : 2.0f, active ? YELLOW : WHITE, 1);
        label(art, status, box.x + box.w / 2.0f, box.y + box.h - 18.0f,
              1.0f, active ? YELLOW : WHITE, 1);
    }
    art->color = WHITE;

    const char *hint = portrait ? "TAP FOR MENU"
                                : "MOUSE, KEYBOARD OR CONTROLLER";
    label(art, hint, (float)w / 2.0f, (float)h - 66.0f, 1.0f, WHITE, 1);
}

static void draw_board(Art *art, const Game *game, int portrait)
{
    Rect board;
    layout_board(portrait, &board);
    float cell = layout_cell(portrait);

    art->color = WHITE;
    art_rect(art, board.x - 3.0f, board.y - 3.0f,
             board.w + 6.0f, board.h + 6.0f);
    art->color = BLACK;
    art_rect(art, board.x, board.y, board.w, board.h);

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            float x;
            float y;
            layout_cell_center(portrait, row, col, &x, &y);
            art_image(art, 1, (row + col) & 1, x, y,
                      cell / 60.0f, cell / 60.0f);
        }
    }

    if (game->settings.show_moves && game->has_selection) {
        art->color = YELLOW;
        for (int i = 0; i < game->move_count; ++i) {
            float x;
            float y;
            layout_cell_center(portrait, game->moves[i].to.row,
                               game->moves[i].to.col, &x, &y);
            art_rect(art, x - cell * 0.18f, y - cell * 0.18f,
                     cell * 0.36f, cell * 0.36f);
        }
        art->color = WHITE;
    }

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece *piece = &game->board.piece[row][col];
            if (piece->type == PIECE_NONE) {
                if (game->settings.show_terrain ||
                    game->board.terrain[row][col].revealed) {
                    float x;
                    float y;
                    layout_cell_center(portrait, row, col, &x, &y);
                    art->color = (Color){128, 128, 128};
                    art_image(art, 4, game->board.terrain[row][col].value - 1,
                              x, y, cell / 60.0f * 0.7f, cell / 60.0f * 0.7f);
                    art->color = WHITE;
                }
                continue;
            }
            float x;
            float y;
            layout_cell_center(portrait, row, col, &x, &y);
            int id = piece->side == SIDE_WHITE ? 3 : 2;
            art_image(art, id, piece->type == PIECE_SHOGUN ? 1 : 0,
                      x, y, cell / 60.0f, cell / 60.0f);
            art_image(art, 4, piece->value - 1, x, y,
                      cell / 60.0f * 0.75f, cell / 60.0f * 0.75f);
        }
    }

    if (game->screen == SCREEN_PLAY) {
        float x;
        float y;
        layout_cell_center(portrait, game->cursor.row, game->cursor.col, &x, &y);
        art->color = YELLOW;
        art_rect(art, x - 2.0f, y - cell / 2.0f, 4.0f, cell);
        art_rect(art, x - cell / 2.0f, y - 2.0f, cell, 4.0f);
        art->color = WHITE;
    }
}

static void draw_status(Art *art, const Game *game, int portrait)
{
    Rect panel;
    layout_panel(portrait, &panel);
    const char *side = game->turn == SIDE_WHITE ? "WHITE" : "RED";
    char line[64];

    if (game->two_player) {
        snprintf(line, sizeof(line), "%s PLAYER", side);
    } else {
        snprintf(line, sizeof(line), "%s - YOU",
                 game->turn == SIDE_WHITE ? "WHITE" : "RED");
    }
    label(art, line, panel.x + panel.w / 2.0f, panel.y + 24.0f,
          1.0f, WHITE, 1);

    if (game->check_timer > 0) {
        label(art, "CHECK!", panel.x + panel.w / 2.0f, panel.y + 60.0f,
              1.0f, RED, 1);
    }

    Move moves[MAX_MOVES];
    int count = rules_all(&game->board, game->turn, moves, MAX_MOVES);
    if (count == 0) {
        label(art, "NO MOVES - TURN PASSES",
              panel.x + panel.w / 2.0f, panel.y + panel.h - 34.0f,
              1.0f, YELLOW, 1);
    }
}

static void draw_help(Art *art, int portrait)
{
    static const char *lines[] = {
        "SHOGUN - RULES",
        "",
        "CAPTURE THE ENEMY SHOGUN",
        "OR LEAVE HIM WITH ONE PAWN.",
        "",
        "PIECES MOVE EXACTLY THE",
        "NUMBER OF SQUARES SHOWN:",
        "SHOGUN 1 OR 2, PAWNS 1 TO 4.",
        "",
        "NO DIAGONALS, NO JUMPS,",
        "NO DOUBLING BACK, ONE TURN.",
        "",
        "SELECT A PIECE THEN A SQUARE.",
        "ESC PAUSES. SELECT RETURNS."
    };
    int w = layout_width(portrait);

    art->color = WHITE;
    if (portrait) {
        art_image(art, 6, 0, (float)w / 2.0f, 260.0f, 0.7f, 0.7f);
        for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
            label(art, lines[i], (float)w / 2.0f, 470.0f + (float)i * 21.0f,
                  1.0f, WHITE, 1);
        }
    } else {
        art_image(art, 6, 0, 210.0f, 270.0f, 0.85f, 0.85f);
        for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
            label(art, lines[i], 470.0f, 50.0f + (float)i * 31.0f,
                  1.0f, WHITE, 0);
        }
    }
}

static void draw_about(Art *art, int portrait)
{
    static const char *lines[] = {
        "ABOUT THE GAME",
        "",
        "BASED ON THE BOARD GAME BY",
        "TERUO MATSUMOTO, 1976.",
        "",
        "PC / WEB EDITION BY",
        "TCKSOFT, 2013.",
        "",
        "THANK YOU TO DR BEEP.",
        "",
        "SELECT TO RETURN."
    };
    float top = portrait ? 200.0f : 120.0f;
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        label(art, lines[i], (float)layout_width(portrait) / 2.0f,
              top + (float)i * 32.0f, 1.0f, WHITE, 1);
    }
}

static void draw_overlay(Art *art, const Game *game, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    art->color = (Color){0, 0, 0};
    art->alpha = 0.72f;
    art_rect(art, 0.0f, 0.0f, (float)w, (float)h);
    art->alpha = 1.0f;
    label(art, "*PAUSED*", (float)w / 2.0f, (float)h / 2.0f - 30.0f,
          2.0f, YELLOW, 1);
    label(art, "PAUSE / MENU  (ESC)", (float)w / 2.0f,
          (float)h / 2.0f + 30.0f, 1.0f, WHITE, 1);
    (void)game;
}

void render_screen(Art *art, const Game *game, int portrait, uint32_t now)
{
    (void)now;
    background(art, portrait);

    switch (game->screen) {
    case SCREEN_TITLE:
        art->color = WHITE;
        art_image(art, 0, 0, (float)layout_width(portrait) / 2.0f,
                  portrait ? 260.0f : 200.0f,
                  portrait ? 1.5f : 2.0f, portrait ? 1.5f : 2.0f);
        label(art, portrait ? "TAP FOR MENU" : "CLICK / ENTER FOR MENU",
              (float)layout_width(portrait) / 2.0f,
              (float)layout_height(portrait) - 100.0f, 1.0f, WHITE, 1);
        break;
    case SCREEN_MENU:
        draw_menu(art, game, portrait);
        break;
    case SCREEN_PLAY:
    case SCREEN_PAUSE:
        draw_board(art, game, portrait);
        draw_status(art, game, portrait);
        if (game->screen == SCREEN_PAUSE) draw_overlay(art, game, portrait);
        break;
    case SCREEN_HELP:
        draw_help(art, portrait);
        break;
    case SCREEN_ABOUT:
        draw_about(art, portrait);
        break;
    case SCREEN_WINNER: {
        const char *text = game->winner == SIDE_WHITE ? "WHITE WINS" : "RED WINS";
        label(art, "GAME OVER", (float)layout_width(portrait) / 2.0f,
              (float)layout_height(portrait) / 2.0f - 40.0f,
              2.0f, YELLOW, 1);
        label(art, text, (float)layout_width(portrait) / 2.0f,
              (float)layout_height(portrait) / 2.0f + 20.0f,
              1.0f, WHITE, 1);
        label(art, "*THANK YOU*", (float)layout_width(portrait) / 2.0f,
              (float)layout_height(portrait) / 2.0f + 70.0f,
              1.0f, WHITE, 1);
        break;
    }
    }
}
