#ifndef SHOGUN_GAME_H
#define SHOGUN_GAME_H

#include "core/types.h"

typedef struct {
    Board board;
    uint32_t seed;
    Settings settings;
    Screen screen;
    Screen back;
    Side turn;
    int two_player;
    Cell cursor;
    Cell selected;
    int has_selection;
    Move moves[MAX_MOVES];
    int move_count;
    int winner;
    int check_timer;
    Cell check_cell;
    int win_timer;
    Cell guide_from;
    Cell guide_to;
    int has_guide;
    int thinking;
    int menu_cursor;
    int winner_cursor;
    int help_page;
    int hover_item;
    Cell hover_cell;
    int has_hover;
    int tick;
    int mode_cursor;
    OnlinePhase online_phase;
    char name[ONLINE_NAME_SIZE];
    char opponent[ONLINE_OPPONENT_SIZE];
    char server[128];
    char server_entry[128];
    char token[ONLINE_TOKEN_SIZE];
    int setup_cursor;
    int setup_typing;
    char online_message[ONLINE_MESSAGE_SIZE];
    int local_side;
    int match_id;
    int challenge_cursor;
    int has_pending_move;
    Move pending_move;
    int pending_connect;
    int pending_accept;
    int pending_decline;
    int pending_rematch;
    int pending_leave;
} Game;

void game_init(Game *game, uint32_t seed, const Settings *settings);
void game_update(Game *game);
void game_key(Game *game, int code);
void game_mouse_move(Game *game, float x, float y, int portrait);
void game_mouse_down(Game *game, float x, float y, int portrait);
int game_hovering_button(const Game *game);
void game_name_input(Game *game, const char *text);
void game_name_backspace(Game *game);
void game_setup_text(Game *game, const char *text);
void game_setup_backspace(Game *game);
void game_start_online(Game *game, uint32_t seed, Side side);
int game_apply_remote_move(Game *game, const Move *move);

#endif
