#ifndef SHOGUN_BOARD_H
#define SHOGUN_BOARD_H

#include "types.h"

void board_init(Board *board, uint32_t seed);
int board_value(const Board *board, int row, int col);
int board_count_pawns(const Board *board, Side side);
int board_has_shogun(const Board *board, Side side);
void board_vacate(Board *board, int row, int col);
void board_apply(Board *board, const Move *move);

#endif
