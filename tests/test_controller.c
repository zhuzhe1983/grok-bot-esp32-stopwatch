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
static void click_gestures(void) {
    demo_buttons_t b={0};demo_input_t in={0};
    demo_buttons_update(&b,10,true,false,&in);
    demo_buttons_update(&b,80,false,false,&in);assert(!in.next_character);
    demo_buttons_update(&b,180,true,false,&in);
    demo_buttons_update(&b,240,false,false,&in);assert(in.recenter && !in.next_character);
    demo_buttons_update(&b,700,false,false,&in);assert(!in.recenter && !in.next_character);
    demo_buttons_update(&b,800,true,false,&in);
    demo_buttons_update(&b,880,false,false,&in);
    demo_buttons_update(&b,1201,false,false,&in);assert(in.next_character && !in.recenter);
    demo_buttons_update(&b,1300,false,true,&in);
    demo_buttons_update(&b,1380,false,false,&in);assert(in.next_color);
    demo_buttons_update(&b,1500,true,true,&in);
    demo_buttons_update(&b,2500,true,true,&in);assert(in.recenter);
    demo_buttons_update(&b,2600,false,true,&in);assert(!in.next_character && !in.next_color && !in.recenter);
    demo_buttons_update(&b,2700,false,false,&in);assert(!in.next_character && !in.next_color);
    demo_buttons_update(&b,3200,false,false,&in);assert(!in.next_character);
    b=(demo_buttons_t){0};
    demo_buttons_update(&b,UINT32_MAX-200,true,false,&in);
    demo_buttons_update(&b,UINT32_MAX-100,false,false,&in);
    demo_buttons_update(&b,50,true,false,&in);
    demo_buttons_update(&b,100,false,false,&in);assert(in.recenter && !in.next_character);
}
static void three_dimensional_neutral(void) {
    const float poses[][3]={{0,0,1},{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,-1},{.3f,.4f,.8660254f}};
    /* Independent expected tangent directions for these physical poses.
       Do not derive stimulus from the controller's computed basis. */
    const float tangents[][2][3]={
        {{1,0,0},{0,1,0}},{{1,0,0},{0,0,-1}},{{1,0,0},{0,0,1}},
        {{0,0,-1},{0,1,0}},{{0,0,1},{0,1,0}},{{1,0,0},{0,-1,0}},
        {{.9517691f,-.0643078f,-.3f},{-.0643078f,.9142563f,-.4f}}
    };
    for(unsigned pose=0;pose<sizeof(poses)/sizeof(poses[0]);pose++) {
        demo_controller_t d;demo_init(&d,0,42,true);
        demo_input_t in={.acceleration_valid=true,.ax=poses[pose][0],.ay=poses[pose][1],.az=poses[pose][2]};
        for(int i=0;i<40;i++){in.now_ms+=10;demo_update(&d,&in);}
        assert(d.imu_ready && fabsf(d.face.input_x)<.001f && fabsf(d.face.input_y)<.001f);
        for(int axis=0;axis<2;axis++)for(int sign=-1;sign<=1;sign+=2) {
            const float *basis=tangents[pose][axis];
            in.ax=poses[pose][0]*.9396926f+basis[0]*.3420201f*sign;
            in.ay=poses[pose][1]*.9396926f+basis[1]*.3420201f*sign;
            in.az=poses[pose][2]*.9396926f+basis[2]*.3420201f*sign;
            for(int i=0;i<100;i++){in.now_ms+=10;demo_update(&d,&in);}
            float primary=axis?d.face.input_y:d.face.input_x,other=axis?d.face.input_x:d.face.input_y;
            assert(primary*sign>.53f && primary*sign<.59f && fabsf(other)<.01f);
        }
        in.recenter=true;in.now_ms+=10;demo_update(&d,&in);in.recenter=false;
        for(int i=0;i<40;i++){in.now_ms+=10;demo_update(&d,&in);}
        assert(d.imu_ready && fabsf(d.face.input_x)<.01f && fabsf(d.face.input_y)<.01f);
    }
    demo_controller_t d;demo_init(&d,0,42,true);
    demo_input_t in={.acceleration_valid=true,.az=.98f};
    for(int i=0;i<100;i++){in.now_ms+=10;in.ax=(i&1)?.2f:-.2f;demo_update(&d,&in);}
    assert(!d.imu_ready); // Moving samples must not become the new level pose.
    in.ax=0;in.az=1;
    for(int i=0;i<40;i++){in.now_ms+=10;demo_update(&d,&in);}
    assert(d.imu_ready);
    demo_init(&d,0,42,true);in=(demo_input_t){.acceleration_valid=true,.az=1};
    for(int i=0;i<400;i++){in.now_ms+=1;demo_update(&d,&in);}
    assert(d.imu_ready); // A fast sensor must not wrap the stable-sample count.
}
static void gyro_relative_follow(void) {
    /* Physical rotations of a flat or upright display. Upright yaw leaves
       gravity unchanged and therefore exposes the old accel-only blind spot. */
    for(int upright=0;upright<2;upright++)for(int axis=0;axis<2;axis++)for(int sign=-1;sign<=1;sign+=2) {
        demo_controller_t d;demo_init(&d,0,42,true);
        demo_input_t in={.acceleration_valid=true,.angular_velocity_valid=true,
            .ay=upright?1:0,.az=upright?0:1,.gx=.25f,.gy=-.4f,.gz=.18f};
        for(int i=0;i<50;i++){in.now_ms+=10;demo_update(&d,&in);}
        assert(d.imu_ready && d.gyro_seen);
        assert(fabsf(d.gyro_bias[0]-.25f)<.001f && fabsf(d.gyro_bias[1]+.4f)<.001f);
        for(int i=1;i<=100;i++) {
            float a=sign*20.f*i/100*.01745329252f;
            in.gx=.25f+(axis==0?sign*20.f:0);in.gy=-.4f+(axis==1?sign*20.f:0);
            if(axis==0){in.ax=0;in.ay=upright?cosf(a):sinf(a);in.az=upright?-sinf(a):cosf(a);}
            else {in.ax=upright?0:-sinf(a);in.ay=upright?1:0;in.az=upright?0:cosf(a);}
            in.now_ms+=10;demo_update(&d,&in);
        }
        float primary=axis?d.face.input_x:d.face.input_y,other=axis?d.face.input_y:d.face.input_x;
        assert(primary*(axis?-sign:sign)>.53f && primary*(axis?-sign:sign)<.60f && fabsf(other)<.01f);
        in.gx=.25f;in.gy=-.4f;
        for(int i=0;i<300;i++){in.now_ms+=10;demo_update(&d,&in);}
        assert(fabsf((axis?d.face.input_x:d.face.input_y)-primary)<.02f);
        in.touching=true;in.touch_x=.2f;in.touch_y=-.3f;in.now_ms+=10;demo_update(&d,&in);
        assert(d.face.input_x==.2f && d.face.input_y==-.3f);in.touching=false;
        in.recenter=true;in.now_ms+=10;demo_update(&d,&in);in.recenter=false;
        for(int i=0;i<50;i++){in.now_ms+=10;demo_update(&d,&in);}
        assert(d.imu_ready && fabsf(d.face.input_x)<.01f && fabsf(d.face.input_y)<.01f);
    }
    demo_controller_t d;demo_init(&d,0,42,true);
    demo_input_t in={.acceleration_valid=true,.angular_velocity_valid=true,.ay=1,.gy=20};
    for(int i=0;i<100;i++){in.now_ms+=10;demo_update(&d,&in);}
    assert(!d.imu_ready); // Rotation around gravity is not a stable neutral pose.
}
int main(void) {
    buttons_and_bag(); tilt_touch_and_dropout(); millis_wrap();
    click_gestures();three_dimensional_neutral();gyro_relative_follow();
    puts("controller: buttons, full random cycles, touch priority, filtered tilt, 6-axis relative rotation, gyro bias, recenter, IMU dropout, timer wrap passed");
    return 0;
}
