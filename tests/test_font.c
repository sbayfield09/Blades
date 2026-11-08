#include "resources.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font_fixture.h"
static void put16(unsigned char *p,unsigned n){p[0]=n>>8;p[1]=n;}
static void put32(unsigned char *p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static unsigned char *pack(const unsigned char *png,size_t png_size,size_t *size,unsigned char **meta) {
    const char *names[]={"font/metrics.bin","font/atlas.png"};unsigned table=0;
    for(int i=0;i<2;i++)table+=9+(unsigned)strlen(names[i])*2;
    unsigned base=22+table;*size=base+206+png_size;
    unsigned char *out=calloc(*size,1);assert(out);
    memcpy(out,"XUIZ",4);put32(out+4,1);put32(out+8,*size);put32(out+16,table);put16(out+20,2);
    unsigned cursor=22;
    for(int i=0;i<2;i++) {
        put32(out+cursor,i?png_size:206);put32(out+cursor+4,i?206:0);
        unsigned n=(unsigned)strlen(names[i]);out[cursor+8]=n;cursor+=9;
        for(unsigned j=0;j<n;j++)out[cursor+j*2+1]=names[i][j];cursor+=n*2;
    }
    *meta=out+base;memcpy(*meta,"BFN1",4);put16(*meta+4,48);put16(*meta+6,64);
    put16(*meta+8,32);put16(*meta+10,95);put16(*meta+12,16);
    for(int i=0;i<95;i++)put16(*meta+16+i*2,8*64);
    put16(*meta+16+('A'-32)*2,24*64);
    memcpy(out+base+206,png,png_size);return out;
}
int main(void) {
    uint32_t *mem=calloc(WIDTH*HEIGHT+2,sizeof *mem),*fallback=malloc(WIDTH*HEIGHT*sizeof *fallback);assert(mem&&fallback);
    mem[0]=0x123456;mem[WIDTH*HEIGHT+1]=0xabcdef;Canvas c={.pixels=mem+1};
    int original_width=text_width(48,"A");assert(original_width!=24);
    text(&c,0,0,48,0xffffffff,"A");memcpy(fallback,c.pixels,WIDTH*HEIGHT*sizeof *fallback);
    size_t size;unsigned char *meta,*bytes=pack(fixture_font_png,sizeof fixture_font_png,&size,&meta);
    assert(resources_open_memory(bytes,size));assert(text_width(48,"AA")==48&&text_width(48,"A\nAA")==48);
    memset(c.pixels,0,WIDTH*HEIGHT*sizeof *c.pixels);text(&c,0,0,48,0xffffffff,"A");
    assert(c.pixels[10*WIDTH+10]==0xffffffff&&c.pixels[25*WIDTH+21]==0xffffffff&&c.pixels[10*WIDTH+22]==0);
    text(&c,100,100,24,0xffffffff,"A");assert(c.pixels[105*WIDTH+105]==0xffffffff);
    text(&c,-12,-12,48,0xffffffff,"AA");text_down(&c,60,-5,48,0x80ffffff,"AA");
    put16(meta+16+('A'-32)*2,5000);assert(resources_open_memory(bytes,size));
    assert(text_width(48,"A")==original_width);
    memset(c.pixels,0,WIDTH*HEIGHT*sizeof *c.pixels);text(&c,0,0,48,0xffffffff,"A");
    assert(!memcmp(c.pixels,fallback,WIDTH*HEIGHT*sizeof *fallback));
    put16(meta+16+('A'-32)*2,24*64);put16(meta+10,96);assert(resources_open_memory(bytes,size));
    assert(text_width(48,"A")==original_width);free(bytes);
    bytes=pack(fixture_empty_png,sizeof fixture_empty_png,&size,&meta);
    assert(resources_open_memory(bytes,size));assert(text_width(48,"A")==24);
    memset(c.pixels,0,WIDTH*HEIGHT*sizeof *c.pixels);text(&c,0,0,48,0xffffffff,"A");
    assert(c.pixels[10*WIDTH+10]==0); /* reload must discard the previous scaled glyph cache */
    resources_close();assert(text_width(48,"A")==original_width);
    assert(mem[0]==0x123456&&mem[WIDTH*HEIGHT+1]==0xabcdef);
    free(bytes);free(fallback);free(mem);puts("PASS: local font metrics/raster, scaling, clipping, malformed metadata, reload and fallback");return 0;
}
