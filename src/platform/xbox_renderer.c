#include "xbox_renderer.h"
#include "gpu_plan.h"
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/log.h"
#include "default_shaders.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

enum { CACHE_SIZE=512, CACHE_BYTES=96*1024*1024 };
typedef struct {DrawImage image;XsdkTexture texture,u,v;unsigned used;} CachedTexture;
struct XboxRenderer {
    DrawList list;GpuPlan plan;
    XsdkShader vertex,sprite,swirl,background,copy,cap,video;
    XsdkSamplerState point,linear;
    XsdkBlendState blend;
    CachedTexture cache[CACHE_SIZE];
    XsdkTexture ambient[BLADE_COUNT],base;
    bool ambient_valid[BLADE_COUNT];unsigned ambient_frame[BLADE_COUNT];
    volatile uint32_t *vertices,*full_quad,*clocks;
    unsigned vertex_bytes,resident,epoch,last_fence;
    bool timed,checked,background_checked,video_frame_logged,video_fence_pending;
};
static uint64_t micros(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000000+(unsigned long)t.tv_nsec/1000;
}
static bool load(XsdkShader *s,const XsdkBuiltShader *built) {
    NTSTATUS status=xsdk_shader_load(s,built->container,built->container_bytes);
    if(!NT_SUCCESS(status))xsdk_log_write("gpu: shader load failed %08X",(unsigned)status);
    return NT_SUCCESS(status);
}
static void write_memory(volatile uint32_t *dest,const void *src,unsigned bytes) {
    const unsigned char *data=src;
    for(unsigned i=0;i<bytes/4;i++){uint32_t word;memcpy(&word,data+i*4,4);dest[i]=word;}
    xsdk_gpu_invalidate(xsdk_gpu_address(dest),bytes);
}
static void shaders(XboxRenderer *r,const XsdkShader *fragment,const XsdkBuiltShader *built) {
    xsdk_gpu_set_shaders(&r->vertex,fragment);
    xsdk_built_shader_set_literals(&ui_vert);xsdk_built_shader_set_literals(built);
}
static void target(unsigned w,unsigned h) {
    xsdk_gpu_set_target(w,h);
    /* Xenos' D3D pixel centres are whole coordinates. This half-pixel viewport
       shift makes the shared plan sample at the same centres as OpenGL. */
    xsdk_gpu_set_viewport(-.5f,-.5f,(float)w,(float)h,0,1);
    xsdk_gpu_set_scissor(0,0,w,h);
}
static void full(XboxRenderer *r) {
    xsdk_gpu_set_vertices(0,xsdk_gpu_address(r->full_quad),6*sizeof(GpuVertex));
    xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST,6);
}
static bool texture_create(XsdkTexture *t,unsigned w,unsigned h) {
    NTSTATUS status=xsdk_texture_create(t,XSDK_FORMAT_B8G8R8A8,w,h,1,true);
    if(!NT_SUCCESS(status)){xsdk_log_write("gpu: render texture %ux%u failed %08X",w,h,(unsigned)status);return false;}
    xsdk_texture_base_at_corner(t);return true;
}
static bool same(DrawImage a,DrawImage b) {
    return a.id==b.id&&a.pixels==b.pixels&&a.width==b.width&&a.height==b.height&&a.format==b.format;
}
static CachedTexture *find(XboxRenderer *r,DrawImage image) {
    for(unsigned i=0;i<CACHE_SIZE;i++)if(r->cache[i].texture.texels&&same(r->cache[i].image,image))return r->cache+i;
    return NULL;
}
static unsigned cache_bytes(const CachedTexture *c){return c->texture.size_bytes+c->u.size_bytes+c->v.size_bytes;}
static void cache_free(CachedTexture *c){
    if(c->texture.texels)xsdk_texture_free(&c->texture);
    if(c->u.texels)xsdk_texture_free(&c->u);
    if(c->v.texels)xsdk_texture_free(&c->v);
    memset(c,0,sizeof *c);
}
static bool upload(XboxRenderer *r,DrawImage image) {
    CachedTexture *c=find(r,image);bool yuv=image_yuv(image.format);
    if(c&&c->image.revision==image.revision){c->used=r->epoch;return true;}
    if(!c) {
        for(unsigned i=0;i<CACHE_SIZE;i++)if(!r->cache[i].texture.texels){c=r->cache+i;break;}
        if(!c){xsdk_log_write("gpu: texture cache exhausted");return false;}
        uint32_t format=image.format==IMAGE_RGBA?XSDK_FORMAT_R8G8B8A8:XSDK_FORMAT_R8;
        NTSTATUS status=xsdk_texture_create(&c->texture,format,(unsigned)image.width,(unsigned)image.height,1,!yuv);
        if(yuv&&NT_SUCCESS(status))status=xsdk_texture_create(&c->u,XSDK_FORMAT_R8,(unsigned)image.width/2,(unsigned)image.height/2,1,false);
        if(yuv&&NT_SUCCESS(status))status=xsdk_texture_create(&c->v,XSDK_FORMAT_R8,(unsigned)image.width/2,(unsigned)image.height/2,1,false);
        if(!NT_SUCCESS(status)){xsdk_log_write("gpu: asset texture failed %08X",(unsigned)status);cache_free(c);return false;}
        r->resident+=cache_bytes(c);
        if(image.format==IMAGE_MASK)xsdk_texture_swizzle(&c->texture,(uint32_t[]){
            XSDK_CHANNEL_ONE,XSDK_CHANNEL_ONE,XSDK_CHANNEL_ONE,XSDK_CHANNEL_RED});
    }
    xsdk_texture_upload(&c->texture,0,image.pixels,(unsigned)image.width*(image.format==IMAGE_RGBA?4:1));
    xsdk_gpu_invalidate(c->texture.address,c->texture.size_bytes);
    if(yuv) {
        const unsigned char *chroma=image.pixels+(size_t)image.width*image.height;
        // Source slots retain strided U/V rows; three linear GPU planes avoid
        // atlas boundaries and mirror the standalone player's sampler layout.
        xsdk_texture_upload(&c->u,0,chroma,(unsigned)image.width);
        xsdk_texture_upload(&c->v,0,chroma+image.width/2,(unsigned)image.width);
        xsdk_gpu_invalidate(c->u.address,c->u.size_bytes);xsdk_gpu_invalidate(c->v.address,c->v.size_bytes);
    }
    c->image=image;c->used=r->epoch;return true;
}
static bool prepare(XboxRenderer *r,const Dashboard *d,XboxRenderStats *stats) {
    uint64_t start=micros();unsigned generation=r->list.generation;bool rebuilt=draw_list_update(&r->list,d);
    if(r->list.failed)return false;
    if(rebuilt) {
        if(generation!=r->list.generation) {
            for(unsigned i=0;i<CACHE_SIZE;i++)if(r->cache[i].texture.texels)cache_free(r->cache+i);
            memset(r->cache,0,sizeof r->cache);r->resident=0;
        }
        if(!gpu_plan_build(&r->plan,&r->list))return false;
        unsigned bytes=(unsigned)(r->plan.vertex_count*sizeof(GpuVertex));
        if(bytes>r->vertex_bytes) {
            xsdk_gpu_free(r->vertices);r->vertices=xsdk_gpu_alloc(bytes);
            if(!r->vertices){r->vertex_bytes=0;return false;}
            r->vertex_bytes=bytes;
        }
        write_memory(r->vertices,r->plan.vertices,bytes);stats->builds++;
        r->epoch++;
        for(size_t i=0;i<r->plan.image_count;i++){CachedTexture *c=find(r,r->plan.images[i]);if(c)c->used=r->epoch;}
        /* Evict only textures absent from this list, after the preceding GPU
           fence. Live image/vertex memory is never overwritten in flight. */
        for(unsigned i=0;i<CACHE_SIZE;i++)if(!d->startup_active&&r->cache[i].texture.texels&&r->cache[i].used!=r->epoch&&
            (r->resident>CACHE_BYTES||r->cache[i].used+16<r->epoch)) {
            r->resident-=cache_bytes(r->cache+i);cache_free(r->cache+i);
        }
        uint64_t upload_start=micros();
        for(size_t i=0;i<r->plan.image_count;i++)if(!upload(r,r->plan.images[i]))return false;
        stats->upload_us+=micros()-upload_start;
    }
    stats->cpu_us+=micros()-start;stats->resident_kb=r->resident/1024;
    return true;
}
XboxRenderer *xbox_renderer_open(void) {
    XboxRenderer *r=calloc(1,sizeof *r);if(!r)return NULL;
    if(!load(&r->vertex,&ui_vert)||!load(&r->sprite,&ui_frag)||!load(&r->swirl,&swirl_frag)||
        !load(&r->background,&background_frag)||!load(&r->copy,&copy_frag)||!load(&r->cap,&cap_frag)||!load(&r->video,&video_frag))goto fail;
    static const XsdkVertexElement layout[]={
        {.usage=XSDK_USAGE_POSITION,.format=XSDK_VERTEX_FLOAT2,.offset_bytes=0},
        {.usage=XSDK_USAGE_COLOR,.format=XSDK_VERTEX_UBYTE4N,.offset_bytes=16},
        {.usage=XSDK_USAGE_TEXCOORD,.format=XSDK_VERTEX_FLOAT2,.offset_bytes=8}};
    if(!NT_SUCCESS(xsdk_shader_set_vertex_layout(&r->vertex,layout,3,(uint32_t[]){sizeof(GpuVertex)})))goto fail;
    XsdkSampler sampler={.mag_filter=XSDK_FILTER_POINT,.min_filter=XSDK_FILTER_POINT,
        .address_u=XSDK_ADDRESS_CLAMP,.address_v=XSDK_ADDRESS_CLAMP};
    xsdk_texture_sampler_state(&r->point,&sampler);sampler.mag_filter=sampler.min_filter=XSDK_FILTER_LINEAR;
    xsdk_texture_sampler_state(&r->linear,&sampler);
    XsdkBlend blend={.color_source=XSDK_BLEND_SRC_ALPHA,.color_dest=XSDK_BLEND_ONE_MINUS_SRC_ALPHA,
        .alpha_source=XSDK_BLEND_ONE,.alpha_dest=XSDK_BLEND_ONE_MINUS_SRC_ALPHA,.write_mask=XSDK_WRITE_ALL};
    xsdk_gpu_blend_state(&r->blend,&blend);
    r->full_quad=xsdk_gpu_alloc(6*sizeof(GpuVertex));r->clocks=xsdk_gpu_alloc(6*4);
    if(!r->full_quad||!r->clocks)goto fail;
    const GpuVertex quad[]={
        {-1,1,0,0,0xffffffff},{1,1,1,0,0xffffffff},{1,-1,1,1,0xffffffff},
        {-1,1,0,0,0xffffffff},{1,-1,1,1,0xffffffff},{-1,-1,0,1,0xffffffff}};
    write_memory(r->full_quad,quad,sizeof quad);
    /* Decode/upload the root and launch artwork before starting the animation
       clock. This keeps font/PNG decoding out of the one-second opening. */
    XboxRenderStats warm_stats={0};Dashboard warm;dashboard_init(&warm);
    if(!prepare(r,&warm,&warm_stats))goto fail;
    warm.selected[BLADE_GAMES]=4;warm.highlight=4;warm.disc.state=DISC_READING;
    for(unsigned i=0;i<16;i++) {
        warm.disc_elapsed=i/27.2f+.001f;
        if(!prepare(r,&warm,&warm_stats))goto fail;
    }
    dashboard_begin_startup(&warm);warm.startup_elapsed=.4f;
    if(!prepare(r,&warm,&warm_stats))goto fail;
    warm.startup_elapsed=0;
    if(!prepare(r,&warm,&warm_stats))goto fail;
    if(!texture_create(r->ambient+BLADE_GAMES,384,216)||!texture_create(&r->base,WIDTH,HEIGHT))goto fail;
    r->last_fence=xsdk_gpu_fence();if(!NT_SUCCESS(xsdk_gpu_wait(r->last_fence,5000)))goto fail;
    xsdk_log_write("startup: root, runner and disc-reading artwork prewarmed");
    xsdk_log_write("gpu: retained textured quads; 384x216 swirl shader at 30 Hz; GPU scaling and alpha blending");
    return r;
fail:
    xsdk_log_write("gpu: renderer initialization failed");xbox_renderer_close(r);return NULL;
}
static void copy_base(XboxRenderer *r,float alpha,bool blend) {
    shaders(r,&r->copy,&copy_frag);
    float constants[]={alpha,0,0,0};xsdk_gpu_set_pixel_constants(COPY_FRAG_PUSH,constants,1);
    if(blend)xsdk_gpu_set_blend(&r->blend);
    xsdk_gpu_set_texture(COPY_FRAG_SAMPLER_0_0,&r->base,&r->point);full(r);
}
static uint32_t source_expected(int x,int y,float phase,int blade) {
    float constants[8];gpu_ambient_constants(blade,0,constants);
    double u=((double)x/383-.51)*2,v=((double)y/215-.49)*1.7;
    double a=u+.14*sin(v*3+phase),b=v+.11*cos(phase*.73);
    double radius=sqrt(a*a*.83+b*b);
    double band=(radius-.65-.12*sin(((double)x/383-.51)*6.2+phase*.6))*4.2;
    double ribbon=exp(-band*band),wave=sin(u*3.4)*cos(v*2.8+phase*.8)+cos(u*3.4)*sin(v*2.8+phase*.8);
    double light=.69+ribbon*.57+wave*.08+sin(radius*9-phase)*.045;
    const double extra[]={18,25,5};uint32_t color=0xff000000;
    for(unsigned i=0;i<3;i++) {
        double value=constants[i]*light+ribbon*extra[i];
        color|=(uint32_t)(value<0?0:value>255?255:value)<<(16-8*i);
    }
    return color;
}
static unsigned difference(uint32_t a,uint32_t b) {
    unsigned peak=0;
    for(unsigned s=0;s<=16;s+=8){unsigned d=(unsigned)abs((int)((a>>s)&255)-(int)((b>>s)&255));if(d>peak)peak=d;}
    return peak;
}
static uint32_t mix(uint32_t a,uint32_t b,unsigned weight) {
    uint32_t color=0xff000000;
    for(unsigned s=0;s<=16;s+=8)color|=((((a>>s)&255)*(256-weight)+((b>>s)&255)*weight)>>8)<<s;
    return color;
}
static void self_check(XboxRenderer *r,const Dashboard *d,uint32_t address) {
    if(!r->checked) {
    int blade=d->blade;unsigned worst=0;
    uint32_t row[384];float phase=((float)r->ambient_frame[blade]/30)*.8f;
    const unsigned ys[]={0,53,108,215},xs[]={0,66,192,322,383};
    for(unsigned y=0;y<4;y++) {
        xsdk_texture_download_rows(r->ambient+blade,0,ys[y],1,row,sizeof row);
        for(unsigned x=0;x<5;x++) {
            uint32_t actual=__builtin_bswap32(row[xs[x]]),expected=source_expected((int)xs[x],(int)ys[y],phase,blade);
            unsigned error=difference(actual,expected);if(error>worst)worst=error;
        }
    }
    xsdk_log_write("gpu-check: %s swirl_source samples=20 max_rgb_error=%u",worst<=2?"PASS":"FAIL",worst);
    r->checked=true;
    }
    /* Initial Games root has unobstructed background here. Compare the actual
       front buffer with samples from the actual source texture: this also
       checks resolve byte order, pixel centres and the 720p filtering pass. */
    if(!r->background_checked&&!d->startup_active&&d->blade==BLADE_GAMES&&!d->depth&&d->transition==1&&!d->dialog&&!d->exit_menu) {
        int blade=d->blade;
        const unsigned x=970,y=400;
        unsigned px=x*383*256/1279,py=y*215*256/719;
        uint32_t rows[2][384];
        xsdk_texture_download_rows(r->ambient+blade,0,py>>8,2,rows,sizeof rows[0]);
        uint32_t top=mix(__builtin_bswap32(rows[0][px>>8]),__builtin_bswap32(rows[0][(px>>8)+1]),px&255);
        uint32_t bottom=mix(__builtin_bswap32(rows[1][px>>8]),__builtin_bswap32(rows[1][(px>>8)+1]),px&255);
        uint32_t expected=mix(top,bottom,py&255),pixels[WIDTH];XsdkTexture front;
        if(NT_SUCCESS(xsdk_texture_init(&front,XSDK_FORMAT_B8G8R8A8,WIDTH,HEIGHT,1,true))) {
            xsdk_texture_attach(&front,xsdk_present_pixels(address));
            xsdk_texture_download_rows(&front,0,y,1,pixels,sizeof pixels);
            unsigned error=difference(__builtin_bswap32(pixels[x]),expected);
            xsdk_log_write("gpu-check: %s final_background max_rgb_error=%u",error<=2?"PASS":"FAIL",error);
            r->background_checked=true;
        }
    }
}
bool xbox_renderer_frame(XboxRenderer *r,const Dashboard *d,uint32_t address,XboxRenderStats *stats) {
    memset(stats,0,sizeof *stats);uint64_t start=micros();
    if(r->last_fence&&!NT_SUCCESS(xsdk_gpu_wait(r->last_fence,5000))) {
        xsdk_log_write("gpu: prior frame fence timed out");return false;
    }
    if(r->video_fence_pending){xsdk_log_write("video-gpu: first frame fence completed");r->video_fence_pending=false;}
    if(!d->video_view)r->video_frame_logged=false;
    bool first_video=d->video_view&&d->video_has_picture&&!r->video_frame_logged;
    if(first_video)xsdk_log_write("video-gpu: preparing first frame format=%u size=%ux%u",d->video_picture.format,d->video_picture.width,d->video_picture.height);
    stats->gpu_wait_us=micros()-start;
    if(r->timed) {
        uint64_t begin=xsdk_gpu_clock_value(r->clocks),end=xsdk_gpu_clock_value(r->clocks+3);
        if(end>=begin)stats->gpu_us=(end-begin)*100/49875;
    }
    if(!prepare(r,d,stats)) {xsdk_log_write("gpu: command/texture preparation failed");return false;}
    if(first_video)xsdk_log_write("video-gpu: first frame texture upload completed");
    start=micros();xsdk_gpu_write_clock(xsdk_gpu_address(r->clocks),true);
    unsigned frame=gpu_ambient_frame(d->elapsed);
    for(int b=0;b<BLADE_COUNT;b++)if((r->plan.ambient_blades&(1u<<b))&&
        (!r->ambient_valid[b]||r->ambient_frame[b]!=frame)) {
        if(!r->ambient[b].texels&&!texture_create(r->ambient+b,384,216))return false;
        target(384,216);shaders(r,&r->swirl,&swirl_frag);
        float constants[8];gpu_ambient_constants(b,frame,constants);
        xsdk_gpu_set_pixel_constants(SWIRL_FRAG_PUSH,constants,2);full(r);
        xsdk_gpu_resolve_texture(r->ambient+b,0xff000000);
        r->ambient_valid[b]=true;r->ambient_frame[b]=frame;stats->bg_updates++;
    }
    target(WIDTH,HEIGHT);
    for(size_t i=0;i<r->plan.batch_count;i++) {
        GpuBatch *batch=r->plan.batches+i;
        if(batch->kind==DRAW_FADE_BEGIN) {
            if(!r->base.texels&&!texture_create(&r->base,WIDTH,HEIGHT))return false;
            xsdk_gpu_resolve_texture(&r->base,0xff000000);target(WIDTH,HEIGHT);copy_base(r,1,false);stats->draws++;
        } else if(batch->kind==DRAW_FADE_END) {
            unsigned weight=(unsigned)(batch->amount*256);
            copy_base(r,1-(float)weight/256,true);stats->draws++;
        } else {
            if(batch->kind==DRAW_AMBIENT) {
                shaders(r,&r->background,&background_frag);
                xsdk_gpu_set_texture(BACKGROUND_FRAG_SAMPLER_0_0,r->ambient+batch->blade,&r->point);
            } else {
                CachedTexture *c=find(r,r->plan.images[batch->image]);if(!c)return false;
                if(image_yuv(c->image.format)) {
                    shaders(r,&r->video,&video_frag);
                    float params[16];video_colour_constants(c->image.format,params);
                    xsdk_gpu_set_pixel_constants(VIDEO_FRAG_PUSH,params,4);
                    xsdk_gpu_set_texture(VIDEO_FRAG_SAMPLER_0_0,&c->texture,&r->linear);
                    xsdk_gpu_set_texture(VIDEO_FRAG_SAMPLER_0_1,&c->u,&r->linear);
                    xsdk_gpu_set_texture(VIDEO_FRAG_SAMPLER_0_2,&c->v,&r->linear);
                } else if(batch->box_height) {
                    shaders(r,&r->cap,&cap_frag);
                    float size[]={(float)c->image.width,(float)c->image.height,(float)batch->box_height,0};
                    xsdk_gpu_set_pixel_constants(CAP_FRAG_PUSH,size,1);
                    xsdk_gpu_set_texture(CAP_FRAG_SAMPLER_0_0,&c->texture,&r->point);
                } else {
                    shaders(r,&r->sprite,&ui_frag);
                    xsdk_gpu_set_texture(UI_FRAG_SAMPLER_0_0,&c->texture,c->image.format==IMAGE_MASK?&r->point:&r->linear);
                }
                xsdk_gpu_set_blend(&r->blend);
            }
            xsdk_gpu_set_vertices(0,xsdk_gpu_address(r->vertices)+batch->first*sizeof(GpuVertex),batch->count*sizeof(GpuVertex));
            xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST,batch->count);stats->draws++;
        }
    }
    xsdk_gpu_resolve(address,WIDTH,HEIGHT,0xff000000);
    xsdk_gpu_write_clock(xsdk_gpu_address(r->clocks+3),true);
    r->last_fence=xsdk_gpu_fence();
    if(first_video){r->video_frame_logged=true;r->video_fence_pending=true;xsdk_log_write("video-gpu: first frame submitted fence=%u",r->last_fence);}
    r->timed=true;stats->cpu_us+=micros()-start;
    bool final_ready=!r->background_checked&&!d->startup_active&&d->blade==BLADE_GAMES&&!d->depth&&
        d->transition==1&&!d->dialog&&!d->exit_menu;
    if(!r->checked||final_ready) {
        if(!NT_SUCCESS(xsdk_gpu_wait(r->last_fence,5000))){xsdk_log_write("gpu: first frame fence timed out");return false;}
        self_check(r,d,address);
    }
    return true;
}
void xbox_renderer_close(XboxRenderer *r) {
    if(!r)return;
    /* A failed frame may have queued work before assigning last_fence. Drain
       the actual queue, including that work, before releasing GPU resources. */
    if(!NT_SUCCESS(xsdk_gpu_wait(xsdk_gpu_fence(),5000))) {
        xsdk_log_write("gpu: shutdown fence timed out; retaining GPU memory until title unload");
        draw_list_close(&r->list);gpu_plan_close(&r->plan);free(r);return;
    }
    for(unsigned i=0;i<CACHE_SIZE;i++)if(r->cache[i].texture.texels)cache_free(r->cache+i);
    for(int b=0;b<BLADE_COUNT;b++)if(r->ambient[b].texels)xsdk_texture_free(r->ambient+b);
    if(r->base.texels)xsdk_texture_free(&r->base);
    if(r->vertices)xsdk_gpu_free(r->vertices);
    if(r->full_quad)xsdk_gpu_free(r->full_quad);
    if(r->clocks)xsdk_gpu_free(r->clocks);
    xsdk_shader_free(&r->cap);xsdk_shader_free(&r->video);xsdk_shader_free(&r->copy);xsdk_shader_free(&r->background);xsdk_shader_free(&r->swirl);
    xsdk_shader_free(&r->sprite);xsdk_shader_free(&r->vertex);
    draw_list_close(&r->list);gpu_plan_close(&r->plan);free(r);
}
