#include "agent_interaction.h"
#include "interaction_projection.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static uint32_t pixels[256*256];
static unsigned calls;
static float eye_area(ea_interaction_state_t state,int eye,float yaw,float pitch) {
    float x[EAI_EYE_CAPACITY],y[EAI_EYE_CAPACITY],sum=0;
    int n=eai_project_eye(state,eye,1,yaw,pitch,0,x,y);
    assert(n>=0 && n<=EAI_EYE_CAPACITY);
    for(int i=0;i<n;i++){
        assert(isfinite(x[i]) && isfinite(y[i]) && x[i]*x[i]+y[i]*y[i]<=1.00001f);
        int next=(i+1)%n;sum+=x[i]*y[next]-x[next]*y[i];
    }
    return fabsf(sum)*.5f;
}
static void surface_projection(void) {
    for(int state=0;state<EA_FACE_COUNT;state++) {
        float front=eye_area(state,1,0,0),last=front;
        assert(front>0);
        assert(fabsf(front-eye_area(state,0,0,0))<.00001f || state==EA_FACE_WINK);
        for(int step=1;step<=90;step++) {
            float area=eye_area(state,1,step*.02f,0);
            /* The far eye shrinks continuously into the limb, never folds
               back onto the visible face when its surface turns away. */
            assert(area<=last+.00002f);last=area;
        }
        assert(last==0 && eye_area(state,0,1.5f,0)>0);
        assert(eye_area(state,0,3.14159265f,0)==0 && eye_area(state,1,3.14159265f,0)==0);
        for(int a=-7;a<=7;a++)for(int b=-6;b<=6;b++)
            for(int eye=0;eye<2;eye++)(void)eye_area(state,eye,a*.25f,b*.22f);
    }
}
static float solid_eye_area(unsigned character,int eye,float yaw,float pitch) {
    float x[EAI_EYE_CAPACITY],y[EAI_EYE_CAPACITY],area=0;
    int n=eai_project_surface_eye(character,EA_FACE_IDLE,eye,1,yaw,pitch,x,y);
    for(int i=0;i<n;i++){int j=(i+1)%n;area+=x[i]*y[j]-x[j]*y[i];}
    return fabsf(area)*.5f;
}
static void shape_surface_projection(void) {
    /* A decal on a flat face follows affine foreshortening, not the separate
       near/far eye compression of a sphere. */
    float front=solid_eye_area(2,1,0,0),turned=solid_eye_area(2,1,.8f,.2f);
    assert(fabsf(turned-front*cosf(.8f)*cosf(.2f))<.00001f);
    assert(fabsf(turned-solid_eye_area(2,0,.8f,.2f))<.00001f);
    assert(fabsf(solid_eye_area(0,1,.8f,.2f)-solid_eye_area(0,0,.8f,.2f))>.01f);
    for(unsigned shape=0;shape<EA_INTERACTION_SHAPES;shape++)
        for(int state=0;state<EA_FACE_COUNT;state++)for(int eye=0;eye<2;eye++) {
            float x[EAI_EYE_CAPACITY],y[EAI_EYE_CAPACITY];
            assert(eai_project_surface_eye(shape,state,eye,1,0,0,x,y)>=3);
            assert(eai_project_surface_eye(shape,state,eye,1,3.14159265f,0,x,y)==0);
            for(int a=-5;a<=5;a++)for(int b=-4;b<=4;b++) {
                int n=eai_project_surface_eye(shape,state,eye,1,a*.3f,b*.28f,x,y);
                assert(n>=0 && n<=EAI_EYE_CAPACITY);
                for(int i=0;i<n;i++)assert(isfinite(x[i]) && isfinite(y[i]) && fabsf(x[i])<1.6f && fabsf(y[i])<1.6f);
            }
        }
}
static void span(void *ctx,int x1,int x2,int y,uint32_t color) {
    (void)ctx;assert(x1>=0 && x2<256 && x1<=x2 && y>=0 && y<256);++calls;
    for(int x=x1;x<=x2;x++)pixels[y*256+x]=color;
}
static void write_image(const char *dir,unsigned theme,unsigned shape){
    char path[1024];snprintf(path,sizeof(path),"%s/theme-%u-%02u.ppm",dir,theme,shape);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n256 256\n255\n");
    for(size_t i=0;i<256*256;i++){uint8_t rgb[]={(uint8_t)(pixels[i]>>16),(uint8_t)(pixels[i]>>8),(uint8_t)pixels[i]};assert(fwrite(rgb,1,3,f)==3);}fclose(f);
}
static float body_center_x(void) {
    int left=256,right=-1;
    for(int y=0;y<256;y++)for(int x=0;x<256;x++)if(pixels[y*256+x]==0x915bfc){
        if(x<left)left=x;if(x>right)right=x;
    }
    assert(right>=left);return (left+right)/2.f;
}
static void upright_rigid_pose(void) {
    for(unsigned shape=0;shape<EA_INTERACTION_SHAPES;shape++) {
        ea_interaction_config_t c=ea_interaction_default();c.enabled=true;c.character=shape;
        ea_interaction_t f;ea_interaction_init(&f,&c,false);
        assert(f.gaze_x==0 && f.gaze_y==0);
        int reference_w=0,reference_h=0;
        for(unsigned state=0;state<EA_FACE_COUNT;state++) {
            ea_interaction_set_state(&f,state);f.level=1;
            ea_interaction_render(&f,256,0x1e1e20,span,NULL);
            int left=256,right=-1,top=256,bottom=-1;
            /* Status dots are below the body; eye holes cannot affect its bounds. */
            for(int y=0;y<222;y++)for(int x=0;x<256;x++)if(pixels[y*256+x]==0x915bfc) {
                if(x<left)left=x;if(x>right)right=x;
                if(y<top)top=y;if(y>bottom)bottom=y;
            }
            if(!state){reference_w=right-left;reference_h=bottom-top;}
            assert(abs(right-left-reference_w)<=1 && abs(bottom-top-reference_h)<=1);
        }
    }
    /* A cone's far eye wraps away before the near eye; a flat triangle would
       compress both decals equally and would not satisfy this check. */
    assert(fabsf(solid_eye_area(8,1,0,0)-solid_eye_area(8,0,0,0))<.0001f);
    assert(solid_eye_area(8,0,1.25f,0)>solid_eye_area(8,1,1.25f,0)*3);
    assert(solid_eye_area(8,1,1.7f,0)==0);
}
static void head_follows_input(void) {
    ea_interaction_config_t c=ea_interaction_default();c.enabled=true;c.character=0;
    float centers[2];
    for(int side=0;side<2;side++) {
        ea_interaction_t u;ea_interaction_init(&u,&c,false);
        ea_interaction_look(&u,side?.9f:-.9f,0,true);
        ea_interaction_step(&u,1.f/60);
        assert(fabsf(u.head_x)>0 && fabsf(u.head_x)<.1f); /* no instant jump */
        for(int frame=0;frame<120;frame++)ea_interaction_step(&u,1.f/60);
        ea_interaction_render(&u,256,0x1e1e20,span,NULL);centers[side]=body_center_x();
        ea_interaction_look(&u,0,0,false);
        for(int frame=0;frame<120;frame++)ea_interaction_step(&u,1.f/60);
        assert(fabsf(u.head_x)<.001f && fabsf(u.head_y)<.001f);
    }
    assert(centers[1]-centers[0]>10); /* body silhouette, not just eye holes */
}
int main(int argc,char **argv){
    surface_projection();
    shape_surface_projection();
    assert(argc==2);ea_interaction_config_t c=ea_interaction_default();ea_interaction_t ui,before;
    ea_interaction_init(&ui,&c,false);before=ui;ea_interaction_step(&ui,1);ea_interaction_render(&ui,256,0x1e1e20,span,NULL);
    assert(!calls && !memcmp(&ui,&before,sizeof(ui)) && !ea_interaction_animated(&ui));
    head_follows_input();
    upright_rigid_pose();
    c.enabled=true;assert(ea_interaction_configure(&ui,&c));assert(ea_interaction_animated(&ui));
    ea_interaction_level(&ui,.5f);assert(!ui.look_active);
    ea_interaction_look(&ui,0,0,true);
    for(int i=0;i<100;i++)ea_interaction_step(&ui,1.f/60);
    assert(fabsf(ui.gaze_x)<.001f && fabsf(ui.gaze_y)<.001f);
    ea_interaction_look(&ui,0,0,false);
    for(int i=0;i<100;i++)ea_interaction_step(&ui,1.f/60);
    assert(fabsf(ui.gaze_x)<.11f && fabsf(ui.gaze_y)<.07f);
    for(unsigned theme=0;theme<2;theme++)for(unsigned shape=0;shape<13;shape++){
        c.theme=theme;c.character=shape;c.color=shape%EA_INTERACTION_COLORS;assert(ea_interaction_configure(&ui,&c));
        ea_interaction_input(&ui,.6f,.8f,-.2f);
        for(unsigned frame=0;frame<120;frame++)ea_interaction_step(&ui,1.f/60);
        for(unsigned state=0;state<EA_FACE_COUNT;state++){
            ea_interaction_set_state(&ui,state);ea_interaction_render(&ui,256,0x1e1e20,span,NULL);
            for(int i=0;i<64;i++)assert(isfinite(ui.shape[i]) && ui.shape[i]>.3f && ui.shape[i]<1.6f);
        }
        ea_interaction_set_state(&ui,EA_FACE_IDLE);ea_interaction_render(&ui,256,0x1e1e20,span,NULL);write_image(argv[1],theme,shape);
    }
    ui.visible=false;before=ui;ea_interaction_step(&ui,1);assert(!memcmp(&ui,&before,sizeof(ui)));
    ui.visible=true;ui.monochrome=true;before=ui;ea_interaction_step(&ui,1);assert(!memcmp(&ui,&before,sizeof(ui)));
    c.fps=255;assert(!ea_interaction_configure(&ui,&c));c.fps=20;
    ea_interaction_input(&ui,NAN,INFINITY,-INFINITY);assert(ui.input_level==0 && isfinite(ui.input_x));
    int16_t pcm[]={32767,-32768,1000,-1000},copy[4];memcpy(copy,pcm,sizeof(pcm));
    assert(ea_interaction_pcm_level(pcm,4)>0.9f && !memcmp(copy,pcm,sizeof(pcm)));
    assert(ea_interaction_pcm_level(NULL,0)==0);int16_t silence[512]={0};assert(ea_interaction_pcm_level(silence,512)==0);
    ui.monochrome=false;clock_t start=clock();for(int i=0;i<100;i++)ea_interaction_render(&ui,256,0,span,NULL);
    printf("renderer=%zu bytes, RGB565 canvas=131072 bytes, desktop mean=%.3f ms/frame\n",sizeof(ui),1000.*(clock()-start)/CLOCKS_PER_SEC/100);
    puts("shape/state bounds, off/hidden/e-paper inactivity, PCM integrity passed");return 0;
}
