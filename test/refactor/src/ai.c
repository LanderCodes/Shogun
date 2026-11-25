#include "ai.h"

#include "board.h"
#include "rules.h"

#include <stdlib.h>

static int score_move(const Board *board, const Move *move, Side side, int improved)
{
    const Piece *target = &board->piece[move->to.row][move->to.col];
    int score = 0;

    if (target->type == PIECE_SHOGUN) score += 1000;
    else if (target->type == PIECE_PAWN) score += 30;

    /* Advance towards the other side's home rank. */
    score += side == SIDE_WHITE ? (6 - move->to.row) : (move->to.row - 1);

    if (improved) {
        Board copy = *board;
        board_apply(&copy, move);
        if (rules_in_check(&copy, side)) score -= 250;
        if (rules_in_check(&copy, other_side(side))) score += 60;
    }
    return score;
}

Move ai_choose(const Board *board, Side side, int improved)
{
    Move moves[MAX_MOVES];
    Move best = {{0, 0}, {0, 0}, 0};
    int count = rules_all(board, side, moves, MAX_MOVES);
    if (count == 0) return best;

    int best_score = -1000000;
    for (int i = 0; i < count; ++i) {
        int score = score_move(board, &moves[i], side, improved);
        score += rand() % 5;
        if (score > best_score) {
            best_score = score;
            best = moves[i];
        }
    }
    return best;
}
