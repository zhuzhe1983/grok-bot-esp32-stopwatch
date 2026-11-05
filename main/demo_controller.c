#include "demo_controller.h"
#include <math.h>
#include <string.h>

static float bound(float x, float lo, float hi) {
    return isfinite(x) ? fmaxf(lo, fminf(hi, x)) : 0;
}
static uint32_t random_next(demo_controller_t *d) {
    uint32_t x=d->rng;
    x^=x<<13; x^=x>>17; x^=x<<5;
    return d->rng=x;
}
static float angle(float a, float b, float c) { return atan2f(a, hypotf(b,c)); }
static void shuffle(demo_controller_t *d) {
    for (unsigned i=0;i<EA_FACE_COUNT;i++) d->bag[i]=(uint8_t)i;
    for (unsigned i=EA_FACE_COUNT-1;i>0;i--) {
        unsigned j=random_next(d)%(i+1);
        uint8_t old=d->bag[i]; d->bag[i]=d->bag[j]; d->bag[j]=old;
    }
    if (d->bag[0]==d->face.state) {
        uint8_t old=d->bag[0]; d->bag[0]=d->bag[1]; d->bag[1]=old;
    }
    d->bag_index=0;
}
void demo_init(demo_controller_t *d, uint32_t now, uint32_t seed, bool imu) {
    memset(d,0,sizeof(*d));
    ea_interaction_config_t config=ea_interaction_default();
    config.enabled=true; config.fps=30;
    ea_interaction_init(&d->face,&config,false);
    d->rng=seed?seed:0x695ab123;
    d->last_ms=now; d->next_expression_ms=now+3000;
    d->bag_index=EA_FACE_COUNT; d->imu_enabled=imu;
}
static void update_acceleration(demo_controller_t *d,const demo_input_t *in,float dt) {
    if (in->recenter) {
        d->imu_ready=false; d->calibration_samples=0;
        d->sum_x=d->sum_y=d->sum_z=0;
    }
    if (!d->imu_enabled || !in->acceleration_valid || !isfinite(in->ax) ||
        !isfinite(in->ay) || !isfinite(in->az)) return;
    float magnitude=sqrtf(in->ax*in->ax+in->ay*in->ay+in->az*in->az);
    /* Free fall and strong translation are not reliable gravity references. */
    if (magnitude<.75f || magnitude>1.25f) return;
    if (!d->have_acceleration) {
        d->filtered_x=in->ax; d->filtered_y=in->ay; d->filtered_z=in->az;
        d->have_acceleration=true;
    } else {
        float alpha=1-expf(-dt*7);
        d->filtered_x+=(in->ax-d->filtered_x)*alpha;
        d->filtered_y+=(in->ay-d->filtered_y)*alpha;
        d->filtered_z+=(in->az-d->filtered_z)*alpha;
    }
    d->last_imu_ms=in->now_ms;
    if (!d->imu_ready) {
        d->sum_x+=in->ax; d->sum_y+=in->ay; d->sum_z+=in->az;
        if (++d->calibration_samples==32) {
            d->neutral_x=angle(d->sum_x,d->sum_y,d->sum_z);
            d->neutral_y=angle(d->sum_y,d->sum_x,d->sum_z);
            d->imu_ready=true;
        }
    }
}
void demo_update(demo_controller_t *d,const demo_input_t *in) {
    float dt=fminf((uint32_t)(in->now_ms-d->last_ms)*.001f,.1f);
    d->last_ms=in->now_ms;
    if (in->next_character || in->next_color) {
        ea_interaction_config_t c=d->face.config;
        if (in->next_character) c.character=(c.character+1)%EA_INTERACTION_SHAPES;
        if (in->next_color) {
            static const uint8_t order[]={6,8,4,9,3,2,7,1,0,5,10};
            for (unsigned i=0;i<EA_INTERACTION_COLORS;i++) if (order[i]==c.color) {
                c.color=order[(i+1)%EA_INTERACTION_COLORS]; break;
            }
        }
        ea_interaction_configure(&d->face,&c);
    }
    if ((int32_t)(in->now_ms-d->next_expression_ms)>=0) {
        if (d->bag_index==EA_FACE_COUNT) shuffle(d);
        ea_interaction_set_state(&d->face,(ea_interaction_state_t)d->bag[d->bag_index++]);
        d->next_expression_ms=in->now_ms+2500+random_next(d)%2501;
    }
    update_acceleration(d,in,dt);
    if (in->touching) ea_interaction_look(&d->face,in->touch_x,in->touch_y,true);
    else if (d->imu_ready && (uint32_t)(in->now_ms-d->last_imu_ms)<500) {
        float x=(angle(d->filtered_x,d->filtered_y,d->filtered_z)-d->neutral_x)/.60f;
        float y=(angle(d->filtered_y,d->filtered_x,d->filtered_z)-d->neutral_y)/.60f;
        ea_interaction_look(&d->face,bound(x,-.80f,.80f),bound(y,-.80f,.80f),true);
    } else ea_interaction_look(&d->face,0,0,false);
    /* Silent demonstration envelope: no microphone or speech synthesis. */
    float t=d->face.time;
    float level=d->face.state==EA_FACE_SPEAK ? .25f+.45f*fabsf(sinf(t*8)*sinf(t*2.7f)) : 0;
    ea_interaction_level(&d->face,level);
    ea_interaction_step(&d->face,dt);
}
const char *demo_expression_name(ea_interaction_state_t state) {
    static const char *names[]={"IDLE","LISTEN","THINK","SPEAK","DONE","ERROR",
        "SLEEP","ASK","HAPPY","SURPRISE","WINK","LOVE"};
    return (unsigned)state<EA_FACE_COUNT?names[state]:"UNKNOWN";
}
