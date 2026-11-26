/* Headless checks for the pure-C game logic. No SDL, no config file written. */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "core/ai.h"
#include "core/game.h"
#include "core/rules.h"
#include "render/glyph.h"
#include "render/layout.h"
#include "ui/menu.h"

static Piece make_piece(PieceType type, Side side, int value)
{
    Piece piece;
    piece.type = type;
    piece.side = side;
    piece.value = value;
    return piece;
}

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

static void check_text(const char *text)
{
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p == ' ') continue;
        int frame = glyph_frame(*p);
        assert(frame > 0 && frame < GLYPH_FRAMES);
    }
}

static void check_online_setup(void)
{
    Settings settings = {0, 0, 0};
    Game game;
    game_init(&game, 1u, &settings);
    game.screen = SCREEN_ONLINE_SETUP;

    game_key(&game, 5);
    assert(game.setup_typing);
    game_setup_text(&game, "abc123");
    assert(strcmp(game.token, "abc123") == 0);
    game_setup_backspace(&game);
    assert(strcmp(game.token, "abc12") == 0);
    game_key(&game, 5);
    assert(!game.setup_typing);

    game_key(&game, 2);
    assert(game.setup_cursor == 1);
    game_key(&game, 5);
    assert(game.screen == SCREEN_ONLINE_NAME);
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

    /* Two players now offers a local game or an online game. */
    Game local;
    game_init(&local, 42u, &settings);
    game_key(&local, 4);                /* TWO PLAYERS */
    game_key(&local, 5);
    assert(local.screen == SCREEN_PLAY_MODE);
    game_key(&local, 5);                /* LOCAL GAME */
    assert(local.screen == SCREEN_PLAY);
    assert(local.two_player && local.local_side < 0);

    Game online;
    game_init(&online, 12345u, &settings);
    online.name[0] = '\0';
    game_name_input(&online, " ada ");
    assert(strcmp(online.name, "ADA ") == 0);
    game_name_backspace(&online);
    assert(strcmp(online.name, "ADA") == 0);
    game_start_online(&online, 12345u, SIDE_WHITE);
    assert(online.screen == SCREEN_PLAY);
    assert(online.local_side == SIDE_WHITE);
    assert(online.online_phase == ONLINE_PLAYING);

    Move incoming[MAX_MOVES];
    int incoming_count = rules_all(&online.board, online.turn,
                                   incoming, MAX_MOVES);
    assert(incoming_count > 0);
    assert(game_apply_remote_move(&online, &incoming[0]));
    assert(online.turn == SIDE_WHITE);

    game_key(&game, 2);                 /* down to row 1 */
    game_key(&game, 4);                 /* right to column 1 */
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
    ai_board.piece[7][4] = make_piece(PIECE_SHOGUN, SIDE_WHITE, 1);
    ai_board.piece[7][3] = make_piece(PIECE_PAWN, SIDE_WHITE, 1);
    ai_board.piece[7][2] = make_piece(PIECE_PAWN, SIDE_WHITE, 1);
    ai_board.piece[6][3] = make_piece(PIECE_SHOGUN, SIDE_RED, 1);
    ai_board.piece[0][3] = make_piece(PIECE_PAWN, SIDE_RED, 1);
    ai_board.piece[0][4] = make_piece(PIECE_PAWN, SIDE_RED, 1);

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

    /* Every character used by the UI must resolve to a real, non-blank glyph. */
    assert(glyph_frame(' ') == 0);
    assert(glyph_frame('!') == 1);
    assert(glyph_frame('"') == 2);
    assert(glyph_frame('A') == 33);
    assert(glyph_frame('a') == 65);
    assert(glyph_frame('_') == 63);
    assert(glyph_frame('`') == 7);
    assert(glyph_frame((unsigned char)0x80) == 31);
    for (int i = 0; i < MENU_ITEMS; ++i) {
        char status[32];
        check_text(menu_name(i));
        check_text(menu_name_portrait(i));
        menu_status(i, &settings, status, sizeof(status));
        check_text(status);
    }
    check_text("Made by LanderCodes");
    check_text("MOUSE, KEYBOARD OR CONTROLLER");
    check_text("PAUSE / MENU  (ESC)");
    check_text("*PAUSED*");
    check_text("CONTINUE");
    check_text("MAIN MENU");
    check_text("ESC AGAIN FOR MENU");
    check_text("RED TO MOVE");
    check_text("WHITE TO MOVE");
    check_text("RED PLAYER");
    check_text("WHITE - COMPUTER");
    check_text("THINKING...");
    check_text("PLAY AGAIN");
    check_text("ENTER: SELECT    UP/DOWN: CHOOSE");
    check_text("TWO PLAYERS");
    check_text("LOCAL GAME");
    check_text("ONLINE GAME");
    check_text("ONLINE SETUP");
    check_text("ACCESS TOKEN");
    check_text("SERVER");
    check_text("NEXT");
    check_text("SELECT A FIELD, TYPE, THEN PRESS NEXT");
    check_text("ENTER YOUR NAME");
    check_text("TYPE A NAME - ENTER: CONNECT - ESC: BACK");
    check_text("BACK");
    check_text("NAME: PLAYER");
    check_text("WAITING FOR AN OPPONENT...");
    check_text("SERVER: AUTO");
    check_text("OPPONENT: BOB");
    check_text("MATCH FOUND");
    check_text("WANTS TO PLAY");
    check_text("ACCEPT");
    check_text("DECLINE");
    check_text("RETRY");
    check_text("(RED)");
    check_text("(WHITE)");
    check_text("ABOUT THE GAME");
    check_text("PAGE 1/3 - CLICK OR ENTER: NEXT    ESC: MENU");
    check_text("a 'check' warning");
    check_text("*THANK YOU*");

    check_online_setup();

    puts("logic test: ok");
    return 0;
}
