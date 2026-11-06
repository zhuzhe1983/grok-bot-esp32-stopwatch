#include "agent_interaction.h"
#include "interaction_projection.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define N 64
/* Same sampled RGB values and stable color IDs as the WebUI. */
static const uint32_t colors[] = {0x915bfc,0x1784fc,0x14c370,0xf59410,0xfc2d3e,0xfb3399,0xffffff,0x15b8a3,0xa16c39,0xfa6710,0x777777};
_Static_assert(sizeof(colors)/sizeof(colors[0]) == EA_INTERACTION_COLORS, "palette size");
static float clamp(float x, float lo, float hi) { return !isfinite(x) ? lo : fminf(hi, fmaxf(lo, x)); }
static void rest_pose(ea_interaction_state_t state,float *yaw,float *pitch) {
    *yaw=state==EA_FACE_THINK?-.15f:0;
    *pitch=state==EA_FACE_THINK?.12f:0;
}
#include "interaction_shapes.h"
#include "interaction_surface_data.h"
static void targets(ea_interaction_t *u) {
    for (int i=0;i<N;i++) u->target[i] = shape_radii[u->config.character][i];
}
/* Interpolate the 64 spring controls for smooth crowns at large display sizes.
   No additional persistent state or image buffer is required. */
static float body_radius(const ea_interaction_t *u,int i) {
    int k=i/2;
    if(!(i&1)) return u->shape[k];
    return (-u->shape[(k+N-1)%N]+9*u->shape[k]+9*u->shape[(k+1)%N]-u->shape[(k+2)%N])/16;
}
void ea_interaction_init(ea_interaction_t *u, const ea_interaction_config_t *c, bool mono) {
    if (!u) return;
    memset(u,0,sizeof(*u)); u->config=ea_interaction_default(); u->monochrome=mono; u->visible=true;
    rest_pose(EA_FACE_IDLE,&u->gaze_x,&u->gaze_y);
    if (ea_interaction_config_valid(c)) u->config=*c;
    targets(u); memcpy(u->shape,u->target,sizeof(u->shape));
}
bool ea_interaction_configure(ea_interaction_t *u, const ea_interaction_config_t *c) {
    if (!u || !ea_interaction_config_valid(c)) return false;
    u->config=*c; targets(u);
    if (!ea_interaction_animated(u)) { memcpy(u->shape,u->target,sizeof(u->shape)); memset(u->velocity,0,sizeof(u->velocity)); }
    return true;
}
void ea_interaction_set_state(ea_interaction_t *u, ea_interaction_state_t s) {
    if (u && (unsigned)s < EA_FACE_COUNT && u->state != s) { u->state=s; u->state_time=0; }
}
void ea_interaction_input(ea_interaction_t *u,float level,float x,float y) {
    ea_interaction_level(u,level);ea_interaction_look(u,x,y,true);
}
void ea_interaction_level(ea_interaction_t *u,float level) {
    if(u)u->input_level=clamp(level,0,1);
}
void ea_interaction_look(ea_interaction_t *u,float x,float y,bool active) {
    if (!u) return;
    u->input_x=clamp(x,-1,1);u->input_y=clamp(y,-1,1);u->look_active=active;
}
bool ea_interaction_animated(const ea_interaction_t *u) {
    return u && u->config.enabled && u->config.motion && u->visible && !u->monochrome;
}
void ea_interaction_step(ea_interaction_t *u,float dt) {
    if (!ea_interaction_animated(u)) return;
    dt=clamp(dt,0,.1f); u->time+=dt; u->state_time+=dt;
    u->level+=(u->input_level-u->level)*(1-expf(-dt*(u->input_level>u->level?22:8)));
    float yaw,pitch;rest_pose(u->state,&yaw,&pitch);
    if(u->look_active){yaw=u->input_x*1.50f;pitch=-u->input_y*1.15f;}
    else if(u->state!=EA_FACE_SLEEP){yaw+=.10f*sinf(u->time*.43f);pitch+=.06f*sinf(u->time*.37f);}
    u->gaze_x+=(yaw-u->gaze_x)*(1-expf(-dt*9));
    u->gaze_y+=(pitch-u->gaze_y)*(1-expf(-dt*9));
    /* Bounded 120 Hz spring substeps, including dropped frames. */
    while (dt>0) {
        float h=fminf(dt,1.f/120); dt-=h;
        float hx=u->look_active?u->input_x:0,hy=u->look_active?u->input_y:0;
        u->head_vx+=(110*(hx-u->head_x)-18*u->head_vx)*h;
        u->head_vy+=(110*(hy-u->head_y)-18*u->head_vy)*h;
        u->head_x+=u->head_vx*h;u->head_y+=u->head_vy*h;
        for (int i=0;i<N;i++) { u->velocity[i]+=(170*(u->target[i]-u->shape[i])-19*u->velocity[i])*h; u->shape[i]+=u->velocity[i]*h; }
    }
}
float ea_interaction_pcm_level(const int16_t *pcm,size_t n) {
    if (!pcm || !n) return 0;
    double sum=0;
    for (size_t i=0;i<n;i++) { double v=pcm[i]/32768.; sum+=v*v; }
    return clamp((float)sqrt(sum/n)*3,0,1);
}
typedef struct { int size; ea_interaction_span_t span; void *ctx; } painter_t;
static void span(painter_t *p,int x1,int x2,int y,uint32_t c) {
    if (y<0 || y>=p->size || x2<0 || x1>=p->size || x1>x2) return;
    p->span(p->ctx,x1<0?0:x1,x2>=p->size?p->size-1:x2,y,c);
}
static void polygon(painter_t *p,const float *x,const float *y,int n,uint32_t c) {
    float low=y[0],high=y[0];
    for(int i=1;i<n;i++){low=fminf(low,y[i]);high=fmaxf(high,y[i]);}
    int first=(int)fmaxf(0,ceilf(low)),last=(int)fminf(p->size-1,floorf(high));
    for(int row=first;row<=last;row++) {
        float hits[N*2]; int count=0;
        for(int i=0,j=n-1;i<n;j=i++) if ((y[i]>row)!=(y[j]>row)) {
            float hit=x[i]+(row-y[i])*(x[j]-x[i])/(y[j]-y[i]);
            int k=count++; while(k>0 && hits[k-1]>hit){hits[k]=hits[k-1];k--;} hits[k]=hit;
        }
        for(int i=0;i+1<count;i+=2) span(p,(int)ceilf(hits[i]),(int)floorf(hits[i+1]),row,c);
    }
}
static void capsule(painter_t *p,float cx,float cy,float w,float h,float angle,uint32_t c) {
    float x[32],y[32],r=fminf(w,h)/2,straight=fmaxf(0,h/2-r),ca=cosf(angle),sa=sinf(angle);
    for(int i=0;i<32;i++){float a=2*PI*i/32,px=r*cosf(a),py=r*sinf(a)+(sinf(a)<0?-straight:straight);x[i]=cx+px*ca-py*sa;y[i]=cy+px*sa+py*ca;}
    polygon(p,x,y,32,c);
}
static uint32_t blend(uint32_t c,uint32_t bg,float alpha) {
    uint32_t out=0;
    for(int shift=0;shift<=16;shift+=8) out|=(uint32_t)(((c>>shift)&255)*alpha+((bg>>shift)&255)*(1-alpha))<<shift;
    return out;
}
typedef struct {float x,y,z;} point3_t;
static point3_t horizon(point3_t a,point3_t b) {
    float t=a.z/(a.z-b.z),x=a.x+t*(b.x-a.x),y=a.y+t*(b.y-a.y);
    float length=hypotf(x,y);
    return (point3_t){x/length,y/length,0};
}
static void eye_point(float *x,float *y,int *count,float px,float py) {
    if(*count<EAI_EYE_CAPACITY){x[*count]=px;y[(*count)++]=py;}
}
int eai_project_eye(ea_interaction_state_t state,int eye,float blink,
                    float yaw,float pitch,float roll,
                    float x[EAI_EYE_CAPACITY],float y[EAI_EYE_CAPACITY]) {
    enum { SAMPLES=64 };
    if(!x || !y || !isfinite(yaw) || !isfinite(pitch) || !isfinite(roll))return 0;
    float width=.26f,height=.57f*clamp(blink,.035f,1);
    if(state==EA_FACE_SLEEP || (state==EA_FACE_WINK && !eye)){height=.045f;width=.28f;}
    if(state==EA_FACE_SURPRISE)height=.66f;
    if(state==EA_FACE_ERROR)height*=.65f;
    bool smile=state==EA_FACE_HAPPY||state==EA_FACE_DONE||state==EA_FACE_LOVE;
    float tilt=state==EA_FACE_ERROR?(eye?-.18f:.18f):0,ct=cosf(tilt),st=sinf(tilt);
    float az=eye?.32f:-.32f,ac=cosf(az),as=sinf(az),cap=fminf(width,height)/2;
    float cy=cosf(yaw),sy=sinf(yaw),cp=cosf(pitch),sp=sinf(pitch),cr=cosf(roll),sr=sinf(roll);
    point3_t p[SAMPLES];int start=-1;
    for(int i=0;i<SAMPLES;i++) {
        float px,py;
        if(smile){float a=i<SAMPLES/2?PI+PI*i/(SAMPLES/2-1):2*PI-PI*(i-SAMPLES/2)/(SAMPLES/2-1);px=cosf(a)*(i<SAMPLES/2?.16f:.09f);py=sinf(a)*(i<SAMPLES/2?.16f:.09f);}
        else {
            /* Sample the long edges too: projecting only their endpoints
               leaves straight screen-space sides and loses the surface bend. */
            int segment=i/(SAMPLES/4);float f=(i%(SAMPLES/4))/(float)(SAMPLES/4),length=fabsf(height-width);
            if(segment==0){float a=PI*f;px=cap*cosf(a);py=length/2+cap*sinf(a);}
            else if(segment==1){px=-cap;py=length*(.5f-f);}
            else if(segment==2){float a=PI+PI*f;px=cap*cosf(a);py=-length/2+cap*sinf(a);}
            else {px=cap;py=length*(f-.5f);}
            if(width>height){float old=px;px=py;py=-old;}
        }
        float tx=px*ct-py*st,ty=px*st+py*ct,q=hypotf(tx,ty),sinc=q>1e-6f?sinf(q)/q:1;
        /* Exponential map of a fixed tangent-plane decal onto its sphere.
           Rotate the whole surface: Rz(roll) * Ry(yaw) * Rx(pitch). */
        float vx=cosf(q)*as+sinc*tx*ac,vy=sinc*ty,vz=cosf(q)*ac-sinc*tx*as;
        float py1=vy*cp-vz*sp,pz1=vy*sp+vz*cp,px2=vx*cy+pz1*sy,pz2=-vx*sy+pz1*cy;
        p[i]=(point3_t){px2*cr-py1*sr,px2*sr+py1*cr,pz2};
        if(p[i].z>0 && start<0)start=i;
    }
    if(start<0)return 0;
    int count=0;point3_t exit={0};bool exited=false;
    eye_point(x,y,&count,p[start].x,p[start].y);
    for(int k=0;k<SAMPLES;k++) {
        point3_t a=p[(start+k)%SAMPLES],b=p[(start+k+1)%SAMPLES];
        if(a.z>0 && b.z>0)eye_point(x,y,&count,b.x,b.y);
        else if(a.z>0){exit=horizon(a,b);exited=true;eye_point(x,y,&count,exit.x,exit.y);}
        else if(b.z>0){
            point3_t enter=horizon(a,b);
            if(exited){
                float from=atan2f(exit.y,exit.x),delta=atan2f(exit.x*enter.y-exit.y*enter.x,exit.x*enter.x+exit.y*enter.y);
                int steps=(int)ceilf(fabsf(delta)/.01f);if(steps<1)steps=1;
                for(int j=1;j<steps;j++){float angle=from+delta*j/steps;eye_point(x,y,&count,cosf(angle),sinf(angle));}
            }
            eye_point(x,y,&count,enter.x,enter.y);eye_point(x,y,&count,b.x,b.y);exited=false;
        }
    }
    return count;
}
typedef struct { float cy,sy,cp,sp; } rotation_t;
static point3_t rotate_surface(point3_t p,rotation_t q) {
    float y=p.y*q.cp-p.z*q.sp,z=p.y*q.sp+p.z*q.cp;
    return (point3_t){p.x*q.cy+z*q.sy,y,-p.x*q.sy+z*q.cy};
}
/* Sample the SAME triangles used for the body. This keeps decals attached
   at bevels and avoids a smooth analytic eye floating above a coarse mesh. */
