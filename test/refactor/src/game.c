#include "game.h"

#include "ai.h"
#include "board.h"
#include "config.h"
#include "layout.h"
#include "rules.h"

#include <string.h>

static void start_game(Game *game, int two_player)
{
    game->two_player = two_player;
    game->screen = SCREEN_PLAY;
    game->turn = SIDE_WHITE;
    game->cursor.row = 6;
    game->cursor.col = 0;
    game->has_selection = 0;
    game->winner = -1;
    game->check_timer = 0;
    game->thinking = 0;
}

static void activate_menu(Game *game)
{
    switch (game->menu_cursor) {
    case 0: start_game(game, 0); break;
    case 1: start_game(game, 1); break;
    case 2:
        game->back = SCREEN_MENU;
        game->screen = SCREEN_HELP;
        break;
    case 3:
        game->settings.show_moves = !game->settings.show_moves;
        config_save(&game->settings);
        break;
    case 4:
        game->settings.show_terrain = !game->settings.show_terrain;
        config_save(&game->settings);
        break;
    case 5:
        game->settings.improved_ai = !game->settings.improved_ai;
        config_save(&game->settings);
        break;
    default: break;
    }
}

static void finish_move(Game *game, const Move *move)
{
    board_apply(&game->board, move);
    game->has_selection = 0;
    game->move_count = 0;
    game->cursor = move->to;

    game->winner = rules_winner(&game->board);
    if (game->winner >= 0) {
        game->screen = SCREEN_WINNER;
        return;
    }

    game->turn = other_side(game->turn);
    game->check_timer = rules_in_check(&game->board, game->turn) ? 60 : 0;

    if (!game->two_player && game->turn == SIDE_RED) game->thinking = 12;
}

static void select_cell(Game *game, int row, int col)
{
    if (!in_board(row, col)) return;
    game->cursor.row = row;
    game->cursor.col = col;

    /* Human input is only accepted on a human turn. */
    if (!game->two_player && game->turn == SIDE_RED) return;

    if (game->has_selection) {
        for (int i = 0; i < game->move_count; ++i) {
            const Move *move = &game->moves[i];
            if (move->to.row == row && move->to.col == col) {
                finish_move(game, move);
                return;
            }
        }
        if (game->selected.row == row && game->selected.col == col) {
            game->has_selection = 0;
            game->move_count = 0;
            return;
        }
    }

    const Piece *piece = &game->board.piece[row][col];
    if (piece->type == PIECE_NONE || piece->side != game->turn) {
        game->has_selection = 0;
        game->move_count = 0;
        return;
    }

    game->selected.row = row;
    game->selected.col = col;
    game->has_selection = 1;
    game->move_count = rules_generate(&game->board, row, col,
                                      game->moves, MAX_MOVES);
    if (game->move_count == 0) game->has_selection = 0;
}

static void move_cursor(Game *game, int code)
{
    int row = game->cursor.row;
    int col = game->cursor.col;
    if (code == 1) row -= 1;
    else if (code == 2) row += 1;
    else if (code == 3) col -= 1;
    else if (code == 4) col += 1;
    if (in_board(row, col)) {
        game->cursor.row = row;
        game->cursor.col = col;
    }
}

void game_init(Game *game, uint32_t seed, const Settings *settings)
{
    memset(game, 0, sizeof(*game));
    board_init(&game->board, seed);
    game->settings = *settings;
    game->screen = SCREEN_TITLE;
    game->back = SCREEN_MENU;
    game->winner = -1;
    game->cursor.row = 6;
    game->cursor.col = 0;
}

void game_update(Game *game)
{
    ++game->tick;
    if (game->check_timer > 0) --game->check_timer;

    if (game->screen == SCREEN_PLAY && game->thinking > 0 && --game->thinking == 0) {
        Move move = ai_choose(&game->board, game->turn, game->settings.improved_ai);
        if (move.to.row == move.from.row && move.to.col == move.from.col) {
            /* No legal move: the turn passes. */
            game->turn = other_side(game->turn);
            game->check_timer = rules_in_check(&game->board, game->turn) ? 60 : 0;
        } else {
            finish_move(game, &move);
        }
    }
}

void game_key(Game *game, int code)
{
    if (code < 1 || code > 6) return;

    switch (game->screen) {
    case SCREEN_TITLE:
        game->screen = SCREEN_MENU;
        break;
    case SCREEN_MENU:
        if (code <= 4) {
            int row = game->menu_cursor / 2;
            int col = game->menu_cursor % 2;
            if (code == 1) row = (row + 2) % 3;
            else if (code == 2) row = (row + 1) % 3;
            else if (code == 3) col = 0;
            else if (code == 4) col = 1;
            game->menu_cursor = row * 2 + col;
        } else if (code == 5) {
            activate_menu(game);
        } else {
            game->screen = SCREEN_TITLE;
        }
        break;
    case SCREEN_PLAY:
        if (code == 6) game->screen = SCREEN_PAUSE;
        else if (code == 5) select_cell(game, game->cursor.row, game->cursor.col);
        else move_cursor(game, code);
        break;
    case SCREEN_PAUSE:
        if (code == 6) game->screen = SCREEN_MENU;
        else if (code == 5) game->screen = SCREEN_PLAY;
        break;
    case SCREEN_HELP:
        game->screen = code == 5 ? SCREEN_ABOUT : game->back;
        break;
    case SCREEN_ABOUT:
    case SCREEN_WINNER:
        game->screen = SCREEN_MENU;
        break;
    }
}

void game_mouse_move(Game *game, float x, float y, int portrait)
{
    game->hover_item = game->screen == SCREEN_MENU
                           ? layout_menu_item_at(portrait, x, y) : -1;
}

int game_hovering_button(const Game *game)
{
    return game->hover_item >= 0;
}

void game_mouse_down(Game *game, float x, float y, int portrait)
{
    game_mouse_move(game, x, y, portrait);
    switch (game->screen) {
    case SCREEN_TITLE:
        game->screen = SCREEN_MENU;
        break;
    case SCREEN_MENU:
        if (game->hover_item >= 0) {
            game->menu_cursor = game->hover_item;
            activate_menu(game);
        }
        break;
    case SCREEN_PLAY: {
        int row;
        int col;
        if (layout_cell_at(portrait, x, y, &row, &col)) {
            select_cell(game, row, col);
        }
        break;
    }
    case SCREEN_PAUSE:
        game->screen = SCREEN_PLAY;
        break;
    case SCREEN_HELP:
        game->screen = SCREEN_ABOUT;
        break;
    case SCREEN_ABOUT:
    case SCREEN_WINNER:
        game->screen = SCREEN_MENU;
        break;
    }
}
