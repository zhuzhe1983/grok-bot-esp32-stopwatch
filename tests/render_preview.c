#include "demo_controller.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {SIDE=466,COLUMNS=4,ROWS=4,WIDTH=SIDE*COLUMNS,HEIGHT=SIDE*ROWS};
static unsigned char pixels[WIDTH*HEIGHT*3];
typedef struct {int x,y;} tile_t;
static void span(void *context,int x1,int x2,int y,uint32_t color) {
    tile_t *tile=context;
    assert(x1>=0 && x2<SIDE && y>=0 && y<SIDE);
    for (int x=x1;x<=x2;x++) {
        float dx=x-(SIDE-1)*.5f,dy=y-(SIDE-1)*.5f;
        uint32_t c=dx*dx+dy*dy<(SIDE*.5f-2)*(SIDE*.5f-2)?color:0x09090b;
        unsigned char *p=pixels+((tile->y+y)*WIDTH+tile->x+x)*3;
        p[0]=c>>16; p[1]=c>>8; p[2]=c;
    }
}
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    memset(pixels,9,sizeof(pixels));
    for (unsigned i=0;i<16;i++) {
        ea_interaction_config_t c=ea_interaction_default();
        c.enabled=true; c.character=i<13?i:6;
        c.color=i<13?0:(i==13?4:i==14?7:3);
        ea_interaction_t face; ea_interaction_init(&face,&c,false);
        ea_interaction_set_state(&face,i<13?EA_FACE_IDLE:i==13?EA_FACE_HAPPY:i==14?EA_FACE_WINK:EA_FACE_SURPRISE);
        if (i>=13) ea_interaction_look(&face,(i-14.f)*.35f,0,true);
        for (int j=0;j<30;j++) ea_interaction_step(&face,1.f/30);
        tile_t tile={(int)(i%COLUMNS)*SIDE,(int)(i/COLUMNS)*SIDE};
        ea_interaction_render(&face,SIDE,0x1f1f1f,span,&tile);
    }
    FILE *f=fopen(argv[1],"wb"); assert(f);
    fprintf(f,"P6\n%d %d\n255\n",WIDTH,HEIGHT);
    assert(fwrite(pixels,1,sizeof(pixels),f)==sizeof(pixels));
    fclose(f); return 0;
}
