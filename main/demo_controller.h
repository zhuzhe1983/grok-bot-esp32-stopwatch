#pragma once
#include "agent_interaction.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t now_ms;
    bool next_character, next_color, recenter;
    bool touching;
    float touch_x, touch_y; /* Normalized screen coordinates, -1..1. */
    bool acceleration_valid;
    float ax, ay, az; /* Screen-aligned acceleration in g. */
    bool angular_velocity_valid;
    float gx, gy, gz; /* Same right-handed axes, degrees/second. */
} demo_input_t;

typedef struct {
    ea_interaction_t face;
    uint32_t rng, last_ms, next_expression_ms, last_imu_ms;
    uint8_t bag[EA_FACE_COUNT], bag_index;
    uint32_t calibration_samples;
    float filtered_x, filtered_y, filtered_z;
    float neutral[3],basis_x[3],basis_y[3],sum_x,sum_y,sum_z;
    uint32_t calibration_started_ms,last_gyro_ms,gyro_samples;
    float orientation[4],gyro_sum[3],gyro_bias[3]; /* w,x,y,z; body -> neutral */
    bool gyro_seen;
    bool imu_enabled, imu_ready, have_acceleration;
} demo_controller_t;

typedef struct {
    uint32_t left_down_ms,right_down_ms,left_up_ms,both_since_ms;
    bool left,right,left_pending,left_second,chord,chord_fired;
} demo_buttons_t;
/* Debounced raw button states; single A is delayed 320 ms to distinguish a
   double click, which recenters without changing the face. */
void demo_buttons_update(demo_buttons_t *buttons,uint32_t now,bool left,bool right,demo_input_t *events);

void demo_init(demo_controller_t *demo, uint32_t now_ms, uint32_t seed, bool imu);
void demo_update(demo_controller_t *demo, const demo_input_t *input);
const char *demo_expression_name(ea_interaction_state_t state);
#ifdef __cplusplus
}
#endif
