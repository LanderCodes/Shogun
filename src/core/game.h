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
} Game;

void game_init(Game *game, uint32_t seed, const Settings *settings);
void game_update(Game *game);
void game_key(Game *game, int code);
void game_mouse_move(Game *game, float x, float y, int portrait);
void game_mouse_down(Game *game, float x, float y, int portrait);
int game_hovering_button(const Game *game);

#endif