static bool surface_point(unsigned character,const float *radii,float x,float y,point3_t *p,point3_t *normal) {
    float angle=atan2f(y,x)+PI;
    int i=(int)(angle*(N/(2*PI)));if(i>=N)i=0;int j=(i+1)%N;
    float ax=shape_radii[character][i]*surface_directions[i][0],ay=shape_radii[character][i]*surface_directions[i][1];
    float bx=shape_radii[character][j]*surface_directions[j][0],by=shape_radii[character][j]*surface_directions[j][1];
    float determinant=ax*by-ay*bx,a=(x*by-y*bx)/determinant,b=(ax*y-ay*x)/determinant,rho=a+b;
    if(a<-.00001f || b<-.00001f || rho>1)return false;
    int ring=1;while(ring<8 && rho>surface_rings[ring])ring++;
    if(radii){ax=radii[i]*surface_directions[i][0];ay=radii[i]*surface_directions[i][1];bx=radii[j]*surface_directions[j][0];by=radii[j]*surface_directions[j][1];}
    float lo=surface_rings[ring-1],hi=surface_rings[ring],t=(rho-lo)/(hi-lo);
    point3_t v0={lo*ax,lo*ay,surface_depths[character][ring-1][i]/16384.f},v1,v2;
    float w1,w2;
    if(b/hi<=t || !lo){
        v1=(point3_t){hi*ax,hi*ay,surface_depths[character][ring][i]/16384.f};
        v2=(point3_t){hi*bx,hi*by,surface_depths[character][ring][j]/16384.f};
        w2=b/hi;w1=t-w2;
    }else{
        v1=(point3_t){hi*bx,hi*by,surface_depths[character][ring][j]/16384.f};
        v2=(point3_t){lo*bx,lo*by,surface_depths[character][ring-1][j]/16384.f};
        w1=t;w2=(b-hi*t)/lo;
    }
    *p=(point3_t){v0.x+(v1.x-v0.x)*w1+(v2.x-v0.x)*w2,v0.y+(v1.y-v0.y)*w1+(v2.y-v0.y)*w2,v0.z+(v1.z-v0.z)*w1+(v2.z-v0.z)*w2};
    float dx1=v1.x-v0.x,dy1=v1.y-v0.y,dz1=v1.z-v0.z,dx2=v2.x-v0.x,dy2=v2.y-v0.y,dz2=v2.z-v0.z;
    *normal=(point3_t){dy1*dz2-dz1*dy2,dz1*dx2-dx1*dz2,dx1*dy2-dy1*dx2};return true;
}
/* Cone UV: longitude follows circular cross-sections and narrows towards
   the apex. The final position and visibility still use the actual mesh. */
