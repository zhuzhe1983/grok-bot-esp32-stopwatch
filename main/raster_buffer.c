#include "raster_buffer.h"
#include <string.h>
void demo_raster_begin(demo_raster_t *r,uint8_t *pixels,size_t stride,int width,int height,uint32_t background) {
    *r=(demo_raster_t){.pixels=pixels,.stride=stride,.width=width,.height=height,
        .left=width,.top=height,.right=-1,.bottom=-1,.background=background,.foreground=background};
}
void demo_raster_span(void *context,int x1,int x2,int y,uint32_t color) {
    demo_raster_t *r=context;
    if(y<0 || y>=r->height || x2<0 || x1>=r->width || x1>x2)return;
    if(x1<0)x1=0;
    if(x2>=r->width)x2=r->width-1;
    int ink=color!=r->background;
    if(ink) {
        r->foreground=color;
        if(x1<r->left)r->left=x1;
        if(x2>r->right)r->right=x2;
        if(y<r->top)r->top=y;
        if(y>r->bottom)r->bottom=y;
    }
    uint8_t *p=r->pixels+y*r->stride;
    int a=x1>>3,b=x2>>3;
    uint8_t left=(uint8_t)(0xffu>>(x1&7)),right=(uint8_t)(0xffu<<(7-(x2&7)));
    if(a==b) {uint8_t mask=left&right;p[a]=ink?(p[a]|mask):(p[a]&~mask);return;}
    p[a]=ink?(p[a]|left):(p[a]&~left);
    if(b>a+1)memset(p+a+1,ink?255:0,(size_t)(b-a-1));
    p[b]=ink?(p[b]|right):(p[b]&~right);
}
