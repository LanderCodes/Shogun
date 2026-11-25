/* Headless checks for the pure-C game logic. No SDL, no config file written. */

#include <assert.h>
#include <stdio.h>

#include "game.h"
#include "layout.h"
#include "rules.h"

/* game.c only needs this when a setting is toggled. */
void config_save(const Settings *settings) { (void)settings; }

static int piece_count(const Game *game, Side side)
{
    int count = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece *piece = &game->board.piece[row][col];
            if (piece->type != PIECE_NONE && piece->side == side) ++count;
        }
    }
    return count;
}

int main(void)
{
    Settings settings = {0, 0, 0};
    Game game;
    game_init(&game, 12345u, &settings);

    assert(game.screen == SCREEN_TITLE);
    assert(piece_count(&game, SIDE_WHITE) == 8);
    assert(piece_count(&game, SIDE_RED) == 8);

    game_key(&game, 5);                 /* title -> menu */
    assert(game.screen == SCREEN_MENU);
    game_key(&game, 5);                 /* one player */
    assert(game.screen == SCREEN_PLAY);

    Move moves[MAX_MOVES];
    int count = rules_all(&game.board, SIDE_WHITE, moves, MAX_MOVES);
    assert(count > 0);

    /* Select a piece and play one of its legal moves with the mouse. */
    Move pick = moves[0];
    float x;
    float y;
    layout_cell_center(0, pick.from.row, pick.from.col, &x, &y);
    game_mouse_down(&game, x, y, 0);
    assert(game.has_selection);
    layout_cell_center(0, pick.to.row, pick.to.col, &x, &y);
    game_mouse_down(&game, x, y, 0);
    assert(!game.has_selection);
    assert(game.turn == SIDE_RED);

    /* Let the computer reply and hand the turn back. */
    for (int i = 0; i < 30; ++i) game_update(&game);
    assert(game.turn == SIDE_WHITE || game.screen == SCREEN_WINNER);

    game_key(&game, 6);                 /* pause */
    assert(game.screen == SCREEN_PAUSE);
    game_key(&game, 6);                 /* menu */
    assert(game.screen == SCREEN_MENU);

    game_key(&game, 2);                 /* down to row 1 */
    game_key(&game, 3);                 /* right to column 1 */
    assert(game.menu_cursor == 3);
    game_key(&game, 5);                 /* toggle move guides */
    assert(game.settings.show_moves == 1);

    /* A missing shogun ends the game. */
    game.board.piece[1][7].type = PIECE_NONE;
    assert(rules_winner(&game.board) == SIDE_WHITE);

    puts("logic test: ok");
    return 0;
}
