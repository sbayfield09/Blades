#include "gpu_plan.h"
#include "theme.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
void gpu_plan_close(GpuPlan *p) {free(p->vertices);free(p->batches);free(p->images);memset(p,0,sizeof *p);}
static bool image_same(DrawImage a,DrawImage b) {
    return a.id==b.id&&a.pixels==b.pixels&&a.width==b.width&&a.height==b.height&&a.format==b.format;
}
static unsigned image_index(GpuPlan *p,DrawImage image) {
    for(size_t i=0;i<p->image_count;i++)if(image_same(p->images[i],image)) {
        if(image.revision>p->images[i].revision)p->images[i].revision=image.revision;
        return (unsigned)i;
    }
    p->images[p->image_count]=image;return (unsigned)p->image_count++;
}
static void quad(GpuPlan *p,const DrawOp *op) {
    float x[4]={(float)op->x,(float)(op->x+op->w),(float)(op->x+op->w),(float)op->x};
    float y[4]={(float)op->y,(float)op->y,(float)(op->y+op->h),(float)(op->y+op->h)};
    float u[4]={.5f,.5f,.5f,.5f},v[4]={.5f,.5f,.5f,.5f};
    if(op->kind==DRAW_AMBIENT) {
        u[0]=u[3]=(float)op->x/WIDTH;u[1]=u[2]=(float)(op->x+op->w)/WIDTH;
        v[0]=v[1]=0;v[2]=v[3]=1;
        y[0]=y[1]=0;y[2]=y[3]=HEIGHT;
    } else if(op->image.pixels) {
        if(op->rotate) {
            u[0]=u[1]=(float)op->sx/op->image.width;u[2]=u[3]=(float)(op->sx+op->sw)/op->image.width;
            v[0]=v[3]=(float)(op->sy+op->sh)/op->image.height;v[1]=v[2]=(float)op->sy/op->image.height;
        } else {
            float l=(op->sx+.5f-.5f*op->sw/op->w)/op->image.width;
            float t=(op->sy+.5f-.5f*op->sh/op->h)/op->image.height;
            u[0]=u[3]=l;u[1]=u[2]=l+(float)op->sw/op->image.width;
            v[0]=v[1]=t;v[2]=v[3]=t+(float)op->sh/op->image.height;
        }
    }
    const unsigned corners[]={0,1,2,0,2,3};
    uint32_t c=op->kind==DRAW_AMBIENT?0xffffffff:op->color;
    uint32_t packed=(c&0xff00ff00)|((c>>16)&255)|((c&255)<<16);
    for(unsigned n=0;n<6;n++) {
        unsigned i=corners[n];
        p->vertices[p->vertex_count++]=(GpuVertex){x[i]*(2.0f/WIDTH)-1,1-y[i]*(2.0f/HEIGHT),u[i],v[i],packed};
    }
}
bool gpu_plan_build(GpuPlan *p,const DrawList *list) {
    if(!list->valid||list->count>262144)return false;
    GpuPlan next={0};
    next.vertices=malloc((list->count*6+1)*sizeof *next.vertices);
    next.batches=malloc((list->count+1)*sizeof *next.batches);
    next.images=malloc((list->count+1)*sizeof *next.images);
    if(!next.vertices||!next.batches||!next.images){gpu_plan_close(&next);return false;}
    static const unsigned char white[]={255,255,255,255};
    image_index(&next,(DrawImage){white,0,0,1,1,IMAGE_RGBA});
    for(size_t i=0;i<list->count;i++) {
        const DrawOp *op=list->ops+i;unsigned first=(unsigned)next.vertex_count;
        unsigned image=op->kind==DRAW_QUAD&&op->image.pixels?image_index(&next,op->image):0;
        int box=op->kind==DRAW_QUAD&&op->image.format==IMAGE_RGBA&&op->image.pixels&&
            op->w==op->image.width&&op->h<op->image.height&&op->image.height<=2*op->h?op->h:0;
        if(op->kind==DRAW_QUAD||op->kind==DRAW_AMBIENT)quad(&next,op);
        if(op->kind==DRAW_AMBIENT)next.ambient_blades|=1u<<op->color;
        GpuBatch *last=next.batch_count?next.batches+next.batch_count-1:NULL;
        if(op->kind==DRAW_QUAD&&last&&last->kind==DRAW_QUAD&&last->image==image&&last->box_height==box&&last->count+6<=65532)last->count+=6;
        else next.batches[next.batch_count++]=(GpuBatch){op->kind,first,(unsigned)next.vertex_count-first,image,(int)op->color,op->amount,box};
    }
    gpu_plan_close(p);*p=next;return true;
}
unsigned gpu_ambient_frame(float seconds) {
    if(!isfinite(seconds)||seconds<0)seconds=0;
    return (unsigned)(fmodf(seconds,785.398163397f)*30+.0001f);
}
void gpu_ambient_constants(int blade,unsigned frame,float values[8]) {
    uint32_t c=theme_color(blade);
    float constants[8]={(float)((c>>16)&255),(float)((c>>8)&255),(float)(c&255),1,
        ((float)frame/30)*.8f,0,0,0};
    memcpy(values,constants,sizeof constants);
}
