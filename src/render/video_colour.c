#include "draw_list.h"
#include <math.h>
void video_colour_constants(ImageFormat f,float v[16]) {
    bool hd=f==IMAGE_YUV709||f==IMAGE_YUV709_FULL;
    bool full=f==IMAGE_YUV601_FULL||f==IMAGE_YUV709_FULL;
    float kr=hd?.2126f:.299f,kb=hd?.0722f:.114f,kg=1-kr-kb;
    float ys=full?1:255.f/219,cs=full?1:255.f/224;
    float matrix[]={ys,0,2*(1-kr)*cs,0,ys,-2*kb*(1-kb)*cs/kg,-2*kr*(1-kr)*cs/kg,0,
        ys,2*(1-kb)*cs,0,0,full?0:16.f/255,128.f/255,0,0};
    for(unsigned i=0;i<16;i++)v[i]=matrix[i];
}
static float sample(DrawImage i,float x,float y,int left,int top,int right,int bottom) {
    x=fmaxf(left,fminf(right,x));y=fmaxf(top,fminf(bottom,y));
    int x0=(int)x,y0=(int)y,x1=x0<right?x0+1:x0,y1=y0<bottom?y0+1:y0;
    float dx=x-x0,dy=y-y0;
    float a=i.pixels[y0*i.width+x0]*(1-dx)+i.pixels[y0*i.width+x1]*dx;
    float b=i.pixels[y1*i.width+x0]*(1-dx)+i.pixels[y1*i.width+x1]*dx;return a*(1-dy)+b*dy;
}
static unsigned channel(float x){return (unsigned)lroundf(fmaxf(0,fminf(255,x)));}
void canvas_yuv420(Canvas *c,DrawImage i,int x,int y,int w,int h) {
    if(!c||!c->pixels||!i.pixels||w<=0||h<=0||i.width<2||i.height<2)return;
    bool hd=i.format==IMAGE_YUV709||i.format==IMAGE_YUV709_FULL;
    bool full=i.format==IMAGE_YUV601_FULL||i.format==IMAGE_YUV709_FULL;
    float kr=hd?.2126f:.299f,kb=hd?.0722f:.114f,kg=1-kr-kb,scale=full?1:255.f/224;
    for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++) {
        int px=x+dx,py=y+dy;if(px<0||px>=WIDTH||py<0||py>=HEIGHT)continue;
        float sx=dx*(float)i.width/w,sy=dy*(float)i.height/h;
        float yy=sample(i,sx,sy,0,0,i.width-1,i.height-1);
        float u=sample(i,(sx-.5f)*.5f,i.height+(sy-.5f)*.5f,0,i.height,i.width/2-1,i.height+i.height/2-1)-128;
        float v=sample(i,i.width/2+(sx-.5f)*.5f,i.height+(sy-.5f)*.5f,i.width/2,i.height,i.width-1,i.height+i.height/2-1)-128;
        float luma=full?yy:(yy-16)*255.f/219;
        unsigned r=channel(luma+2*(1-kr)*v*scale),b=channel(luma+2*(1-kb)*u*scale);
        unsigned g=channel(luma-2*(kb*(1-kb)*u+kr*(1-kr)*v)*scale/kg);
        c->pixels[py*WIDTH+px]=0xff000000|r<<16|g<<8|b;
    }
}
