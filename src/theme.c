#include "theme.h"
#include <string.h>

static Theme current_theme;

static Theme themes[4] = {
    {"PHOSPHOR GREEN", COLOR_GREEN,  COLOR_GREEN,  COLOR_YELLOW, COLOR_RED,   COLOR_GREEN,  COLOR_BLACK},
    {"AMBER",          COLOR_YELLOW, COLOR_YELLOW, COLOR_RED,    COLOR_RED,   COLOR_YELLOW, COLOR_BLACK},
    {"COLD CYAN",      COLOR_CYAN,   COLOR_CYAN,   COLOR_YELLOW, COLOR_RED,   COLOR_CYAN,   COLOR_BLACK},
    {"MINIMAL WHITE",  COLOR_WHITE,  COLOR_WHITE,  COLOR_YELLOW, COLOR_RED,   COLOR_WHITE,  COLOR_BLACK},
};

void theme_init(ThemeID id) {
    current_theme = themes[id];

    init_pair(1, current_theme.primary,    current_theme.background);
    init_pair(2, current_theme.secondary,  current_theme.background);
    init_pair(3, current_theme.warning,    current_theme.background);
    init_pair(4, current_theme.critical,   current_theme.background);
    init_pair(5, current_theme.accent,     current_theme.background);
}

Theme theme_get(void) {
    return current_theme;
}
