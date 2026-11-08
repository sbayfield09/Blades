#ifndef BLADES_DRAW_LIST_H
#define BLADES_DRAW_LIST_H
#include "blades.h"
#include <stddef.h>
/* A texture's identity is independent of its allocation address. Revisions
   let the font atlas acquire new glyphs without leaving stale GPU texels. */
typedef enum { IMAGE_RGBA, IMAGE_MASK,IMAGE_YUV601,IMAGE_YUV709,IMAGE_YUV601_FULL,IMAGE_YUV709_FULL } ImageFormat;
static inline bool image_yuv(ImageFormat f){return f>=IMAGE_YUV601&&f<=IMAGE_YUV709_FULL;}
typedef struct {
    const unsigned char *pixels;
    unsigned id,revision;
    int width,height;
    ImageFormat format;
} DrawImage;
/* Scalar reference/SDL fallback; GPU renderers sample the same R8 atlas. */
void video_colour_constants(ImageFormat,float values[16]);
void canvas_yuv420(Canvas *,DrawImage image,int x,int y,int w,int h);
unsigned canvas_image_id(void);
typedef enum { DRAW_QUAD, DRAW_AMBIENT, DRAW_FADE_BEGIN, DRAW_FADE_END } DrawKind;
typedef struct {
    DrawKind kind;
    DrawImage image;
    int x,y,w,h,sx,sy,sw,sh;
    uint32_t color;
    float amount;
    bool rotate;
} DrawOp;
typedef struct CanvasOps {
    void (*append)(void *context,DrawOp op);
} CanvasOps;
typedef struct {
    DrawOp *ops;
    size_t count,capacity;
    bool failed,valid;
    Dashboard state;
    unsigned generation,builds;
} DrawList;
void canvas_emit(Canvas *c,DrawOp op);
void canvas_image_tagged(Canvas *c,DrawImage image,int x,int y,int w,int h);
void canvas_image_tagged_opacity(Canvas *c,DrawImage image,int x,int y,int w,int h,unsigned opacity);
void canvas_mask(Canvas *c,DrawImage image,int sx,int sy,int side,int x,int y,uint32_t color,bool rotate);
/* Returns true if the UI changed and the command list was rebuilt. Time is
   supplied at playback: an idle menu reuses its commands while the swirl moves. */
bool draw_list_update(DrawList *list,const Dashboard *d);
void draw_list_close(DrawList *list);
/* Reference playback exercises the same commands without an actual GPU. */
bool draw_list_software(const DrawList *list,Canvas *canvas,float seconds);
#endif
