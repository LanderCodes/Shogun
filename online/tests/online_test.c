/* Headless checks for the online protocol and a loopback matchmaking game. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/game.h"
#include "core/rules.h"
#include "client.h"
#include "protocol.h"
#include "server.h"
#include "render/layout.h"

#ifndef _WIN32
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_PORT 7799

static pid_t server_pid = -1;

static void stop_server(void)
{
    if (server_pid > 0) {
        kill(server_pid, SIGTERM);
        waitpid(server_pid, NULL, 0);
        server_pid = -1;
    }
}

static void start_server(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        (void)online_server_run(TEST_PORT);
        _exit(0);
    }
    assert(pid > 0);
    server_pid = pid;
    atexit(stop_server);
    usleep(200000);
}

static void pump(OnlineNet *a, OnlineNet *b, Game *ga, Game *gb, int iterations)
{
    for (int i = 0; i < iterations; ++i) {
        online_net_update(a, ga);
        online_net_update(b, gb);
        usleep(5000);
    }
}
#endif

static void check_protocol(void)
{
    char name[ONLINE_NAME_SIZE];
    online_sanitize_name("  ada lovelace!  ", name, sizeof(name));
    assert(strcmp(name, "ADA LOVELACE") == 0);
    online_sanitize_name("!!", name, sizeof(name));
    assert(name[0] == '\0');

    int match_id = 0;
    int side = -1;
    int from = -1;
    int to = -1;
    int accepted = -1;
    uint32_t seed = 0;
    char text[64];

    assert(online_parse_start("START 7 12345 1", &match_id, &seed, &side));
    assert(match_id == 7 && seed == 12345u && side == 1);
    assert(online_parse_challenge("CHALLENGE 3 BOB THE BUILDER", &match_id,
                                  text, sizeof(text)));
    assert(match_id == 3 && strcmp(text, "BOB THE BUILDER") == 0);
    assert(online_parse_move("MOVE 3 60 52", &match_id, &from, &to));
    assert(match_id == 3 && from == 60 && to == 52);
    assert(online_parse_rematch("REMATCH 3 YES", &match_id, &accepted));
    assert(match_id == 3 && accepted == 1);
    assert(!online_parse_move("MOVE 3 99 52", &match_id, &from, &to));

    char hash[ONLINE_HASH_HEX_SIZE];
    char parsed[ONLINE_HASH_HEX_SIZE];
    assert(online_hash_hex(online_hash("0123456789abcdef", "test-token"),
                           hash, sizeof(hash)));
    assert(strlen(hash) == ONLINE_HASH_HEX_SIZE - 1);
    assert(online_parse_nonce("NONCE 0123456789abcdef", parsed, sizeof(parsed)));
    assert(strcmp(parsed, "0123456789abcdef") == 0);
    snprintf(text, sizeof(text), "AUTH %s", hash);
    assert(online_parse_auth(text, parsed, sizeof(parsed)));
    assert(strcmp(parsed, hash) == 0);
    assert(!online_parse_auth("AUTH nothex", parsed, sizeof(parsed)));
    assert(!online_parse_auth(text + 1, parsed, sizeof(parsed)));
}

#ifndef _WIN32
static void check_loopback(void)
{
    Settings settings = {0, 0, 0};
    Game ga;
    Game gb;
    game_init(&ga, 1u, &settings);
    game_init(&gb, 2u, &settings);

    OnlineNet a;
    OnlineNet b;
    online_net_init(&a);
    online_net_init(&b);

    assert(online_net_connect_addr(&a, "127.0.0.1", TEST_PORT, "ALICE",
                                   "test-token"));
    assert(online_net_connect_addr(&b, "127.0.0.1", TEST_PORT, "BOB",
                                   "test-token"));
    pump(&a, &b, &ga, &gb, 200);
    assert(a.state == NET_CHALLENGE && b.state == NET_CHALLENGE);
    assert(strcmp(ga.opponent, "BOB") == 0);
    assert(strcmp(gb.opponent, "ALICE") == 0);

    online_net_send_decline(&a, ga.match_id);
    pump(&a, &b, &ga, &gb, 100);
    assert(ga.screen == SCREEN_ONLINE_WAIT);
    assert(gb.screen == SCREEN_ONLINE_WAIT);
    online_net_close(&a);
    online_net_close(&b);
    usleep(100000);

    Game bad_game;
    OnlineNet bad;
    game_init(&bad_game, 3u, &settings);
    online_net_init(&bad);
    assert(!online_net_connect_addr(&bad, "127.0.0.1", TEST_PORT, "EVE", NULL));
    assert(online_net_connect_addr(&bad, "127.0.0.1", TEST_PORT, "EVE",
                                   "wrong-token"));
    for (int i = 0; i < 100; ++i) {
        online_net_update(&bad, &bad_game);
        usleep(5000);
    }
    assert(bad.state != NET_CHALLENGE && bad.state != NET_PLAYING);
    online_net_close(&bad);
    usleep(100000);
    assert(setenv("SHOGUN_TOKEN", "test-token", 1) == 0);

    game_init(&ga, 1u, &settings);
    game_init(&gb, 2u, &settings);
    online_net_init(&a);
    online_net_init(&b);
    assert(online_net_connect_addr(&a, "127.0.0.1", TEST_PORT, "ALICE",
                                   "test-token"));
    assert(online_net_connect_addr(&b, "127.0.0.1", TEST_PORT, "BOB",
                                   "test-token"));
    pump(&a, &b, &ga, &gb, 200);
    assert(a.state == NET_CHALLENGE && b.state == NET_CHALLENGE);

    online_net_send_accept(&a, ga.match_id);
    online_net_send_accept(&b, gb.match_id);
    pump(&a, &b, &ga, &gb, 200);
    assert(ga.screen == SCREEN_PLAY && gb.screen == SCREEN_PLAY);
    assert(ga.local_side == 0 || ga.local_side == 1);
    assert(ga.local_side != gb.local_side);
    assert(ga.seed == gb.seed);

    Game *mover = ga.turn == (Side)ga.local_side ? &ga : &gb;
    OnlineNet *mover_net = mover == &ga ? &a : &b;
    Game *watcher = mover == &ga ? &gb : &ga;
    OnlineNet *watcher_net = mover == &ga ? &b : &a;

    Move moves[MAX_MOVES];
    int count = rules_all(&mover->board, mover->turn, moves, MAX_MOVES);
    assert(count > 0);
    Move move = moves[0];
    Piece moved = mover->board.piece[move.from.row][move.from.col];
    Side before = mover->turn;

    float x;
    float y;
    layout_cell_center(0, move.from.row, move.from.col, &x, &y);
    game_mouse_down(mover, x, y, 0);
    layout_cell_center(0, move.to.row, move.to.col, &x, &y);
    game_mouse_down(mover, x, y, 0);
    assert(mover->has_pending_move);

    online_net_send_move(mover_net, mover->match_id, &mover->pending_move);
    mover->has_pending_move = 0;
    for (int i = 0; i < 200 && !watcher->has_guide; ++i) {
        online_net_update(watcher_net, watcher);
        usleep(5000);
    }
    assert(watcher->has_guide);
    assert(watcher->guide_to.row == move.to.row &&
           watcher->guide_to.col == move.to.col);
    assert(watcher->board.piece[move.to.row][move.to.col].type == moved.type);
    assert(watcher->board.piece[move.to.row][move.to.col].side == moved.side);
    assert(watcher->turn != before);
    assert(!watcher->has_pending_move);

    pump(&a, &b, &ga, &gb, 100);
    assert(mover->online_phase == ONLINE_PLAYING);
    assert(mover->screen == SCREEN_PLAY);
    assert(strstr(mover->online_message, "INVALID") == NULL);
    assert(strstr(watcher->online_message, "OPPONENT LEFT") == NULL);

    online_net_send_leave(&a, ga.match_id);
    online_net_send_leave(&b, gb.match_id);
    online_net_close(&a);
    online_net_close(&b);
}
#endif

int main(void)
{
    check_protocol();

#ifndef _WIN32
    assert(setenv("SHOGUN_TOKEN", "test-token", 1) == 0);
    start_server();
    check_loopback();
    stop_server();
#endif

    puts("online test: ok");
    return 0;
}
