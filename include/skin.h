#ifndef BLADES_SKIN_H
#define BLADES_SKIN_H
#include "blades.h"
bool skin_frame(Canvas *c,int blade,float depth,float header,float footer);
bool skin_chrome(Canvas *c,int blade,float depth,bool closing);
bool skin_leaf(Canvas *c,int from,int to,float progress);
bool skin_tray(Canvas *c,int blade,bool selected,const char *label);
bool skin_startup_base(Canvas *c,float seconds);
bool skin_startup_shutters(Canvas *c,float seconds);
#endif
