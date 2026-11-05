#pragma once
#include "agent_interaction.h"

/* Unit-sphere eye geometry, private to the renderer and its geometry tests.
   The contour is clipped to z >= 0 before orthographic projection. */
enum { EAI_EYE_CAPACITY = 128 };
int eai_project_eye(ea_interaction_state_t state, int eye, float blink,
                    float yaw, float pitch, float roll,
                    float x[EAI_EYE_CAPACITY], float y[EAI_EYE_CAPACITY]);
/* Fixed decals on the selected solid's front surface, in body coordinates. */
int eai_project_surface_eye(unsigned character, ea_interaction_state_t state,
                    int eye, float blink, float yaw, float pitch,
                    float x[EAI_EYE_CAPACITY], float y[EAI_EYE_CAPACITY]);
