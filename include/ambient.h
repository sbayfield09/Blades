#ifndef BLADES_AMBIENT_H
#define BLADES_AMBIENT_H
#include "blades.h"
typedef struct {uint64_t draw_us;unsigned draws,updates;} AmbientStats;
void ambient_draw(Canvas *canvas,int blade,float seconds);
void ambient_profile(bool enabled);
void ambient_stats(AmbientStats *stats);
#endif
