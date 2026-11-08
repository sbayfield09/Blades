/* Motion/lighting adapted from zivvoz / DashX360's BladeAmbientTexture.cs,
   commit 0c8adb2cef4ad7dff2bf889351efa1f1c244aef2. See DASHX360-NOTICE.txt.
   Keeps the supplied 384x216 texture, equations and timing. Lookup tables
   accelerate their evaluation; they do not supply a different pattern. */
#define _POSIX_C_SOURCE 200809L
#include "ambient.h"
#include "theme.h"
#include "pixel_ops.h"
#include "draw_list.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

enum { AW=384, AH=216, LUT=4096 };
typedef struct {float light,ribbon;} Field;
typedef struct {unsigned short index,weight;} Sample;
static struct {
    Field field[AW*AH];
    _Alignas(16) uint32_t pixels[AW*AH];
    _Alignas(16) uint32_t horizontal[AH*WIDTH];
    unsigned short first[AW+1];
    float sine[LUT+1],decay[LUT+1],u[AW],sin_u[AW],cos_u[AW];
    Sample x[WIDTH],y[HEIGHT];
    unsigned frame;
    int blade;
    bool valid;
} *ambient;

static void *ambient_storage;
static bool profiling;
static AmbientStats profile;
void ambient_profile(bool enabled) {profiling=enabled;profile=(AmbientStats){0};}
void ambient_stats(AmbientStats *stats) {if(stats)*stats=profile;}
static uint64_t microseconds(void) {
    struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
    return (uint64_t)now.tv_sec*1000000+(unsigned long)now.tv_nsec/1000;
}

