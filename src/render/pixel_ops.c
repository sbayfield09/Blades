#include "pixel_ops.h"
#include <string.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__ALTIVEC__)
#include <altivec.h>
typedef unsigned char Bytes16 __attribute__((vector_size(16),may_alias));
typedef unsigned short Shorts8 __attribute__((vector_size(16)));
typedef unsigned int Words4 __attribute__((vector_size(16)));
typedef float Floats4 __attribute__((vector_size(16)));
static inline Words4 vmx_lerp4(Words4 a,Words4 b,Floats4 s,Floats4 t) {
    /* Use vector floating-point multiply-add without relying on classic
       AltiVec integer-multiply instructions. All values here are dyadic with
       <=16 significant bits, so float arithmetic and truncation are exact. */
    Floats4 result=vec_madd(vec_ctf(a,0),s,vec_madd(vec_ctf(b,0),t,vec_splats(0.0f)));
    return vec_ctu(result,0);
}
#endif
static uint32_t lerp(uint32_t a,uint32_t b,unsigned t) {
    unsigned s=256-t;
    uint32_t rb=(((a&0x00ff00ff)*s+(b&0x00ff00ff)*t)>>8)&0x00ff00ff;
    uint32_t ag=((((a>>8)&0x00ff00ff)*s+((b>>8)&0x00ff00ff)*t)>>8)&0x00ff00ff;
    return rb|(ag<<8);
}
const char *pixels_lerp_backend(void) {
#if defined(__SSE2__)
    return "SSE2";
#elif defined(__ALTIVEC__)
    return "VMX";
#else
    return "scalar";
#endif
}
void pixels_lerp_row(uint32_t *out,const uint32_t *a,const uint32_t *b,unsigned weight,size_t count) {
    if(!count)return;
    if(weight>256)weight=256;
    if(!weight||a==b){memcpy(out,a,count*4);return;}
    if(weight==256){memcpy(out,b,count*4);return;}
    size_t x=0;
#if defined(__SSE2__)
    __m128i zero=_mm_setzero_si128(),t=_mm_set1_epi16((short)weight),s=_mm_set1_epi16((short)(256-weight));
    for(;x+4<=count;x+=4) {
        __m128i av=_mm_loadu_si128((const __m128i *)(a+x)),bv=_mm_loadu_si128((const __m128i *)(b+x));
        __m128i lo=_mm_add_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(av,zero),s),_mm_mullo_epi16(_mm_unpacklo_epi8(bv,zero),t));
        __m128i hi=_mm_add_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(av,zero),s),_mm_mullo_epi16(_mm_unpackhi_epi8(bv,zero),t));
        _mm_storeu_si128((__m128i *)(out+x),_mm_packus_epi16(_mm_srli_epi16(lo,8),_mm_srli_epi16(hi,8)));
    }
#elif defined(__ALTIVEC__)
    if((((uintptr_t)a|(uintptr_t)b)&15)==0) {
        const Bytes16 zero={0};const Shorts8 z={0};
        Floats4 t=vec_ctf(vec_splats(weight),8),s=vec_sub(vec_splats(1.0f),t);
        for(;x+4<=count;x+=4) {
            Bytes16 av=*(const Bytes16 *)(a+x),bv=*(const Bytes16 *)(b+x);
            Shorts8 ah=(Shorts8)vec_mergeh(zero,av),al=(Shorts8)vec_mergel(zero,av);
            Shorts8 bh=(Shorts8)vec_mergeh(zero,bv),bl=(Shorts8)vec_mergel(zero,bv);
            Shorts8 hi=vec_pack(
                vmx_lerp4((Words4)vec_mergeh(z,ah),(Words4)vec_mergeh(z,bh),s,t),
                vmx_lerp4((Words4)vec_mergel(z,ah),(Words4)vec_mergel(z,bh),s,t));
            Shorts8 lo=vec_pack(
                vmx_lerp4((Words4)vec_mergeh(z,al),(Words4)vec_mergeh(z,bl),s,t),
                vmx_lerp4((Words4)vec_mergel(z,al),(Words4)vec_mergel(z,bl),s,t));
            Bytes16 result=vec_pack(hi,lo);
            /* Canvas pointers need only uint32 alignment. Keep vector loads
               aligned and use ordinary stores if the destination is offset. */
            if(((uintptr_t)(out+x)&15)==0)*(Bytes16 *)(out+x)=result;
            else {
                union {Bytes16 bytes;uint32_t words[4];} value={.bytes=result};
                volatile uint32_t *dest=out+x;
                for(unsigned i=0;i<4;i++)dest[i]=value.words[i];
            }
        }
    }
#endif
    for(;x<count;x++)out[x]=lerp(a[x],b[x],weight);
}
