#ifndef SHOGUN_LAYOUT_H
#define SHOGUN_LAYOUT_H

#define MENU_ITEMS 6

typedef struct { float x, y, w, h; } Rect;

int layout_width(int portrait);
int layout_height(int portrait);
float layout_cell(int portrait);
void layout_board(int portrait, Rect *out);
void layout_cell_center(int portrait, int row, int col, float *x, float *y);
int layout_cell_at(int portrait, float x, float y, int *row, int *col);

/* Menu items in canonical order:
   0 ONE PLAYER, 1 TWO PLAYERS, 2 GAME RULES,
   3 SHOW MOVE GUIDES, 4 ALWAYS SHOW TERRAIN, 5 COMPUTER OPPONENT. */
void layout_menu_item(int portrait, int index, Rect *out);
int layout_menu_item_at(int portrait, float x, float y);

/* Coordinates used by the in-game pause button and pause overlay buttons. */
void layout_pause_control(int portrait, Rect *out);
void layout_pause_button(int portrait, int index, Rect *outer, Rect *inner);

/* Winner menu buttons: 0 = PLAY AGAIN, 1 = MAIN MENU. */
void layout_winner_button(int portrait, int index, Rect *outer, Rect *inner);
int layout_winner_button_at(int portrait, float x, float y);

#endif
