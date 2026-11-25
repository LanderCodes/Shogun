#include "menu.h"

#include <stdio.h>

/* Labels as drawn by the original game. The settings captions are rendered
   in upper case in landscape and in mixed case in portrait. */
static const char *NAMES[MENU_ITEMS] = {
    "ONE PLAYER", "TWO PLAYERS", "GAME RULES", "SHOW MOVE GUIDES",
    "ALWAYS SHOW TERRAIN", "COMPUTER OPPONENT"
};

static const char *PORTRAIT_NAMES[MENU_ITEMS] = {
    "ONE PLAYER", "TWO PLAYERS", "GAME RULES", "Show move guides:",
    "Always show terrain:", "Computer:"
};

const char *menu_name(int index)
{
    if (index < 0 || index >= MENU_ITEMS) return "";
    return NAMES[index];
}

const char *menu_name_portrait(int index)
{
    if (index < 0 || index >= MENU_ITEMS) return "";
    return PORTRAIT_NAMES[index];
}

void menu_status(int index, const Settings *settings, char *out, size_t size)
{
    if (size == 0) return;
    switch (index) {
    case 3:
        snprintf(out, size, "%s", settings->show_moves ? "ON" : "OFF");
        break;
    case 4:
        snprintf(out, size, "%s", settings->show_terrain ? "ON" : "OFF");
        break;
    case 5:
        snprintf(out, size, "%s", settings->improved_ai ? "IMPROVED" : "CLASSIC");
        break;
    default:
        out[0] = '\0';
        break;
    }
}
