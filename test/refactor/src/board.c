#include "board.h"

static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

void board_init(Board *board, uint32_t seed)
{
    uint32_t rng = seed ? seed : 0x9e3779b9u;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            board->piece[row][col].type = PIECE_NONE;
            board->piece[row][col].side = SIDE_WHITE;
            board->piece[row][col].value = 0;

            /* Pawns live on 1..4 terrain, shoguns only start on 1..2. */
            int value = 1 + (int)(next_random(&rng) % 4);
            if ((row == 6 && col == BOARD_SIZE - 1) ||
                (row == 1 && col == BOARD_SIZE - 1)) {
                value = 1 + (int)(next_random(&rng) % 2);
            }
            board->terrain[row][col].value = (uint8_t)value;
            board->terrain[row][col].revealed = 0;
        }
    }

    /* White plays up the board, red plays down it. */
    for (int col = 0; col < BOARD_SIZE; ++col) {
        Piece *white = &board->piece[6][col];
        white->type = (col == BOARD_SIZE - 1) ? PIECE_SHOGUN : PIECE_PAWN;
        white->side = SIDE_WHITE;
        white->value = board_value(board, 6, col);

        Piece *red = &board->piece[1][col];
        red->type = (col == BOARD_SIZE - 1) ? PIECE_SHOGUN : PIECE_PAWN;
        red->side = SIDE_RED;
        red->value = board_value(board, 1, col);
    }
}

int board_value(const Board *board, int row, int col)
{
    if (!in_board(row, col)) return 0;
    int value = board->terrain[row][col].value;
    if (value < 1) value = 1;
    if (value > 4) value = 4;
    return value;
}

int board_count_pawns(const Board *board, Side side)
{
    int count = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece *p = &board->piece[row][col];
            if (p->type == PIECE_PAWN && p->side == side) ++count;
        }
    }
    return count;
}

int board_has_shogun(const Board *board, Side side)
{
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece *p = &board->piece[row][col];
            if (p->type == PIECE_SHOGUN && p->side == side) return 1;
        }
    }
    return 0;
}

void board_vacate(Board *board, int row, int col)
{
    board->piece[row][col].type = PIECE_NONE;
    board->piece[row][col].value = 0;
    board->terrain[row][col].revealed = 1;
}

void board_apply(Board *board, const Move *move)
{
    Piece moving = board->piece[move->from.row][move->from.col];
    board_vacate(board, move->from.row, move->from.col);
    moving.value = board_value(board, move->to.row, move->to.col);
    if (moving.type == PIECE_SHOGUN && moving.value > 2) moving.value = 2;
    board->piece[move->to.row][move->to.col] = moving;
    board->terrain[move->to.row][move->to.col].revealed = 1;
}
