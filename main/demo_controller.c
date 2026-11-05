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
static float dot(const float a[3],const float b[3]) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
void demo_buttons_update(demo_buttons_t *b,uint32_t now,bool left,bool right,demo_input_t *in) {
    in->next_character=in->next_color=in->recenter=false;
    if(left && right && !b->chord) {
        b->chord=true;b->chord_fired=false;b->both_since_ms=now;
        b->left_pending=b->left_second=false;
    }
    if(b->chord) {
        if(left && right && !b->chord_fired && (uint32_t)(now-b->both_since_ms)>=1000) {
            in->recenter=true;b->chord_fired=true;
        }
        if(!left && !right)b->chord=false;
    } else {
        if(b->left_pending && (uint32_t)(now-b->left_up_ms)>320) {
            in->next_character=true;b->left_pending=false;
        }
        if(left && !b->left) {
            b->left_second=b->left_pending;
            b->left_pending=false;b->left_down_ms=now;
        }
        if(!left && b->left && (uint32_t)(now-b->left_down_ms)<600) {
            if(b->left_second) {in->recenter=true;b->left_second=false;}
            else {b->left_pending=true;b->left_up_ms=now;}
        }
        if(right && !b->right)b->right_down_ms=now;
        if(!right && b->right && (uint32_t)(now-b->right_down_ms)<600)in->next_color=true;
    }
    b->left=left;b->right=right;
}
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
static void update_acceleration(demo_controller_t *d,const demo_input_t *in) {
    if (in->recenter) {
        d->imu_ready=false; d->calibration_samples=0;
        d->sum_x=d->sum_y=d->sum_z=0;
        d->have_acceleration=false;
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
        float dt=fminf((uint32_t)(in->now_ms-d->last_imu_ms)*.001f,.1f);
        float alpha=1-expf(-dt*12);
        d->filtered_x+=(in->ax-d->filtered_x)*alpha;
        d->filtered_y+=(in->ay-d->filtered_y)*alpha;
        d->filtered_z+=(in->az-d->filtered_z)*alpha;
    }
    d->last_imu_ms=in->now_ms;
    if (!d->imu_ready) {
        /* Calibrate a stable gravity vector, not two independent Euler angles.
           That preserves the sign of forward/backward tilt when held upright. */
        if(d->calibration_samples) {
            float n=d->calibration_samples;
            float dx=in->ax-d->sum_x/n,dy=in->ay-d->sum_y/n,dz=in->az-d->sum_z/n;
            if(dx*dx+dy*dy+dz*dz>.0036f) {
                d->calibration_samples=0;d->sum_x=d->sum_y=d->sum_z=0;
            }
        }
        if(!d->calibration_samples)d->calibration_started_ms=in->now_ms;
        d->sum_x+=in->ax; d->sum_y+=in->ay; d->sum_z+=in->az;
        ++d->calibration_samples;
        if(d->calibration_samples>=32 && (uint32_t)(in->now_ms-d->calibration_started_ms)>=350) {
            float length=sqrtf(d->sum_x*d->sum_x+d->sum_y*d->sum_y+d->sum_z*d->sum_z);
            float x=d->sum_x/length,y=d->sum_y/length,z=d->sum_z/length;
            d->neutral[0]=x;d->neutral[1]=y;d->neutral[2]=z;
            if(z>-.9999f) {
                float k=1/(1+z);
                d->basis_x[0]=1-x*x*k;d->basis_x[1]=-x*y*k;d->basis_x[2]=-x;
                d->basis_y[0]=-x*y*k;d->basis_y[1]=1-y*y*k;d->basis_y[2]=-y;
            } else {
                d->basis_x[0]=1;d->basis_x[1]=d->basis_x[2]=0;
                d->basis_y[1]=-1;d->basis_y[0]=d->basis_y[2]=0;
            }
            d->filtered_x=x;d->filtered_y=y;d->filtered_z=z;
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
    update_acceleration(d,in);
    if (in->touching) ea_interaction_look(&d->face,in->touch_x,in->touch_y,true);
    else if (d->imu_ready && (uint32_t)(in->now_ms-d->last_imu_ms)<500) {
        const float g[]={d->filtered_x,d->filtered_y,d->filtered_z};
        float z=dot(g,d->neutral);
        float x=atan2f(dot(g,d->basis_x),z)/.60f;
        float y=atan2f(dot(g,d->basis_y),z)/.60f;
        /* Small center deadband removes sensor jitter without a hard jump. */
        x=copysignf(fmaxf(0,fabsf(x)-.025f),x);
        y=copysignf(fmaxf(0,fabsf(y)-.025f),y);
        ea_interaction_look(&d->face,bound(x,-.80f,.80f),bound(y,-.80f,.80f),true);
    } else if(d->imu_enabled && !d->imu_ready) {
        ea_interaction_look(&d->face,0,0,true);
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
