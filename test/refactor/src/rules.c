#include "rules.h"

#include "board.h"

#include <string.h>

typedef struct { int dr; int dc; } Direction;

static const Direction DIRECTIONS[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

static int move_steps(const Board *board, int row, int col)
{
    const Piece *piece = &board->piece[row][col];
    int steps = board_value(board, row, col);
    if (piece->type == PIECE_SHOGUN && steps > 2) steps = 2;
    return steps;
}

/*
 * Walk one orthogonal leg.  A move must stop after exactly `steps` squares,
 * may turn at most once, cannot revisit a square, cannot jump a piece, and
 * may only capture on the final square.
 */
static void walk(const Board *board, Cell from, int row, int col,
                 Direction dir, int left, int turned,
                 uint8_t used[BOARD_SIZE][BOARD_SIZE],
                 Move *out, int *count, int max)
{
    int nr = row + dir.dr;
    int nc = col + dir.dc;
    if (!in_board(nr, nc) || used[nr][nc]) return;

    const Piece *target = &board->piece[nr][nc];
    Side mover = board->piece[from.row][from.col].side;
    if (target->type != PIECE_NONE) {
        if (target->side == mover || left != 1) return;
    }

    used[nr][nc] = 1;
    if (left == 1) {
        Move *move = &out[*count];
        move->from = from;
        move->to.row = nr;
        move->to.col = nc;
        move->capture = target->type != PIECE_NONE;
        ++*count;
    } else {
        walk(board, from, nr, nc, dir, left - 1, turned, used, out, count, max);
        if (!turned) {
            for (int i = 0; i < 4; ++i) {
                Direction next = DIRECTIONS[i];
                if (next.dr == -dir.dr && next.dc == -dir.dc) continue;
                if (next.dr == dir.dr && next.dc == dir.dc) continue;
                walk(board, from, nr, nc, next, left - 1, 1,
                     used, out, count, max);
            }
        }
    }
    used[nr][nc] = 0;
}

int rules_generate(const Board *board, int row, int col, Move *out, int max)
{
    if (!in_board(row, col) || max <= 0) return 0;
    const Piece *piece = &board->piece[row][col];
    if (piece->type == PIECE_NONE) return 0;

    uint8_t used[BOARD_SIZE][BOARD_SIZE];
    memset(used, 0, sizeof(used));
    used[row][col] = 1;

    Cell from = {row, col};
    int steps = move_steps(board, row, col);
    int count = 0;
    for (int i = 0; i < 4 && count < max; ++i) {
        walk(board, from, row, col, DIRECTIONS[i], steps, 0,
             used, out, &count, max);
    }
    return count;
}

int rules_all(const Board *board, Side side, Move *out, int max)
{
    int count = 0;
    for (int row = 0; row < BOARD_SIZE && count < max; ++row) {
        for (int col = 0; col < BOARD_SIZE && count < max; ++col) {
            const Piece *piece = &board->piece[row][col];
            if (piece->type == PIECE_NONE || piece->side != side) continue;
            Move local[MAX_MOVES];
            int n = rules_generate(board, row, col, local, MAX_MOVES);
            for (int i = 0; i < n && count < max; ++i) out[count++] = local[i];
        }
    }
    return count;
}

int rules_has_any(const Board *board, Side side)
{
    Move moves[64];
    return rules_all(board, side, moves, 64) > 0;
}

int rules_in_check(const Board *board, Side side)
{
    Move moves[MAX_MOVES];
    int count = rules_all(board, other_side(side), moves, MAX_MOVES);
    for (int i = 0; i < count; ++i) {
        const Piece *target = &board->piece[moves[i].to.row][moves[i].to.col];
        if (target->type == PIECE_SHOGUN && target->side == side) return 1;
    }
    return 0;
}

int rules_winner(const Board *board)
{
    if (!board_has_shogun(board, SIDE_WHITE)) return SIDE_RED;
    if (!board_has_shogun(board, SIDE_RED)) return SIDE_WHITE;
    if (board_count_pawns(board, SIDE_WHITE) <= 1) return SIDE_RED;
    if (board_count_pawns(board, SIDE_RED) <= 1) return SIDE_WHITE;
    return -1;
}
