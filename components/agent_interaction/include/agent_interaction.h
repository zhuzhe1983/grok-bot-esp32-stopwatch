#pragma once
#include <stddef.h>
#include "agent_interaction_config.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    EA_FACE_IDLE, EA_FACE_LISTEN, EA_FACE_THINK, EA_FACE_SPEAK, EA_FACE_DONE,
    EA_FACE_ERROR, EA_FACE_SLEEP, EA_FACE_ASK, EA_FACE_HAPPY, EA_FACE_SURPRISE,
    EA_FACE_WINK, EA_FACE_LOVE, EA_FACE_COUNT
} ea_interaction_state_t;
/* Caller owns the clock and display. No allocation, threads or timers here. */
typedef struct {
    ea_interaction_config_t config;
    ea_interaction_state_t state;
    float shape[64], target[64], velocity[64];
    float time, state_time, level, gaze_x, gaze_y;
    float input_level, input_x, input_y;
    float head_x, head_y, head_vx, head_vy;
    bool visible, monochrome, look_active;
} ea_interaction_t;
/* Inclusive horizontal span in an already clipped viewport; color is RGB888. */
typedef void (*ea_interaction_span_t)(void *ctx, int x1, int x2, int y, uint32_t color);
void ea_interaction_init(ea_interaction_t *ui, const ea_interaction_config_t *config, bool monochrome);
bool ea_interaction_configure(ea_interaction_t *ui, const ea_interaction_config_t *config);
void ea_interaction_set_state(ea_interaction_t *ui, ea_interaction_state_t state);
void ea_interaction_input(ea_interaction_t *ui, float level, float gaze_x, float gaze_y);
/* Normalized pointer/touch position; (0,0) looks straight at the viewer.
   active=false returns to the resting pose. Audio never activates gaze. */
void ea_interaction_look(ea_interaction_t *ui, float x, float y, bool active);
void ea_interaction_level(ea_interaction_t *ui, float level);
bool ea_interaction_animated(const ea_interaction_t *ui);
void ea_interaction_step(ea_interaction_t *ui, float seconds);
void ea_interaction_render(const ea_interaction_t *ui, int size, uint32_t background,
                           ea_interaction_span_t span, void *ctx);
/* Read-only RMS, normalized 0..1. Audio bytes/rate/pitch are never modified. */
float ea_interaction_pcm_level(const int16_t *samples, size_t count);
#ifdef __cplusplus
}
#endif
