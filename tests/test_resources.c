#include "resources.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,240,31,0,5,0,1,255,137,153,61,29,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char wide_png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,6,0,0,0,244,34,127,138,0,0,0,17,73,68,65,84,120,156,99,252,207,192,240,159,129,129,129,1,0,13,5,2,0,224,7,107,2,0,0,0,0,73,69,78,68,174,66,96,130};
static void put32(unsigned char *p,unsigned value) {p[0]=value>>24;p[1]=value>>16;p[2]=value>>8;p[3]=value;}
int main(int argc,char **argv) {
    unsigned char bytes[512]={0};
    const char *name="unit.png";unsigned name_chars=(unsigned)strlen(name);
    unsigned table=9+name_chars*2,base=22+table,size=base+sizeof png;
    memcpy(bytes,"XUIZ",4);put32(bytes+4,1);put32(bytes+8,size);put32(bytes+16,table);bytes[21]=1;
    put32(bytes+22,sizeof png);bytes[30]=(unsigned char)name_chars;
    for(unsigned i=0;i<name_chars;i++)bytes[31+i*2+1]=(unsigned char)name[i];
    memcpy(bytes+base,png,sizeof png);
    assert(!resources_open_memory(NULL,0));
    for(unsigned n=0;n<size;n++)assert(!resources_open_memory(bytes,n));
    assert(resources_open_memory(bytes,size));
    uint32_t *pixels=calloc(WIDTH*HEIGHT+2,sizeof(uint32_t));assert(pixels);
    pixels[0]=0x123456;pixels[WIDTH*HEIGHT+1]=0xabcdef;Canvas c={.pixels=pixels+1};
    assert(resources_draw(&c,name,100,100,1,1));assert(c.pixels[100*WIDTH+100]==0xffff0000);
    assert(resources_draw(&c,name,-2,-2,4,4));assert(c.pixels[0]==0xffff0000);
    assert(pixels[0]==0x123456&&pixels[WIDTH*HEIGHT+1]==0xabcdef);
    assert(!resources_draw(&c,"absent.png",0,0,32,32));
    unsigned char bad[512];memcpy(bad,bytes,size);put32(bad+26,0xffffffff);
    assert(!resources_open_memory(bad,size));
    assert(resources_draw(&c,name,0,0,1,1)); /* failed reload preserves valid pack */
    memcpy(bad,bytes,size);put32(bad+16,0xffffffff);assert(!resources_open_memory(bad,size));
    memcpy(bad,bytes,size);bad[30]=159;assert(!resources_open_memory(bad,size));
    memcpy(bad,bytes,size);bad[base]=0;assert(resources_open_memory(bad,size));
    assert(!resources_draw(&c,name,0,0,1,1));
    resources_close();assert(!resources_draw(&c,name,0,0,1,1));
    /* A non-square icon must be centered within its box without stretching;
       after reload, use the new image's dimensions rather than a cached size. */
    size=base+sizeof wide_png;put32(bytes+8,size);put32(bytes+22,sizeof wide_png);
    memcpy(bytes+base,wide_png,sizeof wide_png);assert(resources_open_memory(bytes,size));
    memset(c.pixels,0,WIDTH*HEIGHT*sizeof *c.pixels);
    assert(resources_draw_fit(&c,name,100,100,20,20));
    assert(c.pixels[100*WIDTH+100]==0&&c.pixels[104*WIDTH+100]==0);
    assert(c.pixels[105*WIDTH+100]==0xffff0000&&c.pixels[114*WIDTH+119]==0xffff0000);
    assert(c.pixels[115*WIDTH+100]==0);
    assert(resources_draw_fit(&c,name,-10,-5,20,20));
    assert(!resources_draw_fit(&c,name,0,0,0,20));
    assert(pixels[0]==0x123456&&pixels[WIDTH*HEIGHT+1]==0xabcdef);
    resources_close();
    if(argc==2) {
        assert(resources_open(argv[1]));
        const char *stock[]={"A-Button.png","B-Button.png","X-Button.png","Y-Button.png", "livelogo.png","splash_360.png","ico_64x_trophy.png","ico_64x_controller.png","64_fffe07d10002000100010001.png"};
        for(unsigned i=0;i<sizeof stock/sizeof stock[0];i++)assert(resources_draw(&c,stock[i],0,0,64,64));
        size_t prompt_size;
        if(resources_blob("prompt/A.png",&prompt_size)) {
            for(unsigned i=0;i<4;i++)for(unsigned disabled=0;disabled<2;disabled++) {
                char key[40];snprintf(key,sizeof key,"prompt/%c%s.png","ABXY"[i],disabled?"-disabled":"");
                assert(resources_draw(&c,key,-4,-4,32,32));
                assert(pixels[0]==0x123456&&pixels[WIDTH*HEIGHT+1]==0xabcdef);
            }
        }
        resources_close();
    }
    free(pixels);puts("PASS: XUIZ/PNG decode, clipping, missing/corrupt resources, safe reload and optional stock pack");return 0;
}
