#ifndef BLADES_PIXEL_OPS_H
#define BLADES_PIXEL_OPS_H
#include <stddef.h>
#include <stdint.h>
/* Per-channel ARGB interpolation, floor((a*(256-weight)+b*weight)/256).
   Vector paths accept aligned input rows; unaligned rows retain a scalar path. */
void pixels_lerp_row(uint32_t *out,const uint32_t *a,const uint32_t *b,unsigned weight,size_t count);
const char *pixels_lerp_backend(void);
#endif
