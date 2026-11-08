/* Run the shipping Xenon filter port on big-endian PPC under QEMU.
 * Compare every subpixel position, put/average, both block widths and all
 * source/destination alignments against FFmpeg's scalar H264 definitions.
 * This verifies arithmetic and guards, not Xbox speed or GPU execution. */
#include "libavcodec/h264chroma.h"
#include "libavcodec/h264qpel.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include "simple360_scalar.h"
void ff_xb_h264_chroma_init(H264ChromaContext *);
void ff_xb_h264_qpel_init(H264QpelContext *);
extern int ff_xb_chroma_verdict,ff_xb_qpel_verdict;
static _Alignas(16) uint8_t src[64*32],a[64*32],b[64*32];
void *memcpy(void *d,const void *s,size_t n){uint8_t *dd=d;const uint8_t *ss=s;for(size_t i=0;i<n;i++)dd[i]=ss[i];return d;}
void *memset(void *d,int c,size_t n){uint8_t *dd=d;for(size_t i=0;i<n;i++)dd[i]=(uint8_t)c;return d;}
int memcmp(const void *a0,const void *b0,size_t n){const uint8_t *aa=a0,*bb=b0;for(size_t i=0;i<n;i++)if(aa[i]!=bb[i])return aa[i]<bb[i]?-1:1;return 0;}
int pthread_mutex_lock(pthread_mutex_t *p){(void)p;return 0;}
int pthread_mutex_unlock(pthread_mutex_t *p){(void)p;return 0;}
static void finish(unsigned code){register unsigned long r0 __asm__("r0")=1,r3 __asm__("r3")=code;__asm__ volatile("sc":"+r"(r0),"+r"(r3)::"memory","cr0");__builtin_unreachable();}
void abort(void){finish(99);}
int av_log(void *c,int n,const char *f,...){(void)c;(void)n;(void)f;return 0;}
static unsigned seed=0x34acfe19;
static void diagnostic(const char *name,unsigned value){
 char line[64];unsigned n=0;while(*name)line[n++]=*name++;line[n++]='=';
 for(int i=7;i>=0;i--)line[n++]="0123456789abcdef"[(value>>(i*4))&15];line[n++]='\n';
 register unsigned long r0 __asm__("r0")=4,r3 __asm__("r3")=1,r4 __asm__("r4")=(unsigned long)line,r5 __asm__("r5")=n;__asm__ volatile("sc":"+r"(r0),"+r"(r3):"r"(r4),"r"(r5):"memory","cr0");
}
static unsigned random_word(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void fill(unsigned pattern){for(unsigned i=0;i<sizeof src;i++){src[i]=pattern==0?0:pattern==1?255:pattern==2?(i&1)*255:(uint8_t)random_word();a[i]=b[i]=(uint8_t)random_word();}}
static int check(void){
 H264ChromaContext c,v;H264QpelContext q,vq;
 memset(&c,0,sizeof c);memset(&q,0,sizeof q);
 scalar_chroma_init(&c);memcpy(&v,&c,sizeof c);ff_xb_h264_chroma_init(&v);
 scalar_qpel_init(&q);memcpy(&vq,&q,sizeof q);ff_xb_h264_qpel_init(&vq);
 diagnostic("chroma self-test",(unsigned)ff_xb_chroma_verdict);diagnostic("qpel self-test",(unsigned)ff_xb_qpel_verdict);
 if(ff_xb_chroma_verdict||ff_xb_qpel_verdict)return 1;
 if(c.put_h264_chroma_pixels_tab[0]==v.put_h264_chroma_pixels_tab[0]||q.put_h264_qpel_pixels_tab[0][0]==vq.put_h264_qpel_pixels_tab[0][0])return 2;
 /* Small blocks remain the original scalar routines, exactly as upstream. */
 if(c.put_h264_chroma_pixels_tab[2]!=v.put_h264_chroma_pixels_tab[2]||q.put_h264_qpel_pixels_tab[2][0]!=vq.put_h264_qpel_pixels_tab[2][0])return 3;
 for(unsigned pattern=0;pattern<5;pattern++)for(unsigned off=0;off<16;off++)for(unsigned width=0;width<2;width++)for(unsigned avg=0;avg<2;avg++){
  for(unsigned pos=0;pos<16;pos++){
   fill(pattern);unsigned so=3*64+off,d=2*64+(15-off);
   (avg?q.avg_h264_qpel_pixels_tab:q.put_h264_qpel_pixels_tab)[width][pos](a+d,src+so,64);
   (avg?vq.avg_h264_qpel_pixels_tab:vq.put_h264_qpel_pixels_tab)[width][pos](b+d,src+so,64);
   if(memcmp(a,b,sizeof a))return 4;
  }
  for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned h=2;h<=8;h*=2){
   fill(pattern);unsigned so=3*64+off,d=2*64+(15-off);
   (avg?c.avg_h264_chroma_pixels_tab:c.put_h264_chroma_pixels_tab)[width](a+d,src+so,64,h,x,y);
   (avg?v.avg_h264_chroma_pixels_tab:v.put_h264_chroma_pixels_tab)[width](b+d,src+so,64,h,x,y);
   if(memcmp(a,b,sizeof a))return 5;
  }
 }
 return 0;
}
void _start(void){
 unsigned result=check();static const char pass[]="PASS: shipping Simple360 chroma/qpel, runtime self-tests; all subpixel positions, put/average, widths, alignments, destination guards and scalar small-block fallback\n";
 if(!result){register unsigned long r0 __asm__("r0")=4,r3 __asm__("r3")=1,r4 __asm__("r4")=(unsigned long)pass,r5 __asm__("r5")=sizeof pass-1;__asm__ volatile("sc":"+r"(r0),"+r"(r3):"r"(r4),"r"(r5):"memory","cr0");}finish(result);
}
