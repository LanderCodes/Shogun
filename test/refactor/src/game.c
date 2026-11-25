#include "game.h"

#include "ai.h"
#include "board.h"
#include "layout.h"
#include "rules.h"

#include <string.h>

static void set_check(Game *game, const Cell *moved_to)
{
    if (!moved_to) {
        game->check_timer = 0;
        game->check_cell.row = 0;
        game->check_cell.col = 0;
        return;
    }

    game->check_timer = rules_in_check(&game->board, game->turn) ? 80 : 0;
    game->check_cell = game->check_timer > 0 ? *moved_to : (Cell){0, 0};
}

static void start_game(Game *game, int two_player)
{
    game->two_player = two_player;
    game->screen = SCREEN_PLAY;
    game->turn = board_starting_side(&game->board);
    game->cursor.row = BOARD_SIZE - 1;
    game->cursor.col = 0;
    game->has_selection = 0;
    game->winner = -1;
    game->check_timer = 0;
    game->win_timer = 0;
    game->check_cell.row = 0;
    game->check_cell.col = 0;
    game->guide_from.row = -1;
    game->guide_from.col = -1;
    game->guide_to.row = -1;
    game->guide_to.col = -1;
    game->has_guide = 0;
    game->thinking = !two_player && game->turn == SIDE_RED ? 12 : 0;
}

static void restart_game(Game *game)
{
    /* Each rematch gets a fresh terrain shuffle without changing settings. */
    game->seed = game->seed * 1664525u + 1013904223u;
    board_init(&game->board, game->seed);
    start_game(game, game->two_player);
}

static void activate_menu(Game *game)
{
    switch (game->menu_cursor) {
    case 0: start_game(game, 0); break;
    case 1: start_game(game, 1); break;
    case 2:
        game->back = SCREEN_MENU;
        game->help_page = 0;
        game->screen = SCREEN_HELP;
        break;
    case 3:
        game->settings.show_moves = !game->settings.show_moves;
        break;
    case 4:
        game->settings.show_terrain = !game->settings.show_terrain;
        break;
    case 5:
        game->settings.improved_ai = !game->settings.improved_ai;
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
    game->guide_from = move->from;
    game->guide_to = move->to;
    game->has_guide = 1;

    game->winner = rules_winner(&game->board);
    if (game->winner >= 0) {
        game->win_timer = 0;
        game->winner_cursor = 0;
        game->screen = SCREEN_WINNER;
        return;
    }

    game->turn = other_side(game->turn);
    set_check(game, &move->to);

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
    game->seed = seed;
    board_init(&game->board, game->seed);
    game->settings = *settings;
    game->screen = SCREEN_MENU;
    game->back = SCREEN_MENU;
    game->winner = -1;
    game->cursor.row = BOARD_SIZE - 1;
    game->cursor.col = 0;
    game->check_cell.row = 0;
    game->check_cell.col = 0;
    game->guide_from.row = -1;
    game->guide_from.col = -1;
    game->guide_to.row = -1;
    game->guide_to.col = -1;
    game->has_guide = 0;
}

void game_update(Game *game)
{
    ++game->tick;
    if (game->check_timer > 0) --game->check_timer;
    if (game->screen == SCREEN_WINNER) ++game->win_timer;

    if (game->screen == SCREEN_PLAY && game->thinking > 0 && --game->thinking == 0) {
        Move move = ai_choose(&game->board, game->turn, game->settings.improved_ai);
        if (move.to.row == move.from.row && move.to.col == move.from.col) {
            /* No legal move: the turn passes. */
            game->turn = other_side(game->turn);
            set_check(game, NULL);
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
        if (code == 6) game->screen = game->back;
        else if (code == 5) {
            ++game->help_page;
            if (game->help_page > 2) game->screen = game->back;
        }
        break;
    case SCREEN_WINNER:
        if (code == 1 || code == 3) game->winner_cursor = 0;
        else if (code == 2 || code == 4) game->winner_cursor = 1;
        else if (code == 5) {
            if (game->winner_cursor == 0) restart_game(game);
            else game->screen = SCREEN_MENU;
        } else if (code == 6) {
            game->screen = SCREEN_MENU;
        }
        break;
    }
}

void game_mouse_move(Game *game, float x, float y, int portrait)
{
    game->hover_item = -1;
    if (game->screen == SCREEN_MENU) {
        game->hover_item = layout_menu_item_at(portrait, x, y);
    } else if (game->screen == SCREEN_WINNER) {
        game->hover_item = layout_winner_button_at(portrait, x, y);
    }

    int row;
    int col;
    game->has_hover = game->screen == SCREEN_PLAY &&
                      layout_cell_at(portrait, x, y, &row, &col);
    if (game->has_hover) game->hover_cell = (Cell){row, col};
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
        Rect control;
        layout_pause_control(portrait, &control);
        if (x >= control.x && x <= control.x + control.w &&
            y >= control.y && y <= control.y + control.h) {
            game->screen = SCREEN_PAUSE;
            break;
        }
        int row;
        int col;
        if (layout_cell_at(portrait, x, y, &row, &col)) {
            select_cell(game, row, col);
        }
        break;
    }
    case SCREEN_PAUSE: {
        if (portrait) {
            Rect control;
            layout_pause_control(portrait, &control);
            if (x >= control.x && x <= control.x + control.w &&
                y >= control.y && y <= control.y + control.h) {
                game->screen = SCREEN_PLAY;
            }
            break;
        }
        /* Landscape uses the CONTINUE and MAIN MENU overlay buttons. */
        Rect outer;
        Rect inner;
        layout_pause_button(portrait, 0, &outer, &inner);
        if (x >= outer.x && x <= outer.x + outer.w &&
            y >= outer.y && y <= outer.y + outer.h) {
            game->screen = SCREEN_PLAY;
        } else {
            layout_pause_button(portrait, 1, &outer, &inner);
            if (x >= outer.x && x <= outer.x + outer.w &&
                y >= outer.y && y <= outer.y + outer.h) {
                game->screen = SCREEN_MENU;
            }
        }
        break;
    }
    case SCREEN_HELP:
        ++game->help_page;
        if (game->help_page > 2) game->screen = game->back;
        break;
    case SCREEN_WINNER: {
        int button = layout_winner_button_at(portrait, x, y);
        if (button == 0) restart_game(game);
        else if (button == 1) game->screen = SCREEN_MENU;
        break;
    }
    }
}
