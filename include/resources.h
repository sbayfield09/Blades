#ifndef BLADES_RESOURCES_H
#define BLADES_RESOURCES_H
#include "blades.h"
#include <stddef.h>
/* Optional local XUIZ archive. No stock archive or extracted image is embedded. */
bool resources_open(const char *path);
bool resources_open_memory(const unsigned char *bytes,size_t size);
void resources_close(void);
const unsigned char *resources_blob(const char *name,size_t *size);
unsigned resources_generation(void);
bool resources_draw(Canvas *c,const char *name,int x,int y,int w,int h);
bool resources_draw_opacity(Canvas *c,const char *name,int x,int y,int w,int h,unsigned opacity);
/* Fit the complete image inside a box without changing its aspect ratio. */
bool resources_draw_fit(Canvas *c,const char *name,int x,int y,int w,int h);
void canvas_image(Canvas *c,const unsigned char *rgba,int iw,int ih,int x,int y,int w,int h);
void canvas_image_opacity(Canvas *c,const unsigned char *rgba,int iw,int ih,int x,int y,int w,int h,unsigned opacity);
unsigned char *resource_png_decode(const unsigned char *bytes,int size,int *w,int *h);
unsigned char *resource_png_decode_bounded(const unsigned char *,int,int *,int *,int max_w,int max_h);
void resource_png_free(void *pixels);
#endif
