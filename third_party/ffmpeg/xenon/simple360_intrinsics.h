/* GCC adaptation of the XDK vector intrinsics used by Simple360Player.
 * GPL-3.0; see third_party/simple360player/COPYING.GPLv3 and UPSTREAM.json.
 * Only the Xenon-supported integer shift/add/permute subset is used here.
 * Generic FFmpeg PPC/AltiVec and compiler auto-vectorisation stay disabled. */
#ifndef BLADES_SIMPLE360_INTRINSICS_H
#define BLADES_SIMPLE360_INTRINSICS_H
#include <altivec.h>
#include <stddef.h>
#include <stdint.h>
typedef vector unsigned char __vector4;
typedef vector unsigned short xb_uh;
typedef vector signed short xb_sh;
typedef vector unsigned int xb_uw;
typedef vector signed int xb_sw;
#define XB(x) ((__vector4)(x))
#define __declspec(x) __attribute__((aligned(16)))
#define __vspltisb(x) XB(vec_splats((signed char)(x)))
#define __vspltish(x) XB(vec_splats((signed short)(x)))
#define __vspltisw(x) XB(vec_splats((signed int)(x)))
#define __vxor(a,b) vec_xor(a,b)
#define __vor(a,b) vec_or(a,b)
#define __vsel(a,b,m) vec_sel(a,b,m)
#define __vadduhm(a,b) XB(vec_add((xb_uh)(a),(xb_uh)(b)))
#define __vsubuhm(a,b) XB(vec_sub((xb_uh)(a),(xb_uh)(b)))
#define __vadduwm(a,b) XB(vec_add((xb_uw)(a),(xb_uw)(b)))
#define __vsubuwm(a,b) XB(vec_sub((xb_uw)(a),(xb_uw)(b)))
#define __vslh(a,b) XB(vec_sl((xb_uh)(a),(xb_uh)(b)))
#define __vslw(a,b) XB(vec_sl((xb_uw)(a),(xb_uw)(b)))
#define __vsrah(a,b) XB(vec_sra((xb_sh)(a),(xb_uh)(b)))
#define __vsraw(a,b) XB(vec_sra((xb_sw)(a),(xb_uw)(b)))
#define __vmrghb(a,b) vec_mergeh(a,b)
#define __vmrglb(a,b) vec_mergel(a,b)
#define __vsldoi(a,b,n) vec_sld(a,b,n)
#define __vupkhsh(a) XB(vec_unpackh((xb_sh)(a)))
#define __vupklsh(a) XB(vec_unpackl((xb_sh)(a)))
#define __vpkswss(a,b) XB(vec_packs((xb_sw)(a),(xb_sw)(b)))
#define __vpkshus(a,b) XB(vec_packsu((xb_sh)(a),(xb_sh)(b)))
#define __vavgub(a,b) vec_avg(a,b)
#define __lvx(p,o) vec_ld((o),(const unsigned char *)(p))
#define __stvx(v,p,o) vec_st((v),(o),(unsigned char *)(p))
/* XDK lvlx/lvrx and stvlx/stvrx are expressed with the identical byte
 * alignment semantics using classic VMX loads, permutes and masked stores.
 * This avoids requiring a VMX128 assembler or unaligned C vector access. */
static inline __vector4 xb_loadu(const void *ptr)
{
    const unsigned char *p = ptr;
    return vec_perm(vec_ld(0,p),vec_ld(15,p),vec_lvsl(0,p));
}
static inline void xb_storeu(__vector4 v, void *ptr)
{
    unsigned char *p = ptr;
    if (!((uintptr_t)p & 15)) { vec_st(v,0,p); return; }
    __vector4 zero=vec_splats((unsigned char)0),ones=vec_splats((unsigned char)255);
    __vector4 shift=vec_lvsr(0,p),rotated=vec_perm(v,v,shift);
    __vector4 mask=vec_perm(zero,ones,shift);
    __vector4 first=vec_ld(0,p),last=vec_ld(15,p);
    vec_st(vec_sel(first,rotated,mask),0,p);
    vec_st(vec_sel(rotated,last,mask),15,p);
}
#endif
