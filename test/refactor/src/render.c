#include "render.h"

#include "board.h"
#include "font.h"
#include "layout.h"
#include "menu.h"
#include "rules.h"

#include <stdio.h>
#include <string.h>

/* Colours copied from the original game's P_color calls. */
static const Color WHITE = {255, 255, 255};
static const Color BLACK = {0, 0, 0};
static const Color NAVY_MENU = {0, 0, 64};
static const Color NAVY_PLAY = {0, 0, 32};
static const Color BOARD_FRAME = {90, 90, 120};
static const Color GOLD = {255, 255, 0};
static const Color SETTING_ON = {80, 255, 80};
static const Color SETTING_OFF = {255, 90, 90};
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

/* The menu and title screens use a checkerboard of 60px tiles. */
static void draw_tiles(Art *art, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);
    art->color = WHITE;
    for (int row = 0; row * 60 < h; ++row) {
        for (int col = 0; col * 60 < w; ++col) {
            art_image(art, 1, (row + col) & 1,
                      (float)col * 60.0f + 30.0f,
                      (float)row * 60.0f + 30.0f, 1.0f, 1.0f);
        }
    }
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

static Color menu_text_color(int index, const Settings *settings)
{
    if (index == 3) return settings->show_moves ? SETTING_ON : SETTING_OFF;
    return WHITE;
}

static void draw_menu(Art *art, const Game *game, int portrait)
{
    int w = layout_width(portrait);
    int h = layout_height(portrait);

    art_clear(art, NAVY_MENU);
    draw_tiles(art, portrait);
    art->alpha = 0.45f;
    art->color = (Color){0, 0, 40};
    art_rect(art, 0.0f, 0.0f, (float)w, (float)h);
    art->alpha = 1.0f;
    draw_logo(art, portrait);

    /* The original paints every box before it paints any text. */
    for (int i = 0; i < MENU_ITEMS; ++i) {
        Rect box;
        layout_menu_item(portrait, i, &box);
        Color base = box_color(portrait, i);
        int active = i == game->menu_cursor;

        art->alpha = active ? 0.70f : 1.0f;
        art->color = active ? GOLD : WHITE;
        art_rect(art, box.x, box.y, box.w, box.h);
        art->alpha = active ? 0.80f : 1.0f;
        art->color = active ? box_highlight(base) : base;
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
    art_clear(art, NAVY_PLAY);
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
        const char *red = game->two_player ? "RED PLAYER" : "RED - COMPUTER";
        const char *white = game->two_player ? "WHITE PLAYER" : "WHITE - YOU";
        status_label(art, red, 40.0f, RED_TEXT);
        draw_captured(art, 3, 7 - board_count_pawns(&game->board, SIDE_WHITE),
                      !board_has_shogun(&game->board, SIDE_WHITE), 92.0f);
        status_label(art, white, 380.0f, PALE_TEXT);
        draw_captured(art, 2, 7 - board_count_pawns(&game->board, SIDE_RED),
                      !board_has_shogun(&game->board, SIDE_RED), 432.0f);
    }

    if (game->screen == SCREEN_WINNER) return;

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
        {374.0f, "CLASSIC is the original 2013 computer player. IMPROVED"},
        {394.0f, "follows the same plan but checks every move for danger"},
        {414.0f, "before making it, so it is much harder to beat. Choose it"},
        {434.0f, "on the main menu."}
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

static void draw_winner(Art *art, const Game *game, int portrait)
{
    /* The original keeps the finished board visible and stamps a ready-made
       gfxtext banner over it. */
    draw_board(art, game, portrait);
    draw_status(art, game, portrait);
    art->color = WHITE;
    art_image(art, 5, 4, portrait ? 240.0f : 758.0f,
              portrait ? 430.0f : 230.0f, 0.9f, 0.9f);
}

void render_screen(Art *art, const Game *game, int portrait, uint32_t now)
{
    (void)now;
    switch (game->screen) {
    case SCREEN_TITLE:
    case SCREEN_MENU:
        draw_menu(art, game, portrait);
        break;
    case SCREEN_PLAY:
        draw_board(art, game, portrait);
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
    }
}
