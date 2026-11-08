#include "ambient.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Independent golden samples from the supplied C# equations/new reference tints/new reference tints, evaluated using
   double precision. Allow two levels for LUT and image resampling rounding. */
static const uint32_t expected[5][6]={
    {0xff974b24,0xff8d4621,0xffa25026,0xffa55327,0xffa45127,0xff944923},
    {0xff956c33,0xff8b6530,0xff9f7437,0xffa27738,0xffa17538,0xff916a32},
    {0xff327926,0xff2f7124,0xff368229,0xff38852a,0xff36832a,0xff317626},
    {0xff285c8c,0xff255682,0xff2b6396,0xff2d6597,0xff2b6497,0xff275a88},
    {0xff624b91,0xff5c4687,0xff69509b,0xff6b539c,0xff6a519d,0xff60498d}
};
int main(void) {
    Dashboard dashboard;dashboard_init(&dashboard);
    dashboard_update(&dashboard,0,.25f);
    assert(fabsf(dashboard.elapsed-.25f)<1e-6f);
    dashboard_update(&dashboard,0,NAN);dashboard_update(&dashboard,0,INFINITY);
    assert(fabsf(dashboard.elapsed-.25f)<1e-6f);
    uint32_t *mem=calloc(WIDTH*HEIGHT+2,sizeof *mem),*copy=malloc(WIDTH*HEIGHT*sizeof *copy);
    assert(mem&&copy);mem[0]=0x123456;mem[WIDTH*HEIGHT+1]=0xabcdef;Canvas c={.pixels=mem+1};
    const int xs[]={0,1279,640,400,0,640},ys[]={0,719,360,300,0,360};
    const float times[]={0,0,0,2,2,5};
    for(int b=0;b<BLADE_COUNT;b++)for(int i=0;i<6;i++) {
        ambient_draw(&c,b,times[i]);uint32_t pixel=c.pixels[ys[i]*WIDTH+xs[i]];
        assert((pixel>>24)==255);
        for(int shift=0;shift<24;shift+=8) {
            int actual=(pixel>>shift)&255,want=(expected[b][i]>>shift)&255;
            assert(abs(actual-want)<=2);
        }
    }
    const struct {int x,y;float time;uint32_t color;} bright[]={
        {220,180,0,0xff4aae36},
        {1080,500,0,0xff67ea48},
        {300,550,2,0xff5ed641},
        {1000,180,5,0xff4eb437}
    };
    for(unsigned i=0;i<sizeof bright/sizeof bright[0];i++) {
        ambient_draw(&c,BLADE_GAMES,bright[i].time);
        uint32_t p=c.pixels[bright[i].y*WIDTH+bright[i].x];
        for(int shift=0;shift<24;shift+=8)assert(abs((int)((p>>shift)&255)-(int)((bright[i].color>>shift)&255))<=2);
    }
    ambient_draw(&c,BLADE_GAMES,0);memcpy(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy);
    ambient_draw(&c,BLADE_GAMES,1.0f/60);assert(!memcmp(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy));
    ambient_draw(&c,BLADE_GAMES,2);
    unsigned changed=0;
    for(int i=0;i<WIDTH*HEIGHT;i++){if(copy[i]!=c.pixels[i])changed++;assert(c.pixels[i]>>24==255);}
    assert(changed>WIDTH*HEIGHT/2);
    memcpy(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy);
    ambient_draw(&c,BLADE_MEDIA,2);ambient_draw(&c,BLADE_GAMES,2);
    assert(!memcmp(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy)); /* shared clock on blade change */
    ambient_draw(&c,BLADE_GAMES,0);memcpy(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy);
    ambient_draw(&c,BLADE_GAMES,NAN);assert(!memcmp(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy));
    ambient_draw(&c,BLADE_GAMES,-1);assert(!memcmp(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy));
    ambient_draw(&c,BLADE_GAMES,INFINITY);assert(!memcmp(copy,c.pixels,WIDTH*HEIGHT*sizeof *copy));
    ambient_draw(&c,BLADE_GAMES,FLT_MAX);
    ambient_draw(&c,BLADE_GAMES,0);ambient_profile(true);
    ambient_draw(&c,BLADE_GAMES,2);ambient_draw(&c,BLADE_GAMES,2);
    AmbientStats stats;ambient_stats(&stats);
    assert(stats.draws==2&&stats.updates==1);ambient_profile(false);
    assert(mem[0]==0x123456&&mem[WIDTH*HEIGHT+1]==0xabcdef);
    free(copy);free(mem);puts("PASS: supplied animation samples, visible motion, shared clock, invalid times and render bounds");return 0;
}
