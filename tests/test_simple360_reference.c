/* Independently validate the interpolation oracle used by the PPC harness
 * against the pinned decoder's scalar routines, rather than its own port. */
#include "libavcodec/h264chroma.h"
#include "libavcodec/h264qpel.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "simple360_scalar.h"
static uint8_t src[64*32],a[64*32],b[64*32];
static unsigned seed=0x129abdc1;
static unsigned next(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void fill(unsigned pat){for(unsigned i=0;i<sizeof src;i++){src[i]=pat==0?0:pat==1?255:pat==2?(i&1)*255:(uint8_t)next();a[i]=b[i]=(uint8_t)next();}}
int main(void){
 H264ChromaContext c={0},v={0};H264QpelContext q={0},vq={0};
 ff_h264chroma_init(&c,8);scalar_chroma_init(&v);ff_h264qpel_init(&q,8);scalar_qpel_init(&vq);
 unsigned cases=0;
 for(unsigned pat=0;pat<5;pat++)for(unsigned off=0;off<16;off++)for(unsigned w=0;w<2;w++)for(unsigned avg=0;avg<2;avg++){
  for(unsigned pos=0;pos<16;pos++){
   fill(pat);unsigned so=3*64+off,d=2*64+(15-off);
   (avg?q.avg_h264_qpel_pixels_tab:q.put_h264_qpel_pixels_tab)[w][pos](a+d,src+so,64);
   (avg?vq.avg_h264_qpel_pixels_tab:vq.put_h264_qpel_pixels_tab)[w][pos](b+d,src+so,64);
   assert(!memcmp(a,b,sizeof a));cases++;
  }
  for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned h=2;h<=8;h*=2){
   fill(pat);unsigned so=3*64+off,d=2*64+(15-off);
   (avg?c.avg_h264_chroma_pixels_tab:c.put_h264_chroma_pixels_tab)[w](a+d,src+so,64,h,x,y);
   (avg?v.avg_h264_chroma_pixels_tab:v.put_h264_chroma_pixels_tab)[w](b+d,src+so,64,h,x,y);
   assert(!memcmp(a,b,sizeof a));cases++;
  }
 }
 printf("PASS: %u independent interpolation/guard cases against pinned FFmpeg scalar H264\n",cases);
}
