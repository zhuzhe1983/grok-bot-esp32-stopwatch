#include "agent_interaction.h"
#include "raster_buffer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Independent, unoptimized triangle rasterizer with the current pose/decal
   contract: protects silhouettes, eye occlusion and shape transitions. */
void reference_render(const ea_interaction_t *,int,uint32_t,ea_interaction_span_t,void *);
enum {SIDE=466,STRIDE=(SIDE+7)/8};
static unsigned char expected[SIDE*SIDE],actual[SIDE*SIDE],packed[SIDE*STRIDE];
static void pixels(void *ctx,int x1,int x2,int y,uint32_t color) {
    assert(x1>=0 && x2<SIDE && y>=0 && y<SIDE && x1<=x2);
    memset((unsigned char *)ctx+y*SIDE+x1,color!=0x1f1f1f,(size_t)(x2-x1+1));
}
static unsigned compare_frame(const ea_interaction_t *f) {
    reference_render(f,SIDE,0x1f1f1f,pixels,expected);
    ea_interaction_render(f,SIDE,0x1f1f1f,pixels,actual);
    demo_raster_t raster;memset(packed,0xa5,sizeof(packed));
    demo_raster_begin(&raster,packed,STRIDE,SIDE,SIDE,0x1f1f1f);
    ea_interaction_render(f,SIDE,0x1f1f1f,demo_raster_span,&raster);
    unsigned diff=0;
    for(int y=0;y<SIDE;y++)for(int x=0;x<SIDE;x++) {
        unsigned bit=(packed[y*STRIDE+x/8]>>(7-x%8))&1;
        assert(bit==actual[y*SIDE+x]);
        if(expected[y*SIDE+x]!=actual[y*SIDE+x])++diff;
    }
    return diff;
}
int main(void) {
    unsigned frames=0,total_diff=0,max_diff=0;
    const float views[][2]={{0,0},{-.9f,.45f},{.9f,-.45f},{1.45f,.8f},{-1.45f,-.8f}};
    for(unsigned shape=0;shape<13;shape++)for(unsigned state=0;state<12;state++)for(unsigned view=0;view<5;view++) {
        ea_interaction_config_t c=ea_interaction_default();c.enabled=true;c.character=shape;
        ea_interaction_t f;ea_interaction_init(&f,&c,false);ea_interaction_set_state(&f,state);
        f.time=state==2?4.82f:.7f;f.state_time=.5f;f.gaze_x=views[view][0];f.gaze_y=views[view][1];
        f.head_x=views[view][0]/1.5f;f.head_y=-views[view][1]/1.15f;
        unsigned diff=compare_frame(&f);
        if(diff>max_diff)max_diff=diff;
        total_diff+=diff;++frames;
        if(diff){fprintf(stderr,"shape=%u state=%u view=%u differing pixels=%u\n",shape,state,view,diff);return 1;}
    }
    /* All source/destination shape pairs at intermediate spring positions.
       Silhouette winding must also preserve the temporarily mixed geometry. */
    for(unsigned from=0;from<13;from++)for(unsigned to=0;to<13;to++)if(from!=to) {
        ea_interaction_config_t c=ea_interaction_default();c.enabled=true;c.character=from;
        ea_interaction_t f;ea_interaction_init(&f,&c,false);
        c.character=to;ea_interaction_configure(&f,&c);
        ea_interaction_look(&f,(from%3-1.f)*.8f,(to%3-1.f)*.6f,true);
        for(unsigned step=0;step<24;step++) {
            ea_interaction_step(&f,1.f/30);
            if(step!=2 && step!=8 && step!=20)continue;
            unsigned diff=compare_frame(&f);++frames;total_diff+=diff;
            if(diff>max_diff)max_diff=diff;
            if(diff){fprintf(stderr,"transition=%u->%u step=%u differing pixels=%u\n",from,to,step,diff);return 1;}
        }
    }
    printf("raster: %u full-resolution frames; packed pixels exact; reference total_diff=%u max_diff=%u\n",frames,total_diff,max_diff);
    return 0;
}
