#include "core/board.h"

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
    /* The original starts with two xorshift steps, then shuffles a bag of
       sixteen each of terrain values 1..4. Shoguns may start on any value;
       their movement is capped to two squares when moves are generated. */
    uint32_t rng = seed ? seed : 0x9e3779b9u;
    next_random(&rng);
    next_random(&rng);
    next_random(&rng);

    int bag[BOARD_SIZE * BOARD_SIZE];
    for (int i = 0; i < 16; ++i) bag[i] = 1;
    for (int i = 16; i < 32; ++i) bag[i] = 2;
    for (int i = 32; i < 48; ++i) bag[i] = 3;
    for (int i = 48; i < 64; ++i) bag[i] = 4;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            board->piece[row][col].type = PIECE_NONE;
            board->piece[row][col].side = SIDE_WHITE;
            board->piece[row][col].value = 0;
            board->terrain[row][col].value = 0;
        }
    }

    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
        uint32_t state = next_random(&rng);
        float fraction = (float)(state >> 8) * (1.0f / 16777216.0f);
        int remaining = BOARD_SIZE * BOARD_SIZE - i;
        int index = (int)(fraction * (float)remaining);
        if (index >= remaining) index = remaining - 1;

        board->terrain[i / BOARD_SIZE][i % BOARD_SIZE].value =
            (uint8_t)bag[index];

        --remaining;
        if (index >= remaining) continue;

        uint32_t offset = 3u + 3u * (uint32_t)i;
        int sum = i + index;
        int back = (3 - sum) & 3;
        int limit = back ? index + back : index;
        int shift = (int)((offset - (uint32_t)index) & 3u);

        /* Compact the bag with the original's two overlapping shifts. */
        for (int k = 0; k < shift; ++k) {
            bag[index + k] = bag[index + k + 1];
        }
        if ((uint32_t)(sum - 60) < 3u) continue;
        int pos = index + shift;
        while (limit < remaining) {
            for (int k = 0; k < 4; ++k) {
                bag[pos + k] = bag[pos + k + 1];
            }
            pos += 4;
            limit += 4;
        }
    }

    /* The original game puts red on the first rank and white on the last.
       Each shogun starts near the middle of its rank, not at the edge. */
    const int red_king_col = 3;
    const int white_king_col = 4;
    for (int col = 0; col < BOARD_SIZE; ++col) {
        Piece *red = &board->piece[0][col];
        red->type = (col == red_king_col) ? PIECE_SHOGUN : PIECE_PAWN;
        red->side = SIDE_RED;
        red->value = board_value(board, 0, col);

        Piece *white = &board->piece[BOARD_SIZE - 1][col];
        white->type = (col == white_king_col) ? PIECE_SHOGUN : PIECE_PAWN;
        white->side = SIDE_WHITE;
        white->value = board_value(board, BOARD_SIZE - 1, col);
    }

    board->rng_state = rng;
}

Side board_starting_side(const Board *board)
{
    /* The original advances the terrain RNG once more and uses that value to
       choose who moves first. Its turn values are 1 (white) and 2 (red). */
    uint32_t state = board->rng_state;
    uint32_t value = next_random(&state);
    float scaled = (float)(value >> 8) * (1.0f / 16777216.0f);
    int picked = (int)(scaled * 1.99f + 1.0f);
    if (picked < 1) picked = 1;
    if (picked > 2) picked = 2;
    return picked == 1 ? SIDE_RED : SIDE_WHITE;
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
}

void board_apply(Board *board, const Move *move)
{
    Piece moving = board->piece[move->from.row][move->from.col];
    board_vacate(board, move->from.row, move->from.col);
    moving.value = board_value(board, move->to.row, move->to.col);
    board->piece[move->to.row][move->to.col] = moving;
}
