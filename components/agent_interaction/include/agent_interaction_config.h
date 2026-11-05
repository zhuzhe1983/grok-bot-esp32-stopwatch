#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Preferences only. Does not enable recording, playback or network services. */
#define EA_INTERACTION_SHAPES 13u
#define EA_INTERACTION_COLORS 11u
typedef enum { EA_THEME_GROK, EA_THEME_RIPPLE } ea_interaction_theme_t;
typedef struct {
    bool enabled;
    uint8_t theme, character, color, fps;
    bool motion;
} ea_interaction_config_t;
static inline ea_interaction_config_t ea_interaction_default(void) {
    return (ea_interaction_config_t){false, EA_THEME_GROK, 6, 0, 20, true};
}
static inline bool ea_interaction_config_valid(const ea_interaction_config_t *c) {
    return c && c->theme <= EA_THEME_RIPPLE && c->character < EA_INTERACTION_SHAPES &&
        c->color < EA_INTERACTION_COLORS && (c->fps == 10 || c->fps == 20 || c->fps == 30);
}
