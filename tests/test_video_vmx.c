/* FFmpeg H264 VMX chroma against independent scalar H264 interpolation.
   Freestanding 32-bit PPC arithmetic check, not Xbox/cycle-accurate execution. */
#include "libavcodec/h264chroma.h"
#include "libavutil/cpu.h"
#include <stdint.h>
#include <stddef.h>
static _Alignas(16) uint8_t source[64*20],dest[64*20],expected[64*20];
int av_get_cpu_flags(void){return AV_CPU_FLAG_ALTIVEC;}
void *memcpy(void *d,const void *s,size_t n){uint8_t *dd=d;const uint8_t *ss=s;for(size_t i=0;i<n;i++)dd[i]=ss[i];return d;}
void *memset(void *d,int c,size_t n){uint8_t *dd=d;for(size_t i=0;i<n;i++)dd[i]=(uint8_t)c;return d;}
static unsigned word(unsigned *s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return *s;}
static int check(void){
 H264ChromaContext c={0};ff_h264chroma_init_ppc(&c,8);if(!c.put_h264_chroma_pixels_tab[0]||!c.avg_h264_chroma_pixels_tab[0])return 1;
 unsigned seed=0x194a62b5;
 for(unsigned align=0;align<16;align++)for(unsigned x=0;x<8;x++)for(unsigned y=0;y<8;y++)for(unsigned avg=0;avg<2;avg++){
  for(unsigned i=0;i<sizeof source;i++){source[i]=(uint8_t)word(&seed);dest[i]=expected[i]=(uint8_t)word(&seed);}
  uint8_t *s=source+64+align,*d=dest+64,*e=expected+64;
  for(unsigned j=0;j<8;j++)for(unsigned i=0;i<8;i++){
   unsigned at=j*64+i;
   unsigned v=((8-x)*(8-y)*s[at]+x*(8-y)*s[at+1]+(8-x)*y*s[at+64]+x*y*s[at+65]+32)>>6;
   e[at]=(uint8_t)(avg?(v+e[at]+1)>>1:v);
  }
  if(avg)c.avg_h264_chroma_pixels_tab[0](d,s,64,8,(int)x,(int)y);else c.put_h264_chroma_pixels_tab[0](d,s,64,8,(int)x,(int)y);
  for(unsigned i=0;i<sizeof dest;i++)if(dest[i]!=expected[i])return 2;
 }
 return 0;
}
void _start(void){
 int result=check();static const char pass[]="PASS: FFmpeg PPC VMX H264 chroma, all offsets/weights, put/average and destination guards\n";
 if(!result){register unsigned long r0 __asm__("r0")=4,r3 __asm__("r3")=1,r4 __asm__("r4")=(unsigned long)pass,r5 __asm__("r5")=sizeof pass-1;__asm__ volatile("sc":"+r"(r0),"+r"(r3):"r"(r4),"r"(r5):"memory","cr0");}
 register unsigned long r0 __asm__("r0")=1,r3 __asm__("r3")=(unsigned)result;__asm__ volatile("sc":"+r"(r0),"+r"(r3)::"memory","cr0");__builtin_unreachable();
}
