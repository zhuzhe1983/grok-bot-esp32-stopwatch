#include "demo_controller.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void buttons_and_bag(void) {
    demo_controller_t d; demo_init(&d,0,123,false);
    unsigned initial=d.face.config.character,seen=0;
    demo_input_t in={.now_ms=1,.next_character=true};
    for (unsigned i=0;i<13;i++) { demo_update(&d,&in); seen|=1u<<d.face.config.character; }
    assert(d.face.config.character==initial && seen==0x1fff);
    initial=d.face.config.color; seen=0; in.next_character=false; in.next_color=true;
    for (unsigned i=0;i<11;i++) { demo_update(&d,&in); seen|=1u<<d.face.config.color; }
    assert(d.face.config.color==initial && seen==0x7ff);
    in.next_color=false;
    ea_interaction_state_t previous=d.face.state;
    for (unsigned cycle=0;cycle<20;cycle++) {
        seen=0;
        for (unsigned i=0;i<EA_FACE_COUNT;i++) {
            in.now_ms=d.next_expression_ms; demo_update(&d,&in);
            assert(d.face.state!=previous); previous=d.face.state;
            assert(!(seen&(1u<<d.face.state))); seen|=1u<<d.face.state;
            assert(d.next_expression_ms-in.now_ms>=2500 && d.next_expression_ms-in.now_ms<=5000);
        }
        assert(seen==0xfff);
    }
}
static void tilt_touch_and_dropout(void) {
    demo_controller_t d; demo_init(&d,0,42,true);
    demo_input_t in={.acceleration_valid=true,.az=1};
    for (unsigned i=0;i<40;i++) { in.now_ms+=10; demo_update(&d,&in); }
    assert(d.imu_ready && d.face.look_active && fabsf(d.face.input_x)<.001f);
    in.ax=.5f; in.az=.8660254f;
    for (unsigned i=0;i<100;i++) { in.now_ms+=10; demo_update(&d,&in); }
    assert(d.face.input_x>.7f && d.face.input_x<=.8f && d.face.head_x>.6f);
    in.touching=true; in.touch_x=-.6f; in.touch_y=.3f; in.now_ms+=10; demo_update(&d,&in);
    assert(d.face.input_x==-.6f && d.face.input_y==.3f);
    float before=d.face.gaze_x;
    in.touching=false; in.now_ms+=10; demo_update(&d,&in);
    assert(d.face.input_x>.7f && fabsf(d.face.gaze_x-before)<.25f);
    in.recenter=true; in.now_ms+=10; demo_update(&d,&in); in.recenter=false;
    for (unsigned i=0;i<40;i++) { in.now_ms+=10; demo_update(&d,&in); }
    assert(d.imu_ready && fabsf(d.face.input_x)<.01f);
    in.acceleration_valid=false; in.now_ms+=600; demo_update(&d,&in);
    assert(!d.face.look_active);
    in.touching=true; in.touch_x=3; in.touch_y=-4; in.now_ms+=10; demo_update(&d,&in);
    assert(d.face.look_active && d.face.input_x==1 && d.face.input_y==-1);
    in.touching=false; in.acceleration_valid=true; in.ax=NAN; in.now_ms+=10; demo_update(&d,&in);
    assert(!d.face.look_active && isfinite(d.face.gaze_x));
}
static void millis_wrap(void) {
    demo_controller_t d; uint32_t start=UINT32_MAX-1000;
    demo_init(&d,start,7,false);
    demo_input_t in={.now_ms=start+500}; demo_update(&d,&in); assert(d.face.state==EA_FACE_IDLE);
    in.now_ms=start+3000; demo_update(&d,&in); assert(d.face.state!=EA_FACE_IDLE);
    assert(isfinite(d.face.time) && d.face.time>0);
}
int main(void) {
    buttons_and_bag(); tilt_touch_and_dropout(); millis_wrap();
    puts("controller: buttons, full random cycles, touch priority, filtered tilt, recenter, IMU dropout, timer wrap passed");
    return 0;
}
