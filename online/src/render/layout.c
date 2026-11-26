#include "render/layout.h"

#include "core/types.h"

int layout_width(int portrait) { return portrait ? PORTRAIT_W : LOGICAL_W; }
int layout_height(int portrait) { return portrait ? PORTRAIT_H : LOGICAL_H; }
float layout_cell(int portrait) { (void)portrait; return 60.0f; }

void layout_board(int portrait, Rect *out)
{
    /* The original keeps the same 60px cell size in both orientations.
       Landscape puts the board in the top-left with a 30px inset; portrait
       puts it at the top of the tall screen with no horizontal inset. */
    float cell = layout_cell(portrait);
    out->w = cell * BOARD_SIZE;
    out->h = cell * BOARD_SIZE;
    out->x = portrait ? 0.0f : 30.0f;
    out->y = portrait ? 160.0f : 30.0f;
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
    if (x < board.x || y < board.y || x >= board.x + board.w ||
        y >= board.y + board.h) {
        return 0;
    }
    float cell = layout_cell(portrait);
    *col = (int)((x - board.x) / cell);
    *row = (int)((y - board.y) / cell);
    return in_board(*row, *col);
}

/* Screen positions of each menu item. Landscape follows the two-column
   keyboard grid; portrait stacks the items in one column in a different order. */
static void menu_rect_landscape(int index, Rect *out)
{
    int col;
    int row;
    switch (index) {
    case 0: col = 0; row = 0; break; /* ONE PLAYER */
    case 1: col = 1; row = 0; break; /* TWO PLAYERS */
    case 2: col = 0; row = 1; break; /* GAME RULES */
    case 3: col = 1; row = 1; break; /* SHOW MOVE GUIDES */
    case 4: col = 0; row = 2; break; /* ALWAYS SHOW TERRAIN */
    default: col = 1; row = 2; break; /* COMPUTER OPPONENT */
    }
    out->x = 58.0f + (float)col * 440.0f;
    out->y = 180.0f + (float)row * 80.0f;
    out->w = 404.0f;
    out->h = 68.0f;
}

static void menu_rect_portrait(int index, Rect *out)
{
    /* Portrait order is: settings, rules, one player, two players. */
    static const float ys[MENU_ITEMS] = {484.0f, 596.0f, 382.0f,
                                         188.0f, 250.0f, 312.0f};
    static const float hs[MENU_ITEMS] = {94.0f, 94.0f, 84.0f,
                                         54.0f, 54.0f, 54.0f};
    out->x = 38.0f;
    out->y = ys[index];
    out->w = 404.0f;
    out->h = hs[index];
}

