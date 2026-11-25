#include "layout.h"

#include "types.h"

int layout_width(int portrait) { return portrait ? PORTRAIT_W : LOGICAL_W; }
int layout_height(int portrait) { return portrait ? PORTRAIT_H : LOGICAL_H; }
float layout_cell(int portrait) { return portrait ? 54.0f : 60.0f; }

void layout_board(int portrait, Rect *out)
{
    float cell = layout_cell(portrait);
    float size = cell * BOARD_SIZE;
    out->w = size;
    out->h = size;
    out->x = portrait ? (PORTRAIT_W - size) / 2.0f : 30.0f;
    out->y = portrait ? 170.0f : 30.0f;
}

void layout_cell_center(int portrait, int row, int col, float *x, float *y)
{
    Rect board;
    layout_board(portrait, &board);
    float cell = layout_cell(portrait);
    *x = board.x + cell * ((float)col + 0.5f);
    *y = board.y + cell * ((float)row + 0.5f);
}

int layout_cell_at(int portrait, float x, float y, int *row, int *col)
{
    Rect board;
    layout_board(portrait, &board);
    if (x < board.x || y < board.y || x >= board.x + board.w || y >= board.y + board.h) {
        return 0;
    }
    float cell = layout_cell(portrait);
    *col = (int)((x - board.x) / cell);
    *row = (int)((y - board.y) / cell);
    return in_board(*row, *col);
}

void layout_menu_item(int portrait, int index, Rect *out)
{
    if (index < 0 || index >= MENU_ITEMS) index = 0;
    int col = index % 2;
    int row = index / 2;

    if (portrait) {
        out->w = 208.0f;
        out->h = 58.0f;
        out->x = 20.0f + (float)col * 232.0f;
        out->y = 210.0f + (float)row * 96.0f;
    } else {
        out->w = 404.0f;
        out->h = 68.0f;
        out->x = 58.0f + (float)col * 440.0f;
        out->y = 180.0f + (float)row * 80.0f;
    }
}

int layout_menu_item_at(int portrait, float x, float y)
{
    Rect rect;
    for (int i = 0; i < MENU_ITEMS; ++i) {
        layout_menu_item(portrait, i, &rect);
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h) return i;
    }
    return -1;
}

void layout_panel(int portrait, Rect *out)
{
    if (portrait) {
        out->x = 20.0f;
        out->y = 640.0f;
        out->w = PORTRAIT_W - 40.0f;
        out->h = 120.0f;
    } else {
        out->x = 540.0f;
        out->y = 30.0f;
        out->w = 390.0f;
        out->h = 480.0f;
    }
}
