#ifndef BLADES_GPU_PLAN_H
#define BLADES_GPU_PLAN_H
#include "draw_list.h"
typedef struct {float x,y,u,v;uint32_t color;} GpuVertex;
typedef struct {
    DrawKind kind;
    unsigned first,count,image;
    int blade;
    float amount;
    int box_height;
} GpuBatch;
typedef struct {
    GpuVertex *vertices;
    GpuBatch *batches;
    DrawImage *images;
    size_t vertex_count,batch_count,image_count;
    unsigned ambient_blades;
} GpuPlan;
bool gpu_plan_build(GpuPlan *plan,const DrawList *list);
void gpu_plan_close(GpuPlan *plan);
/* Quantization/period shared by software animation and both GPU backends. */
unsigned gpu_ambient_frame(float seconds);
void gpu_ambient_constants(int blade,unsigned frame,float values[8]);
#endif
