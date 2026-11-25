#ifndef SHOGUN_TYPES_H
#define SHOGUN_TYPES_H

#include <stdint.h>

#define BOARD_SIZE 8
#define MAX_MOVES 512

#define LOGICAL_W 960
#define LOGICAL_H 540
#define PORTRAIT_W 480
#define PORTRAIT_H 800

typedef enum { SIDE_WHITE = 0, SIDE_RED = 1 } Side;
typedef enum { PIECE_NONE = 0, PIECE_PAWN = 1, PIECE_SHOGUN = 2 } PieceType;

typedef struct {
    PieceType type;
    Side side;
    int value;              /* squares this piece may move right now */
} Piece;

typedef struct {
    uint8_t value;          /* hidden terrain number, 1..4 */
    uint8_t revealed;       /* painted once the square is left */
} Terrain;

typedef struct {
    Piece piece[BOARD_SIZE][BOARD_SIZE];
    Terrain terrain[BOARD_SIZE][BOARD_SIZE];
    uint32_t rng_state;             /* next value after the terrain shuffle */
} Board;

typedef struct {
    int show_moves;
    int show_terrain;
    int improved_ai;
} Settings;

typedef struct {
    int row;
    int col;
} Cell;

typedef struct {
    Cell from;
    Cell to;
    int capture;
} Move;

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_MENU,
    SCREEN_PLAY,
    SCREEN_PAUSE,
    SCREEN_HELP,
    SCREEN_WINNER
} Screen;

static inline Side other_side(Side s) { return s == SIDE_WHITE ? SIDE_RED : SIDE_WHITE; }
static inline int in_board(int row, int col)
{
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

#endif
