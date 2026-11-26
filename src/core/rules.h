#ifndef SHOGUN_RULES_H
#define SHOGUN_RULES_H

#include "core/types.h"

int rules_generate(const Board *board, int row, int col, Move *out, int max);
int rules_all(const Board *board, Side side, Move *out, int max);
int rules_in_check(const Board *board, Side side);
int rules_winner(const Board *board);
int rules_has_any(const Board *board, Side side);

#endif
