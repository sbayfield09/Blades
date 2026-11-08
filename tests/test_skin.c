#include "resources.h"
#include "skin.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Original one-pixel PNG and synthetic layout; no reference artwork. */
static const unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,240,31,0,5,0,1,255,137,153,61,29,0,0,0,0,73,69,78,68,174,66,96,130};
enum { MANIFEST=12+5*12*8+5*HEIGHT*4 };
static void put16(unsigned char *p,unsigned n) {p[0]=n>>8;p[1]=n;}
static void put32(unsigned char *p,unsigned n) {p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static unsigned char *fixture(size_t *size,unsigned char **layout,unsigned char **image,bool extended) {
    char names[61][64];strcpy(names[0],"skin/layout.bin");
    for(int i=0;i<60;i++)snprintf(names[i+1],sizeof names[0],"skin/%d/%d.png",i/12,i%12);
    size_t table=0;
    for(int i=0;i<61;i++)table+=9+strlen(names[i])*2;
    unsigned manifest=MANIFEST+(extended?5*HEIGHT*4:0);
    size_t base=22+table;*size=base+manifest+sizeof png;
    unsigned char *out=calloc(*size,1);assert(out);
    memcpy(out,"XUIZ",4);put32(out+4,1);put32(out+8,*size);put32(out+16,table);put16(out+20,61);
    size_t cursor=22;
    for(int i=0;i<61;i++) {
        put32(out+cursor,i?sizeof png:manifest);put32(out+cursor+4,i?manifest:0);
        size_t n=strlen(names[i]);out[cursor+8]=n;cursor+=9;
        for(size_t j=0;j<n;j++)out[cursor+j*2+1]=names[i][j];
        cursor+=n*2;
    }
    *layout=out+base;*image=out+base+manifest;
    memcpy(*layout,extended?"BSK2":"BSK1",4);put16(*layout+4,WIDTH);put16(*layout+6,HEIGHT);put16(*layout+8,5);put16(*layout+10,12);
    for(int i=0;i<60;i++) {
        unsigned char *r=*layout+12+i*8;put16(r,300);put16(r+2,100);put16(r+4,1);put16(r+6,1);
    }
    for(int i=0;i<5*HEIGHT;i++) {put16(*layout+492+i*4,200);put16(*layout+494+i*4,1100);}
    if(extended)for(int b=0;b<5;b++)for(int y=0;y<HEIGHT;y++) {
        bool corner=y<80||y>640;unsigned char *r=*layout+MANIFEST+(b*HEIGHT+y)*4;
        put16(r,(corner?24:40)+b);put16(r+2,(corner?1250:1240)-b);
    }
    memcpy(*image,png,sizeof png);return out;
}
int main(void) {
    size_t size;unsigned char *layout,*image,*bytes=fixture(&size,&layout,&image,false);
    uint32_t *pixels=calloc(WIDTH*HEIGHT+2,sizeof *pixels);assert(pixels);
    pixels[0]=0x123456;pixels[WIDTH*HEIGHT+1]=0xabcdef;Canvas c={.pixels=pixels+1};
    for(int i=0;i<WIDTH*HEIGHT;i++)c.pixels[i]=0xffffffff;
    assert(resources_open_memory(bytes,size));assert(skin_frame(&c,2,0,68,610));
    /* The synthetic caps overlap at y100. Their red body remains visible;
       header shading fades upward, footer shading fades downward. */
    unsigned body=c.pixels[100*WIDTH+300];
    assert((body&0xff00ffff)==0xff000000&&((body>>16)&255)>128&&((body>>16)&255)<255);
    unsigned upper=c.pixels[96*WIDTH+300]&255,lower=c.pixels[104*WIDTH+300]&255;
    assert(upper<255&&lower<255);
    assert((c.pixels[93*WIDTH+300]&255)>upper&&(c.pixels[107*WIDTH+300]&255)>lower);
    assert(c.pixels[92*WIDTH+300]==0xffffffff&&c.pixels[108*WIDTH+300]==0xffffffff);
    /* Outer tabs clear early on opening, then return near the end of closing.
       The synthetic sprites remain distinguishable by their screen region. */
    for(unsigned closing=0;closing<2;closing++) {
        memset(c.pixels,0,WIDTH*HEIGHT*sizeof *c.pixels);
        assert(skin_chrome(&c,2,.2f,closing!=0));
        bool outer_visible=false;
        for(int x=0;x<180;x++)if(c.pixels[100*WIDTH+x])outer_visible=true;
        assert(outer_visible==(closing!=0));
    }
    /* Reject invalid geometry before touching the canvas; each reload must
       invalidate cached pointers, including recovery after a rejected pack. */
    put16(layout+4,WIDTH+1);assert(resources_open_memory(bytes,size));assert(!skin_frame(&c,2,0,68,610));put16(layout+4,WIDTH);
    put16(layout+12,WIDTH);assert(resources_open_memory(bytes,size));assert(!skin_chrome(&c,2,0,false));put16(layout+12,300);
    put16(layout+492,1100);assert(resources_open_memory(bytes,size));assert(!skin_leaf(&c,1,2,0.5f));put16(layout+492,200);
    image[0]=0;assert(resources_open_memory(bytes,size));assert(!skin_frame(&c,2,0,68,610));image[0]=137;
    assert(resources_open_memory(bytes,size));assert(skin_frame(&c,2,1,147,531));
    resources_close();assert(!skin_frame(&c,2,0,68,610));
    free(bytes);bytes=fixture(&size,&layout,&image,true);
    assert(resources_open_memory(bytes,size));
    /* The expanded shell includes its tab and curved edges, beyond the root
       content rectangle. Guard these previously grey areas on every blade. */
    for(int b=0;b<5;b++) {
        const int rows[]={0,90,180,360,650,HEIGHT-1};
        for(unsigned i=0;i<sizeof rows/sizeof rows[0];i++) {
            int y=rows[i];
            for(int n=0;n<WIDTH*HEIGHT;n++)c.pixels[n]=0xff334455;
            assert(skin_frame(&c,b,1,68,610));
            assert(c.pixels[y*WIDTH+80]==0xff334455&&c.pixels[y*WIDTH+1200]==0xff334455);
            assert(c.pixels[y*WIDTH+10]==0xffb9bec4&&c.pixels[y*WIDTH+1270]==0xffb9bec4);
        }
        for(int n=0;n<WIDTH*HEIGHT;n++)c.pixels[n]=0xff334455;
        assert(skin_frame(&c,b,0,68,610));assert(c.pixels[360*WIDTH+80]==0xffb9bec4);
        for(int n=0;n<WIDTH*HEIGHT;n++)c.pixels[n]=0xff334455;
        assert(skin_frame(&c,b,0.5f,68,610));assert(c.pixels[360*WIDTH+150]==0xff334455&&c.pixels[360*WIDTH+1160]==0xff334455);
    }
    put16(layout+MANIFEST,WIDTH);assert(resources_open_memory(bytes,size));assert(!skin_frame(&c,0,1,68,610));
    resources_close();
    assert(pixels[0]==0x123456&&pixels[WIDTH*HEIGHT+1]==0xabcdef);
    free(pixels);free(bytes);puts("PASS: legacy/new skin bounds, expanded colour coverage, corrupt images and reload/cache invalidation");return 0;
}
