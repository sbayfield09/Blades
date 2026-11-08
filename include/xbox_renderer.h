#ifndef BLADES_XBOX_RENDERER_H
#define BLADES_XBOX_RENDERER_H
#include "blades.h"
typedef struct XboxRenderer XboxRenderer;
typedef struct {
    uint64_t cpu_us,upload_us,gpu_wait_us,gpu_us;
    unsigned builds,draws,bg_updates,resident_kb;
} XboxRenderStats;
XboxRenderer *xbox_renderer_open(void);
bool xbox_renderer_frame(XboxRenderer *renderer,const Dashboard *d,uint32_t address,XboxRenderStats *stats);
void xbox_renderer_close(XboxRenderer *renderer);
#endif