void layout_menu_item(int portrait, int index, Rect *out)
{
    if (index < 0 || index >= MENU_ITEMS) index = 0;
    if (portrait) menu_rect_portrait(index, out);
    else menu_rect_landscape(index, out);
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

void layout_pause_control(int portrait, Rect *out)
{
    if (portrait) *out = (Rect){422.0f, 4.0f, 54.0f, 48.0f};
    else *out = (Rect){558.0f, 476.0f, 354.0f, 44.0f};
}

void layout_pause_button(int portrait, int index, Rect *outer, Rect *inner)
{
    (void)portrait;
    /* index 0 = CONTINUE, 1 = MAIN MENU. */
    if (index == 0) {
        *outer = (Rect){88.0f, 228.0f, 364.0f, 54.0f};
        *inner = (Rect){90.0f, 230.0f, 360.0f, 50.0f};
    } else {
        *outer = (Rect){88.0f, 294.0f, 364.0f, 54.0f};
        *inner = (Rect){90.0f, 296.0f, 360.0f, 50.0f};
    }
}

void layout_winner_button(int portrait, int index, Rect *outer, Rect *inner)
{
    if (index == 0) {
        if (portrait) {
            *outer = (Rect){38.0f, 632.0f, 404.0f, 58.0f};
            *inner = (Rect){40.0f, 634.0f, 400.0f, 54.0f};
        } else {
            *outer = (Rect){558.0f, 300.0f, 354.0f, 50.0f};
            *inner = (Rect){560.0f, 302.0f, 350.0f, 46.0f};
        }
    } else {
        if (portrait) {
            *outer = (Rect){38.0f, 700.0f, 404.0f, 58.0f};
            *inner = (Rect){40.0f, 702.0f, 400.0f, 54.0f};
        } else {
            *outer = (Rect){558.0f, 360.0f, 354.0f, 50.0f};
            *inner = (Rect){560.0f, 362.0f, 350.0f, 46.0f};
        }
    }
}

int layout_winner_button_at(int portrait, float x, float y)
{
    Rect outer;
    Rect inner;
    for (int i = 0; i < 2; ++i) {
        layout_winner_button(portrait, i, &outer, &inner);
        if (x >= outer.x && x <= outer.x + outer.w &&
            y >= outer.y && y <= outer.y + outer.h) return i;
    }
    return -1;
}

void layout_mode_button(int portrait, int index, Rect *outer, Rect *inner)
{
    if (index < 0) index = 0;
    if (index > 2) index = 2;
    if (portrait) {
        float ys[3] = {560.0f, 630.0f, 700.0f};
        *outer = (Rect){38.0f, ys[index], 404.0f, 58.0f};
        *inner = (Rect){40.0f, ys[index] + 2.0f, 400.0f, 54.0f};
    } else {
        float ys[3] = {210.0f, 280.0f, 350.0f};
        *outer = (Rect){250.0f, ys[index], 460.0f, 56.0f};
        *inner = (Rect){252.0f, ys[index] + 2.0f, 456.0f, 52.0f};
    }
}

int layout_mode_button_at(int portrait, float x, float y)
{
    Rect outer;
    Rect inner;
    for (int i = 0; i < 3; ++i) {
        layout_mode_button(portrait, i, &outer, &inner);
        if (x >= outer.x && x <= outer.x + outer.w &&
            y >= outer.y && y <= outer.y + outer.h) return i;
    }
    return -1;
}

void layout_name_box(int portrait, Rect *out)
{
    if (portrait) *out = (Rect){38.0f, 300.0f, 404.0f, 90.0f};
    else *out = (Rect){180.0f, 220.0f, 600.0f, 80.0f};
}

void layout_setup_field(int portrait, int index, Rect *out)
{
    if (index < 0) index = 0;
    if (index > 1) index = 1;
    if (portrait) {
        float ys[2] = {230.0f, 350.0f};
        *out = (Rect){38.0f, ys[index], 404.0f, 80.0f};
    } else {
        float ys[2] = {150.0f, 260.0f};
        *out = (Rect){180.0f, ys[index], 600.0f, 70.0f};
    }
}

void layout_setup_button(int portrait, int index, Rect *outer, Rect *inner)
{
    if (index < 0) index = 0;
    if (index > 1) index = 1;
    if (portrait) {
        float ys[2] = {500.0f, 570.0f};
        *outer = (Rect){38.0f, ys[index], 404.0f, 58.0f};
        *inner = (Rect){40.0f, ys[index] + 2.0f, 400.0f, 54.0f};
    } else {
        float ys[2] = {350.0f, 420.0f};
        *outer = (Rect){250.0f, ys[index], 460.0f, 56.0f};
        *inner = (Rect){252.0f, ys[index] + 2.0f, 456.0f, 52.0f};
    }
}

int layout_setup_item_at(int portrait, float x, float y)
{
    for (int i = 0; i < 2; ++i) {
        Rect field;
        layout_setup_field(portrait, i, &field);
        if (x >= field.x && x <= field.x + field.w &&
            y >= field.y && y <= field.y + field.h) return i;
    }
    for (int i = 0; i < 2; ++i) {
        Rect outer;
        Rect inner;
        layout_setup_button(portrait, i, &outer, &inner);
        if (x >= outer.x && x <= outer.x + outer.w &&
            y >= outer.y && y <= outer.y + outer.h) return i + 2;
    }
    return -1;
}
