#include "blades.h"
#include "font.h"
#include "resources.h"
#include "draw_list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
typedef struct {unsigned char *pixels,*page;unsigned char ready[95];unsigned id,revision;} Atlas;
static Atlas atlases[64];
static struct {
    unsigned generation,origin;
    unsigned char *local;
    const unsigned char *pixels;
    unsigned short advance[95]; /* 1/64 pixel at FONT_EM */
} face;
static unsigned font_u16(const unsigned char *p) {return (unsigned)p[0]<<8|p[1];}
static void font_sync(void) {
    unsigned generation=resources_generation();
    if(face.generation==generation)return;
    for(unsigned i=0;i<64;i++){free(atlases[i].pixels);free(atlases[i].page);memset(&atlases[i],0,sizeof atlases[i]);}
    free(face.local);memset(&face,0,sizeof face);face.generation=generation;face.pixels=font_pixels;
    for(unsigned i=0;i<95;i++)face.advance[i]=(unsigned short)(font_advance[i]*64);
    size_t bytes;const unsigned char *meta=resources_blob("font/metrics.bin",&bytes);
    if(!meta)return;
    if(bytes!=16+95*2||memcmp(meta,"BFN1",4)||font_u16(meta+4)!=FONT_EM||font_u16(meta+6)!=FONT_CELL||
       font_u16(meta+8)!=32||font_u16(meta+10)!=95||font_u16(meta+12)!=16||font_u16(meta+14)>8)return;
    for(unsigned i=0;i<95;i++)if(!font_u16(meta+16+i*2)||font_u16(meta+16+i*2)>FONT_CELL*64)return;
    size_t png_bytes;const unsigned char *png=resources_blob("font/atlas.png",&png_bytes);
    if(!png||png_bytes>16*1024*1024)return;
    int w=0,h=0;unsigned char *rgba=resource_png_decode(png,(int)png_bytes,&w,&h);
    if(!rgba)return;
    if(w!=FONT_CELL*16||h!=FONT_CELL*6){resource_png_free(rgba);return;}
    unsigned char *coverage=malloc(95*FONT_CELL*FONT_CELL);
    if(!coverage){resource_png_free(rgba);return;}
    for(unsigned g=0;g<95;g++)for(unsigned y=0;y<FONT_CELL;y++)for(unsigned x=0;x<FONT_CELL;x++)
        coverage[g*FONT_CELL*FONT_CELL+y*FONT_CELL+x]=rgba[((g/16*FONT_CELL+y)*(unsigned)w+g%16*FONT_CELL+x)*4+3];
    resource_png_free(rgba);face.local=coverage;face.pixels=coverage;face.origin=font_u16(meta+14);
    for(unsigned i=0;i<95;i++)face.advance[i]=(unsigned short)font_u16(meta+16+i*2);
    fprintf(stderr,"font: loaded local glyph atlas\n");
}
static uint32_t blend(uint32_t a,uint32_t b,unsigned t) {
    unsigned r=(((a>>16)&255)*(255-t)+((b>>16)&255)*t)/255;
    unsigned g=(((a>>8)&255)*(255-t)+((b>>8)&255)*t)/255;
    unsigned v=((a&255)*(255-t)+(b&255)*t)/255;
    return 0xff000000u|(r<<16)|(g<<8)|v;
}
void fill_rect(Canvas *c,int x,int y,int w,int h,uint32_t color) {
    if(c->ops){canvas_emit(c,(DrawOp){.kind=DRAW_QUAD,.x=x,.y=y,.w=w,.h=h,.color=color});return;}
    int x1=x+w,y1=y+h;
    if(x<0)x=0;
    if(y<0)y=0;
    if(x1>WIDTH)x1=WIDTH;
    if(y1>HEIGHT)y1=HEIGHT;
    for(int j=y;j<y1;j++) for(int i=x;i<x1;i++) {
        uint32_t *p=&c->pixels[j*WIDTH+i];
        *p=(color>>24)==255?color:blend(*p,color,color>>24);
    }
}
void gradient(Canvas *c,int x,int y,int w,int h,uint32_t a,uint32_t b) {
    for(int i=0;i<h;i++) fill_rect(c,x,y+i,w,1,blend(a,b,(unsigned)(i*255/(h?h:1))));
}
void circle(Canvas *c,int x,int y,int r,uint32_t color) {
    /* Identical integer coverage, one span per row. Missing-artwork buttons
       and spheres must not consume the command budget one pixel at a time. */
    for(int dy=-r;dy<=r;dy++) {
        int square=r*r-dy*dy,dx=(int)sqrtf((float)square);
        while((dx+1)*(dx+1)<=square)dx++;
        while(dx*dx>square)dx--;
        fill_rect(c,x-dx,y+dy,dx*2+1,1,color);
    }
}
static unsigned font_sample_direct(unsigned glyph,int i,int j,int size) {
    unsigned sx=(unsigned)(i*FONT_EM*256/size),sy=(unsigned)(j*FONT_EM*256/size);
    unsigned ix=sx>>8,iy=sy>>8,fx=sx&255,fy=sy&255;
    if(ix>=FONT_CELL-1||iy>=FONT_CELL-1)return 0;
    const unsigned char *p=face.pixels+glyph*FONT_CELL*FONT_CELL+iy*FONT_CELL+ix;
    unsigned top=p[0]*(256-fx)+p[1]*fx;
    unsigned bottom=p[FONT_CELL]*(256-fx)+p[FONT_CELL+1]*fx;
    return (top*(256-fy)+bottom*fy)>>16;
}
/* Cache each scaled glyph on first use; Xbox frames avoid repeated resampling. */
static unsigned font_sample(unsigned glyph,int i,int j,int size) {
    if(size<=0||size>=64)return font_sample_direct(glyph,i,j,size>0?size:1);
    Atlas *atlas=&atlases[size];
    int side=FONT_CELL*size/FONT_EM;
    if(!atlas->pixels)atlas->pixels=malloc((size_t)95*side*side);
    if(!atlas->pixels)return font_sample_direct(glyph,i,j,size);
    unsigned char *map=atlas->pixels+glyph*side*side;
    if(!atlas->ready[glyph]) {
        for(int y=0;y<side;y++)for(int x=0;x<side;x++)
            map[y*side+x]=(unsigned char)font_sample_direct(glyph,x,y,size);
        atlas->ready[glyph]=1;
    }
    return map[j*side+i];
}
static bool draw_glyph(Canvas *c,unsigned glyph,int x,int y,int size,uint32_t color,bool rotate) {
    if(!c->ops||size<=0||size>=64)return false;
    (void)font_sample(glyph,0,0,size);
    Atlas *atlas=&atlases[size];int side=FONT_CELL*size/FONT_EM;
    if(!atlas->pixels)return false;
    if(!atlas->page) {
        atlas->page=calloc((size_t)16*6*side*side,1);
        if(!atlas->page)return false;
        atlas->id=canvas_image_id();
    }
    /* ready==2 means the glyph has also reached the GPU atlas page. */
    if(atlas->ready[glyph]!=2) {
        for(int j=0;j<side;j++)memcpy(atlas->page+((glyph/16*side+j)*16*side+glyph%16*side),
            atlas->pixels+glyph*side*side+j*side,(size_t)side);
        atlas->ready[glyph]=2;atlas->revision++;
    }
    DrawImage image={atlas->page,atlas->id,atlas->revision,side*16,side*6,IMAGE_MASK};
    canvas_mask(c,image,(int)(glyph%16)*side,(int)(glyph/16)*side,side,x,y,color,rotate);
    return true;
}
int text_width(int size,const char *s) {
    font_sync();
    int width=0,line=0;
    for(;*s;s++) {
        if(*s=='\n'){if(line>width)width=line;line=0;continue;}
        unsigned glyph=(unsigned char)*s;if(glyph<32||glyph>126)glyph='?';
        line+=face.advance[glyph-32]*size/FONT_EM;
    }
    return ((line>width?line:width)+32)/64;
}
void text(Canvas *c,int x,int y,int size,uint32_t color,const char *s) {
    font_sync();int cursor=0,bearing=(int)face.origin*size/FONT_EM;
    for(;*s;s++) {
        if(*s=='\n') {cursor=0;y+=size+10;continue;}
        unsigned glyph=(unsigned char)*s; if(glyph<32 || glyph>126)glyph='?'; glyph-=32;
        int side=FONT_CELL*size/FONT_EM;
        if(draw_glyph(c,glyph,x+(cursor+32)/64-bearing,y,size,color,false)) {
            cursor+=face.advance[glyph]*size/FONT_EM;continue;
        }
        for(int j=0;j<side;j++) for(int i=0;i<side;i++) {
            int px=x+(cursor+32)/64+i-bearing,py=y+j; if(px<0||py<0||px>=WIDTH||py>=HEIGHT)continue;
            unsigned a=font_sample(glyph,i,j,size);
            a=a*(color>>24)/255;
            if(a) {
                if(c->ops)fill_rect(c,px,py,1,1,(color&0xffffff)|(a<<24));
                else c->pixels[py*WIDTH+px]=blend(c->pixels[py*WIDTH+px],color,a);
            }
        }
        cursor+=face.advance[glyph]*size/FONT_EM;
    }
}
void vertical_text(Canvas *c,int x,int y,int size,uint32_t color,const char *s) {
    font_sync();int cursor=0,bearing=(int)face.origin*size/FONT_EM;
    for(;*s;s++) {
        unsigned glyph=(unsigned char)*s; if(glyph<32||glyph>126)glyph='?'; glyph-=32;
        int side=FONT_CELL*size/FONT_EM;
        for(int j=0;j<side;j++) for(int i=0;i<side;i++) {
            int px=x+j,py=y-(cursor+32)/64-i+bearing; if(px<0||py<0||px>=WIDTH||py>=HEIGHT)continue;
            unsigned a=font_sample(glyph,i,j,size);
            a=a*(color>>24)/255;
            if(a) {
                if(c->ops)fill_rect(c,px,py,1,1,(color&0xffffff)|(a<<24));
                else c->pixels[py*WIDTH+px]=blend(c->pixels[py*WIDTH+px],color,a);
            }
        }
        cursor+=face.advance[glyph]*size/FONT_EM;
    }
}

