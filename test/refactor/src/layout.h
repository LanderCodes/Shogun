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
void layout_menu_item(int portrait, int index, Rect *out);
int layout_menu_item_at(int portrait, float x, float y);
void layout_panel(int portrait, Rect *out);

#endif
