#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Grok faces use two colors per frame. Keep the full-resolution coverage in
   internal RAM; palette expansion happens only once, during display upload. */
typedef struct {
    uint8_t *pixels;
    size_t stride;
    int width,height,left,top,right,bottom;
    uint32_t background,foreground;
} demo_raster_t;
void demo_raster_begin(demo_raster_t *r,uint8_t *pixels,size_t stride,int width,int height,uint32_t background);
void demo_raster_span(void *context,int x1,int x2,int y,uint32_t color);
#ifdef __cplusplus
}
#endif