static uint32_t mix(uint32_t a,uint32_t b,unsigned t) {
    unsigned s=256-t;
    uint32_t rb=(((a&0x00ff00ff)*s+(b&0x00ff00ff)*t)>>8)&0x00ff00ff;
    uint32_t g=(((a&0x0000ff00)*s+(b&0x0000ff00)*t)>>8)&0x0000ff00;
    return 0xff000000|rb|g;
}
static bool init(void) {
    if(ambient)return true;
    ambient_storage=calloc(1,sizeof *ambient+15);if(!ambient_storage)return false;
    ambient=(void *)(((uintptr_t)ambient_storage+15)&~(uintptr_t)15);
    ambient->blade=-1;
    for(int i=0;i<=LUT;i++) {
        ambient->sine[i]=sinf((float)i*(6.28318530718f/LUT));
        ambient->decay[i]=expf(-(float)i*(16.0f/LUT));
    }
    for(int x=0;x<AW;x++) {
        float u=((float)x/(AW-1)-0.51f)*2;
        ambient->u[x]=u;ambient->sin_u[x]=sinf(u*3.4f);ambient->cos_u[x]=cosf(u*3.4f);
    }
    for(int x=0;x<WIDTH;x++) {
        unsigned position=(unsigned)x*(AW-1)*256/(WIDTH-1);
        ambient->x[x]=(Sample){(unsigned short)(position>>8),(unsigned short)(position&255)};
        if(!x||ambient->x[x].index!=ambient->x[x-1].index)ambient->first[ambient->x[x].index]=(unsigned short)x;
    }
    ambient->first[AW]=WIDTH;
    for(int y=0;y<HEIGHT;y++) {
        unsigned position=(unsigned)y*(AH-1)*256/(HEIGHT-1);
        ambient->y[y]=(Sample){(unsigned short)(position>>8),(unsigned short)(position&255)};
    }
    return true;
}
static float sine(float angle) {
    float p=angle*(LUT/6.28318530718f);int base=(int)p;
    if((float)base>p)base--;
    unsigned i=(unsigned)base&(LUT-1);float t=p-(float)base;
    return ambient->sine[i]+(ambient->sine[i+1]-ambient->sine[i])*t;
}
static float decay(float value) {
    if(value>=16)return 0;
    float p=value*(LUT/16.0f);unsigned i=(unsigned)p;float t=p-(float)i;
    return ambient->decay[i]+(ambient->decay[i+1]-ambient->decay[i])*t;
}
static void update(unsigned frame) {
    if(profiling)profile.updates++;
    float phase=((float)frame/30)*0.8f;
    float drift=0.11f*cosf(phase*0.73f),column[AW];
    for(int x=0;x<AW;x++)column[x]=0.12f*sinf(((float)x/(AW-1)-0.51f)*6.2f+phase*0.6f);
    for(int y=0;y<AH;y++) {
        float v=((float)y/(AH-1)-0.49f)*1.7f;
        float warp=0.14f*sinf(v*3+phase),b=v+drift;
        float row_sin=sinf(v*2.8f+phase*0.8f),row_cos=cosf(v*2.8f+phase*0.8f);
        for(int x=0;x<AW;x++) {
            float a=ambient->u[x]+warp,radius=sqrtf(a*a*0.83f+b*b);
            float band=(radius-0.65f-column[x])*4.2f,ribbon=decay(band*band);
            float wave=ambient->sin_u[x]*row_cos+ambient->cos_u[x]*row_sin;
            ambient->field[y*AW+x]=(Field){0.69f+ribbon*0.57f+wave*0.08f+sine(radius*9-phase)*0.045f,ribbon};
        }
    }
    ambient->frame=frame;ambient->valid=true;ambient->blade=-1;
}
static unsigned channel(float value) {
    if(value<=0)return 0;
    return value>=255?255:(unsigned)value;
}
static void tint(int blade) {
    uint32_t color=theme_color(blade);
    float r=(float)((color>>16)&255),g=(float)((color>>8)&255),b=(float)(color&255);
    for(int i=0;i<AW*AH;i++) {
        Field f=ambient->field[i];
        ambient->pixels[i]=0xff000000|channel(r*f.light+f.ribbon*18)<<16|
            channel(g*f.light+f.ribbon*25)<<8|channel(b*f.light+f.ribbon*5);
    }
    /* Expand each source row once. Equal-colour runs need no interpolation.
       The later vertical pass uses four pixels per vector on the console. */
    for(int y=0;y<AH;y++) {
        const uint32_t *source=ambient->pixels+y*AW;uint32_t *dest=ambient->horizontal+y*WIDTH;
        for(int col=0;col<AW;col++) {
            uint32_t a=source[col],b=source[col+1<AW?col+1:col];
            int start=ambient->first[col],end=ambient->first[col+1];
            if(a==b)for(int x=start;x<end;x++)dest[x]=a;
            else for(int x=start;x<end;x++)dest[x]=mix(a,b,ambient->x[x].weight);
        }
    }
    ambient->blade=blade;
}
static void draw(Canvas *c,int blade,float seconds) {
    if(!init()){fill_rect(c,0,0,WIDTH,HEIGHT,theme_color(blade));return;}
    if(!isfinite(seconds)||seconds<0)seconds=0;
    /* These rational upstream frequencies share a 250*pi-second period.
       Reduce time before trig/LUT indexing to retain precision in long sessions. */
    seconds=fmodf(seconds,785.398163397f);
    unsigned frame=(unsigned)(seconds*30+0.0001f);
    if(!ambient->valid||ambient->frame!=frame)update(frame);
    if(ambient->blade!=blade)tint(blade);
    for(int y=0;y<HEIGHT;y++) {
        Sample sy=ambient->y[y];int next=sy.index+1<AH?sy.index+1:sy.index;
        pixels_lerp_row(c->pixels+y*WIDTH,ambient->horizontal+sy.index*WIDTH,
            ambient->horizontal+next*WIDTH,sy.weight,WIDTH);
    }
}
void ambient_draw(Canvas *c,int blade,float seconds) {
    if(c->ops){canvas_emit(c,(DrawOp){.kind=DRAW_AMBIENT,.x=0,.w=WIDTH,.color=(uint32_t)blade});return;}
    if(!profiling){draw(c,blade,seconds);return;}
    uint64_t start=microseconds();draw(c,blade,seconds);
    profile.draw_us+=microseconds()-start;profile.draws++;
}
