#include "ai.h"

#include "board.h"
#include "rules.h"

#include <stdlib.h>

#define INF 1000000
#define WIN_SCORE 100000
#define CLASSIC_DEPTH 2
#define CLASSIC_BRANCH 12
#define IMPROVED_DEPTH 3
#define IMPROVED_BRANCH 24

static int piece_score(PieceType type, int value)
{
    if (type == PIECE_SHOGUN) return 5000;
    if (type == PIECE_PAWN) return 100 + value * 20;
    return 0;
}

static int advance_score(Side side, int row)
{
    return side == SIDE_WHITE ? (BOARD_SIZE - 2 - row) * 4 : (row - 1) * 4;
}

static int evaluate(const Board *board, Side side)
{
    Side enemy = other_side(side);
    int score = 0;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece *piece = &board->piece[row][col];
            if (piece->type == PIECE_NONE) continue;

            int value = piece_score(piece->type, piece->value);
            if (piece->type == PIECE_PAWN) {
                value += advance_score(piece->side, row);
            }
            score += piece->side == side ? value : -value;
        }
    }

    if (rules_in_check(board, enemy)) score += 120;
    if (rules_in_check(board, side)) score -= 200;

    Move moves[MAX_MOVES];
    int count = rules_all(board, enemy, moves, MAX_MOVES);
    for (int i = 0; i < count; ++i) {
        const Piece *target = &board->piece[moves[i].to.row][moves[i].to.col];
        if (target->type == PIECE_SHOGUN && target->side == side) {
            score -= 300;
        }
    }

    count = rules_all(board, side, moves, MAX_MOVES);
    for (int i = 0; i < count; ++i) {
        const Piece *target = &board->piece[moves[i].to.row][moves[i].to.col];
        if (target->type == PIECE_SHOGUN && target->side == enemy) {
            score += 300;
        }
    }
    return score;
}

static int move_order_score(const Board *board, const Move *move, Side side)
{
    const Piece *target = &board->piece[move->to.row][move->to.col];
    int score = piece_score(target->type, target->value) * 4;
    score += advance_score(side, move->to.row);
    return score;
}

static void order_moves(const Board *board, Move *moves, int count, Side side)
{
    /* Cheap insertion sort: the move lists are short and this keeps the best
       captures and advances first for alpha-beta pruning. */
    for (int i = 1; i < count; ++i) {
        Move current = moves[i];
        int score = move_order_score(board, &current, side);
        int j = i - 1;
        while (j >= 0 && move_order_score(board, &moves[j], side) < score) {
            moves[j + 1] = moves[j];
            --j;
        }
        moves[j + 1] = current;
    }
}

static int search(const Board *board, Side side, int depth, int branch,
                  int alpha, int beta)
{
    int winner = rules_winner(board);
    if (winner >= 0) {
        return winner == (int)side ? WIN_SCORE + depth : -WIN_SCORE - depth;
    }
    if (depth == 0) return evaluate(board, side);

    Move moves[MAX_MOVES];
    int count = rules_all(board, side, moves, MAX_MOVES);
    if (count == 0) return -WIN_SCORE - depth;
    order_moves(board, moves, count, side);
    if (count > branch) count = branch;

    int best = -INF;
    for (int i = 0; i < count; ++i) {
        Board copy = *board;
        board_apply(&copy, &moves[i]);
        int score = -search(&copy, other_side(side), depth - 1, branch,
                            -beta, -alpha);
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

Move ai_choose(const Board *board, Side side, int improved)
{
    Move moves[MAX_MOVES];
    Move best = {{0, 0}, {0, 0}, 0};
    int count = rules_all(board, side, moves, MAX_MOVES);
    if (count == 0) return best;

    order_moves(board, moves, count, side);

    int depth = improved ? IMPROVED_DEPTH : CLASSIC_DEPTH;
    int branch = improved ? IMPROVED_BRANCH : CLASSIC_BRANCH;
    int limit = improved ? count : (count < branch ? count : branch);
    int noise = improved ? 3 : 9;
    int best_score = -INF;

    for (int i = 0; i < limit; ++i) {
        Board copy = *board;
        board_apply(&copy, &moves[i]);
        int score = -search(&copy, other_side(side), depth - 1, branch,
                            -INF, INF);
        score += rand() % noise;
        if (score > best_score) {
            best_score = score;
            best = moves[i];
        }
    }
    return best;
}
