#ifndef BLADES_ANIMATION_H
#define BLADES_ANIMATION_H
#include <stdbool.h>
/* Sampled from the supplied 5766 capture at 30000/1001 fps. Event onset is
   bounded by adjacent captured frames (33.37 ms), not a recovered XUI timer. */
#define BLADE_SECONDS 0.55f
#define MENU_SECONDS 0.94f
#define BLADE_LEAF_START (1.0f/30)
#define BLADE_LEAF_END 0.20f
#define CONTENT_OUT (1.0f/30)
#define MENU_CONTENT_START (17.0f/30)
#define MENU_CONTENT_END (28.0f/30)
static inline float anim_clamp(float t) {return t<0?0:t>1?1:t;}
static inline float anim_smooth(float t) {t=anim_clamp(t);return t*t*(3-2*t);}
static inline float anim_phase(float s,float from,float to) {return anim_clamp((s-from)/(to-from));}
typedef struct {float top,bottom;} AnimationCaps;
typedef struct {float time,top,bottom;} AnimationCapKey;
static inline AnimationCaps anim_caps(float seconds,bool menu,bool closing) {
    static const AnimationCapKey blade[]={
        {.20f,206,461},{13.0f/30,68,610},{14.0f/30,51,620},{16.0f/30,68,610}};
    static const AnimationCapKey open[]={
        {.04f,68,610},{.20f,240,514},{13.0f/30,68,610},{14.0f/30,51,619},{16.0f/30,68,610}};
    static const AnimationCapKey close[]={
        {.10f,68,610},{.25f,240,514},{.50f,68,610},{16.0f/30,51,619},{.60f,68,610}};
    const AnimationCapKey *keys=menu?(closing?close:open):blade;
    unsigned count=menu?5:4;
    AnimationCapKey a=keys[0];
    for(unsigned i=1;i<count;i++) {
        AnimationCapKey b=keys[i];
        if(seconds<b.time) {
            float t=anim_phase(seconds,a.time,b.time);
            return (AnimationCaps){a.top+(b.top-a.top)*t,a.bottom+(b.bottom-a.bottom)*t};
        }
        a=b;
    }
    return (AnimationCaps){a.top,a.bottom};
}
static inline float anim_menu_depth(float seconds,int previous,int next) {
    float start=next?1.0f/30:.10f,end=next?16.0f/30:.60f;
    return previous+(next-previous)*anim_phase(seconds,start,end);
}
#endif
