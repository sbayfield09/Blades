#include "pixel_ops.h"
#include <stdint.h>
#ifndef PIXEL_TEST_FREESTANDING
#include <stdio.h>
#endif
static _Alignas(16) uint32_t a[1300],b[1300],out[1310];
static unsigned random_word(unsigned *state) {*state^=*state<<13;*state^=*state>>17;*state^=*state<<5;return *state;}
static int check(void) {
    unsigned state=0x37b145a9;
    for(unsigned i=0;i<1300;i++){a[i]=random_word(&state);b[i]=random_word(&state);}
    static const unsigned counts[]={0,1,2,3,4,5,7,16,31,63,1279,1280};
    for(unsigned offset=0;offset<4;offset++)for(unsigned d=0;d<4;d++)for(unsigned w=0;w<=256;w++)
        for(unsigned n=0;n<sizeof counts/sizeof counts[0];n++) {
            unsigned count=counts[n];uint32_t *dest=out+4+d;
            dest[-1]=0x12345678;dest[count]=0x89abcdef;
            pixels_lerp_row(dest,a+offset,b+offset,w,count);
            if(dest[-1]!=0x12345678||dest[count]!=0x89abcdef)return 1;
            const unsigned char *aa=(const unsigned char *)(a+offset),*bb=(const unsigned char *)(b+offset),*got=(const unsigned char *)dest;
            for(unsigned i=0;i<count*4;i++)
                if(got[i]!=(aa[i]*(256-w)+bb[i]*w)/256)return 2;
        }
    return 0;
}
#ifndef PIXEL_TEST_FREESTANDING
int main(void) {
    int result=check();if(!result)printf("PASS: %s interpolation, every weight, random RGBA, tails, alignment and guards\n",pixels_lerp_backend());return result;
}
#else
/* Standalone PowerPC verification under QEMU, without Xbox kernel imports. */
void *memcpy(void *dest,const void *src,unsigned long bytes) {
    unsigned char *d=dest;const unsigned char *s=src;for(unsigned long i=0;i<bytes;i++)d[i]=s[i];return dest;
}
void _start(void) {
    int result=check();static const char pass[]="PASS: PowerPC VMX interpolation matches scalar bytes\n";
    if(!result) {
        register unsigned long r0 __asm__("r0")=4,r3 __asm__("r3")=1,r4 __asm__("r4")=(unsigned long)pass,r5 __asm__("r5")=sizeof pass-1;
        __asm__ volatile("sc":"+r"(r0),"+r"(r3):"r"(r4),"r"(r5):"memory","cr0");
    }
    register unsigned long r0 __asm__("r0")=1,r3 __asm__("r3")=(unsigned)result;
    __asm__ volatile("sc":"+r"(r0),"+r"(r3)::"memory","cr0");
    __builtin_unreachable();
}
#endif
