#include "menu.h"

#include <stdio.h>

static const char *NAMES[MENU_ITEMS] = {
    "ONE PLAYER", "TWO PLAYERS", "GAME RULES", "SHOW MOVE GUIDES",
    "ALWAYS SHOW TERRAIN", "COMPUTER OPPONENT"
};

const char *menu_name(int index)
{
    if (index < 0 || index >= MENU_ITEMS) return "";
    return NAMES[index];
}

void menu_status(int index, const Settings *settings, char *out, size_t size)
{
    if (size == 0) return;
    switch (index) {
    case 0:
        snprintf(out, size, "WHITE - YOU");
        break;
    case 1:
        snprintf(out, size, "TWO PLAYERS");
        break;
    case 2:
        snprintf(out, size, "READ THE RULES");
        break;
    case 3:
        snprintf(out, size, "%s", settings->show_moves ? "GUIDES ON" : "GUIDES OFF");
        break;
    case 4:
        snprintf(out, size, "%s", settings->show_terrain ? "TERRAIN ON" : "TERRAIN OFF");
        break;
    case 5:
        snprintf(out, size, "%s", settings->improved_ai ? "IMPROVED" : "CLASSIC");
        break;
    default:
        out[0] = '\0';
        break;
    }
}