static float cone_radius_at(float y) {
    float at=clamp((y-cone_min_y)*64/(cone_max_y-cone_min_y),0,64);
    int i=(int)at;if(i>=64)return cone_profile[64];
    return cone_profile[i]+(cone_profile[i+1]-cone_profile[i])*(at-i);
}
static int project_surface_eye(unsigned character,const float *radii,ea_interaction_state_t state,int eye,float blink,
                    float yaw,float pitch,float x[EAI_EYE_CAPACITY],float y[EAI_EYE_CAPACITY]) {
    if(character>=EA_INTERACTION_SHAPES || !x || !y || !isfinite(yaw) || !isfinite(pitch))return 0;
    if(!character)return eai_project_eye(state,eye,blink,yaw,pitch,face_profiles[0][3],x,y);
    enum { SAMPLES=64 };
    float width=.26f,height=.57f*clamp(blink,.035f,1);
    if(state==EA_FACE_SLEEP || (state==EA_FACE_WINK && !eye)){height=.045f;width=.28f;}
    if(state==EA_FACE_SURPRISE)height=.66f;
    if(state==EA_FACE_ERROR)height*=.65f;
    bool smile=state==EA_FACE_HAPPY||state==EA_FACE_DONE||state==EA_FACE_LOVE;
    const float *face=surface_faces[character];
    float tilt=face[3]+(state==EA_FACE_ERROR?(eye?-.18f:.18f):0),ct=cosf(tilt),st=sinf(tilt),cap=fminf(width,height)/2;
    rotation_t q={cosf(yaw),sinf(yaw),cosf(pitch),sinf(pitch)};
    point3_t points[SAMPLES];float facing[SAMPLES];int count=0;
    for(int i=0;i<SAMPLES;i++) {
        float px,py;
        if(smile){float a=i<SAMPLES/2?PI+PI*i/(SAMPLES/2-1):2*PI-PI*(i-SAMPLES/2)/(SAMPLES/2-1);px=cosf(a)*(i<SAMPLES/2?.16f:.09f);py=sinf(a)*(i<SAMPLES/2?.16f:.09f);}
        else {
            int segment=i/(SAMPLES/4);float f=(i%(SAMPLES/4))/(float)(SAMPLES/4),length=fabsf(height-width);
            if(segment==0){float a=PI*f;px=cap*cosf(a);py=length/2+cap*sinf(a);}
            else if(segment==1){px=-cap;py=length*(.5f-f);}
            else if(segment==2){float a=PI+PI*f;px=cap*cosf(a);py=-length/2+cap*sinf(a);}
            else{px=cap;py=length*(f-.5f);}
            if(width>height){float old=px;px=py;py=-old;}
        }
        px+=eye?.32f:-.32f;
        float u=face[1]+face[0]*(px*ct-py*st),v=face[2]+face[0]*(px*st+py*ct);
        if(character==8)u=cone_radius_at(v)*sinf(u/cone_radius_at(face[2]));
        point3_t p={u,v,0},normal={0,0,-1};
        bool valid=surface_point(character,radii,u,v,&p,&normal);
        points[i]=rotate_surface(p,q);facing[i]=valid?rotate_surface(normal,q).z:-1;
    }
    /* Clip against the real surface normals, not a unit-sphere horizon. */
    for(int i=0,j=SAMPLES-1;i<SAMPLES;j=i++) {
        if((facing[i]>0)!=(facing[j]>0)) {
            float t=facing[j]/(facing[j]-facing[i]);
            eye_point(x,y,&count,points[j].x+t*(points[i].x-points[j].x),points[j].y+t*(points[i].y-points[j].y));
        }
        if(facing[i]>0)eye_point(x,y,&count,points[i].x,points[i].y);
    }
    return count;
}
int eai_project_surface_eye(unsigned character,ea_interaction_state_t state,int eye,float blink,
                    float yaw,float pitch,float x[EAI_EYE_CAPACITY],float y[EAI_EYE_CAPACITY]) {
    return project_surface_eye(character,NULL,state,eye,blink,yaw,pitch,x,y);
}
static void solid_triangle(painter_t *p,point3_t a,point3_t b,point3_t c,uint32_t color) {
    float x[]={a.x,b.x,c.x},y[]={a.y,b.y,c.y};polygon(p,x,y,3,color);
}
static void solid_body(painter_t *p,const ea_interaction_t *u,rotation_t q,float r,float sx,float sy,
                       float ca,float sa,float cx,float cy,uint32_t color) {
    point3_t previous[N],current[N];
    for(int side=-1;side<=1;side+=2)for(int ring=0;ring<9;ring++) {
        for(int i=0;i<N;i++) {
            float rad=u->shape[i]*surface_rings[ring];
            point3_t v=rotate_surface((point3_t){rad*surface_directions[i][0],rad*surface_directions[i][1],side*surface_depths[u->config.character][ring][i]/16384.f},q);
            float x=v.x*r*sx,y=v.y*r*sy;
            current[i]=(point3_t){cx+x*ca-y*sa,cy+x*sa+y*ca,0};
        }
        if(ring)for(int i=0;i<N;i++) {
            int j=(i+1)%N;
            solid_triangle(p,previous[i],current[i],current[j],color);
            if(ring>1)solid_triangle(p,previous[i],current[j],previous[j],color);
        }
        memcpy(previous,current,sizeof(previous));
    }
}
void ea_interaction_render(const ea_interaction_t *u,int size,uint32_t bg,ea_interaction_span_t fn,void *ctx) {
    if (!u || !fn || !u->config.enabled || !u->visible || size<16 || size>1024) return;
    painter_t p={size,fn,ctx};
    bool animated=ea_interaction_animated(u); float t=animated?u->time:0;
    float level=animated?u->level:0;
    uint32_t color=u->monochrome?0:colors[u->config.color];
    for(int row=0;row<size;row++) span(&p,0,size-1,row,bg);
    float mid=size*.5f;
    if(u->config.theme==EA_THEME_RIPPLE) {
        for(int k=4;k>=0;k--) {
            float x[N],y[N],r=size*(.19f+k*.038f+level*.058f);
            for(int i=0;i<N;i++){float a=2*PI*i/N;float wave=animated?(sinf(3*a+t*1.8f+k*.7f)+.45f*sinf(5*a-t*1.3f))*(.008f+level*.026f)*size:0;x[i]=mid+cosf(a)*(r+wave);y[i]=mid+sinf(a)*(r+wave);}
            polygon(&p,x,y,N,u->monochrome?color:blend(color,bg,.24f+(4-k)*.15f));
            float width=u->monochrome?1.3f:size*(.006f+level*.009f);
            float inner=fmaxf(0,1-width/r);
            for(int i=0;i<N;i++){x[i]=mid+(x[i]-mid)*inner;y[i]=mid+(y[i]-mid)*inner;}
            polygon(&p,x,y,N,bg);
        }
        for(int k=0;k<5;k++){float h=size*(.02f+(u->state==EA_FACE_SPEAK||u->state==EA_FACE_LISTEN?.04f:0)+level*(.10f+.07f*cosf(k*1.7f+t*9)));capsule(&p,mid+(k-2)*size*.035f,mid,size*.017f,h,0,color);}
        return;
    }
    /* Runtime is upright. Keep scale fixed across all expressions and input;
       turning is a rigid 3D rotation, never a squash/stretch approximation. */
    const float roll=0,sx=1,sy=1;
    float bob=animated?sinf(t*1.7f)*size*.009f:0;
    if(u->state==EA_FACE_HAPPY||u->state==EA_FACE_DONE){bob-=animated?fabsf(sinf(u->state_time*3))*size*.045f:size*.02f;}
    if(u->state==EA_FACE_SLEEP)bob+=size*.025f;
    float hx=animated?u->head_x:0,hy=animated?u->head_y:0;
    float ca=cosf(roll),sa=sinf(roll),r=size*.31f,x[N*2],y[N*2];
    float center_x=mid+r*.11f*hx,center_y=mid+bob+r*.08f*hy;
    float yaw,pitch;rest_pose(u->state,&yaw,&pitch);
    if(animated){yaw=u->gaze_x;pitch=u->gaze_y;}
    if(!u->config.character){
        for(int i=0;i<N*2;i++){float a=-PI+PI*i/N,rad=body_radius(u,i),px=cosf(a)*rad*r*sx,py=sinf(a)*rad*r*sy;x[i]=center_x+px*ca-py*sa;y[i]=center_y+px*sa+py*ca;}
        polygon(&p,x,y,N*2,color);
    }else{
        rotation_t q={cosf(yaw),sinf(yaw),cosf(pitch),sinf(pitch)};
        solid_body(&p,u,q,r,sx,sy,ca,sa,center_x,center_y,color);
    }
    float cycle=fmodf(t,5.8f),blink=animated&&cycle>4.7f&&cycle<4.96f?fabsf((cycle-4.83f)/.13f):1;
    for(int eye=0;eye<2;eye++) {
        int count=project_surface_eye(u->config.character,u->shape,u->state,eye,blink,yaw,pitch,x,y);
        if(count<3)continue;
        for(int i=0;i<count;i++) {
            float px=r*sx*x[i],py=r*sy*y[i];
            x[i]=center_x+px*ca-py*sa;y[i]=center_y+px*sa+py*ca;
        }
        polygon(&p,x,y,count,bg);
    }
    if(u->state==EA_FACE_THINK || u->state==EA_FACE_ASK) for(int k=0;k<3;k++) capsule(&p,center_x+(k-1)*size*.04f,size*.88f+r*.08f*hy+(animated?sinf(t*4-k)*size*.006f:0),size*.014f,size*.014f,0,color);
}