void text_down(Canvas *c,int x,int y,int size,uint32_t color,const char *s) {
    font_sync();int cursor=0,bearing=(int)face.origin*size/FONT_EM;
    for(;*s;s++) {
        unsigned glyph=(unsigned char)*s;
        if(glyph<32||glyph>126)glyph='?';
        glyph-=32;
        int side=FONT_CELL*size/FONT_EM;
        if(draw_glyph(c,glyph,x-side+1,y+(cursor+32)/64-bearing,size,color,true)) {
            cursor+=face.advance[glyph]*size/FONT_EM;continue;
        }
        for(int j=0;j<side;j++)for(int i=0;i<side;i++) {
            int px=x-j,py=y+(cursor+32)/64+i-bearing;
            if(px<0||py<0||px>=WIDTH||py>=HEIGHT)continue;
            unsigned a=font_sample(glyph,i,j,size);
            a=a*(color>>24)/255;
            if(a) {
                if(c->ops)fill_rect(c,px,py,1,1,(color&0xffffff)|(a<<24));
                else c->pixels[py*WIDTH+px]=blend(c->pixels[py*WIDTH+px],color,a);
            }
        }
        cursor+=face.advance[glyph]*size/FONT_EM;
    }
}
void line(Canvas *c,int x0,int y0,int x1,int y1,uint32_t color) {
    int dx=x1>x0?x1-x0:x0-x1, sx=x0<x1?1:-1;
    int dy=y1>y0?y0-y1:y1-y0, sy=y0<y1?1:-1, error=dx+dy;
    for(;;) {
        fill_rect(c,x0,y0,1,1,color);
        if(x0==x1&&y0==y1)break;
        int e2=2*error;
        if(e2>=dy){error+=dy;x0+=sx;}
        if(e2<=dx){error+=dx;y0+=sy;}
    }
}
void ellipse(Canvas *c,int x,int y,int rx,int ry,uint32_t color) {
    if(rx<=0||ry<=0)return;
    for(int dy=-ry;dy<=ry;dy++) {
        float f=1-(float)(dy*dy)/(float)(ry*ry);
        int dx=(int)(rx*__builtin_sqrtf(f));
        fill_rect(c,x-dx,y+dy,dx*2+1,1,color);
    }
}

