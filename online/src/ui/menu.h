#ifndef SHOGUN_MENU_H
#define SHOGUN_MENU_H

#include <stddef.h>

#include "core/types.h"
#include "render/layout.h"

const char *menu_name(int index);
const char *menu_name_portrait(int index);
void menu_status(int index, const Settings *settings, char *out, size_t size);

#endif
