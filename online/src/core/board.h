#ifndef SHOGUN_BOARD_H
#define SHOGUN_BOARD_H

#include "core/types.h"

void board_init(Board *board, uint32_t seed);
Side board_starting_side(const Board *board);
int board_value(const Board *board, int row, int col);
int board_count_pawns(const Board *board, Side side);
int board_has_shogun(const Board *board, Side side);
void board_vacate(Board *board, int row, int col);
void board_apply(Board *board, const Move *move);

#endif
