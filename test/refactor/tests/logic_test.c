/* Headless checks for the pure-C game logic. No SDL, no config file written. */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ai.h"
#include "game.h"
#include "layout.h"
#include "rules.h"

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

    assert(game.screen == SCREEN_MENU);
    assert(piece_count(&game, SIDE_WHITE) == 8);
    assert(piece_count(&game, SIDE_RED) == 8);
    assert(game.board.piece[0][3].type == PIECE_SHOGUN);
    assert(game.board.piece[BOARD_SIZE - 1][4].type == PIECE_SHOGUN);
    assert(game.board.terrain[0][0].value == 2);
    assert(game.board.terrain[0][3].value == 4);
    assert(game.board.terrain[7][4].value == 1);

    game_key(&game, 5);                 /* one player */
    assert(game.screen == SCREEN_PLAY);
    assert(game.turn == SIDE_RED);      /* the computer moves first here */

    /* Let the opening computer move finish before taking the human turn. */
    for (int i = 0; i < 30 && game.turn == SIDE_RED &&
                    game.screen == SCREEN_PLAY; ++i) {
        game_update(&game);
    }
    assert(game.turn == SIDE_WHITE);

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
    assert(game.has_guide);
    assert(game.guide_from.row == pick.from.row &&
           game.guide_from.col == pick.from.col);
    assert(game.guide_to.row == pick.to.row &&
           game.guide_to.col == pick.to.col);

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
    game.board.piece[0][3].type = PIECE_NONE;
    assert(rules_winner(&game.board) == SIDE_WHITE);

    /* Both computer settings must take a shogun capture when it is offered. */
    Board ai_board;
    memset(&ai_board, 0, sizeof(ai_board));
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            ai_board.terrain[row][col].value = 1;
        }
    }
    ai_board.piece[7][4] = (Piece){PIECE_SHOGUN, SIDE_WHITE, 1};
    ai_board.piece[7][3] = (Piece){PIECE_PAWN, SIDE_WHITE, 1};
    ai_board.piece[7][2] = (Piece){PIECE_PAWN, SIDE_WHITE, 1};
    ai_board.piece[6][3] = (Piece){PIECE_SHOGUN, SIDE_RED, 1};
    ai_board.piece[0][3] = (Piece){PIECE_PAWN, SIDE_RED, 1};
    ai_board.piece[0][4] = (Piece){PIECE_PAWN, SIDE_RED, 1};

    for (int improved = 0; improved < 2; ++improved) {
        Move move = ai_choose(&ai_board, SIDE_WHITE, improved);
        assert(move.from.row == 7 && move.from.col == 3);
        assert(move.to.row == 6 && move.to.col == 3);
    }

    /* The winner menu can start a fresh game without closing the app. */
    Game finished;
    game_init(&finished, 777u, &settings);
    finished.two_player = 1;
    finished.screen = SCREEN_WINNER;
    finished.winner = SIDE_WHITE;
    game_mouse_down(&finished, 735.0f, 325.0f, 0);  /* PLAY AGAIN */
    assert(finished.screen == SCREEN_PLAY);
    assert(finished.winner == -1);
    assert(piece_count(&finished, SIDE_WHITE) == 8);
    assert(piece_count(&finished, SIDE_RED) == 8);

    finished.screen = SCREEN_WINNER;
    finished.winner = SIDE_RED;
    finished.winner_cursor = 1;
    game_key(&finished, 5);                         /* MAIN MENU */
    assert(finished.screen == SCREEN_MENU);

    puts("logic test: ok");
    return 0;
}
