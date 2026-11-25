#ifndef SHOGUN_CONFIG_H
#define SHOGUN_CONFIG_H

#include "types.h"

/* Returns -1 when no config file exists, otherwise a settings bitfield. */
int config_load(void);
void config_save(const Settings *settings);
void settings_from_bits(Settings *settings, int bits);

#endif
