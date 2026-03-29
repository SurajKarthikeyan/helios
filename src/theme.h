#ifndef THEME_H
#define THEME_H

#include <ncurses.h>

typedef struct {
    char name[32];
    short primary;
    short secondary;
    short warning;
    short critical;
    short accent;
    short background;
} Theme;

typedef enum {
    THEME_PHOSPHOR = 0,
    THEME_AMBER    = 1,
    THEME_CYAN     = 2,
    THEME_MINIMAL  = 3
} ThemeID;

void theme_init(ThemeID id);
Theme theme_get(void);

#endif
