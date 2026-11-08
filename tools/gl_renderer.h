#ifndef BLADES_GL_RENDERER_H
#define BLADES_GL_RENDERER_H
#include "blades.h"
typedef struct GlRenderer GlRenderer;
/* Headless desktop playback of the production command plan and shader source.
   It validates composition; it is not a measurement of Xbox GPU performance. */
GlRenderer *gl_renderer_open(void);
bool gl_renderer_frame(GlRenderer *r,const Dashboard *d,Canvas *readback);
void gl_renderer_close(GlRenderer *r);
#endif