/* Straight-alpha RGBA, independent of host byte order; bilinear resampling. */
void canvas_image(Canvas *c,const unsigned char *rgba,int iw,int ih,int x,int y,int w,int h) {
    canvas_image_opacity(c,rgba,iw,ih,x,y,w,h,255);
}
void canvas_image_opacity(Canvas *c,const unsigned char *rgba,int iw,int ih,int x,int y,int w,int h,unsigned opacity) {
    if(!rgba||iw<=0||ih<=0||w<=0||h<=0)return;
    if(opacity>255)opacity=255;
    if(!opacity)return;
    if(c->ops){canvas_image_tagged_opacity(c,(DrawImage){rgba,0,0,iw,ih,IMAGE_RGBA},x,y,w,h,opacity);return;}
    int first_x=x<0?-x:0,first_y=y<0?-y:0;
    int last_x=x+w>WIDTH?WIDTH-x:w,last_y=y+h>HEIGHT?HEIGHT-y:h;
    if(first_x>=last_x||first_y>=last_y)return;
    if(iw==w&&ih==h) {
        for(int dy=first_y;dy<last_y;dy++)for(int dx=first_x;dx<last_x;dx++) {
            const unsigned char *p=rgba+(dy*iw+dx)*4;
            unsigned alpha=p[3]*opacity/255;if(!alpha)continue;
            uint32_t color=(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];
            uint32_t *pixel=c->pixels+(y+dy)*WIDTH+x+dx;
            *pixel=alpha==255?0xff000000|color:blend(*pixel,color,alpha);
        }
        return;
    }
    /* Visible spans never exceed WIDTH. Compute source positions once per
       column/row instead of dividing twice for every destination pixel. */
    unsigned source_x[WIDTH];
    for(int dx=first_x;dx<last_x;dx++)source_x[dx-first_x]=(unsigned)((uint64_t)dx*(unsigned)iw*256/(unsigned)w);
    for(int dy=first_y;dy<last_y;dy++) {
      unsigned sy=(unsigned)((uint64_t)dy*(unsigned)ih*256/(unsigned)h);
      for(int dx=first_x;dx<last_x;dx++) {
        unsigned sx=source_x[dx-first_x];
        int ix=(int)(sx>>8),iy=(int)(sy>>8),next_x=ix+1<iw?ix+1:ix,next_y=iy+1<ih?iy+1:iy;
        unsigned fx=sx&255,fy=sy&255,channel[4];
        const unsigned char *a=rgba+(iy*iw+ix)*4,*b=rgba+(iy*iw+next_x)*4;
        const unsigned char *d=rgba+(next_y*iw+ix)*4,*e=rgba+(next_y*iw+next_x)*4;
        for(int k=0;k<4;k++) {
            unsigned top=a[k]*(256-fx)+b[k]*fx,bottom=d[k]*(256-fx)+e[k]*fx;
            channel[k]=(top*(256-fy)+bottom*fy)>>16;
        }
        channel[3]=channel[3]*opacity/255;
        uint32_t color=channel[0]<<16|channel[1]<<8|channel[2];
        uint32_t *pixel=c->pixels+(y+dy)*WIDTH+x+dx;
        if(channel[3])*pixel=channel[3]==255?0xff000000|color:blend(*pixel,color,channel[3]);
      }
    }
}
