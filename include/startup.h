#ifndef BLADES_STARTUP_H
#define BLADES_STARTUP_H
#include "animation.h"
#include "blades.h"
#include <math.h>
#define STARTUP_SECONDS (35.0f/30)
#define STARTUP_SOUND_TIME 1.0f
typedef struct {float time,value;} StartupKey;
typedef struct {
    float left,right,center,glow,content,shutters,runners,offset[BLADE_COUNT];
} StartupFrame;
static inline float startup_curve(float seconds,const StartupKey *keys,unsigned count) {
    if(seconds<=keys[0].time)return keys[0].value;
    for(unsigned i=1;i<count;i++)if(seconds<keys[i].time) {
        float t=anim_phase(seconds,keys[i-1].time,keys[i].time);
        return keys[i-1].value+(keys[i].value-keys[i-1].value)*t;
    }
    return keys[count-1].value;
}
/* Shutter positions measured at 720p in the supplied 29.97 fps recording.
   Side-leaf offsets adapt DashX360's startup timeline, pinned
   0c8adb2cef4ad7dff2bf889351efa1f1c244aef2 (1920x1080 coordinates).
   Keep the short opening and late content reveal; no console boot video. */
static inline StartupFrame startup_sample(float seconds) {
    if(!isfinite(seconds)||seconds<0)seconds=0;
    static const StartupKey left[]={
        {0.f/30,640},{1.f/30,518},{2.f/30,502},{3.f/30,484},{4.f/30,476},
        {5.f/30,459},{6.f/30,442},{7.f/30,425},{8.f/30,398},{9.f/30,392},
        {10.f/30,367},{11.f/30,351},{12.f/30,336},{13.f/30,331},{14.f/30,316},
        {15.f/30,302},{16.f/30,278},{17.f/30,274},{18.f/30,261},{19.f/30,247},
        {20.f/30,234},{21.f/30,212},{22.f/30,208},{23.f/30,196},{24.f/30,183},
        {25.f/30,170},{26.f/30,149},{27.f/30,146},{28.f/30,134},{29.f/30,122},
        {35.f/30,70}};
    static const StartupKey right[]={
        {0.f/30,640},{1.f/30,775},{2.f/30,789},{3.f/30,808},{4.f/30,836},
        {5.f/30,853},{6.f/30,869},{7.f/30,885},{8.f/30,891},{9.f/30,906},
        {10.f/30,921},{11.f/30,947},{12.f/30,951},{13.f/30,965},{14.f/30,989},
        {15.f/30,994},{16.f/30,1007},{17.f/30,1020},{18.f/30,1033},{19.f/30,1046},
        {20.f/30,1058},{21.f/30,1071},{22.f/30,1083},{23.f/30,1095},{24.f/30,1107},
        {25.f/30,1119},{26.f/30,1131},{27.f/30,1142},{28.f/30,1154},{29.f/30,1165},
        {35.f/30,1167}};
    static const StartupKey green[]={{0,.18f},{.1f,.26f},{.2f,.36f},{.3f,.55f},{.4f,.72f},{.5f,.92f},{17.f/30,1}};
    static const StartupKey content[]={{30.f/30,0},{31.f/30,.12f},{32.f/30,.36f},{33.f/30,.57f},{34.f/30,.83f},{35.f/30,1}};
    static const StartupKey games[]={
        {17.f/30,210},{18.f/30,205},{19.f/30,203},{20.f/30,194},{21.f/30,167},
        {22.f/30,138},{23.f/30,111},{24.f/30,97},{25.f/30,87},{26.f/30,78},
        {27.f/30,68},{28.f/30,59},{29.f/30,49},{30.f/30,39},{31.f/30,30},
        {32.f/30,20},{33.f/30,12},{34.f/30,3},{35.f/30,0}};
    static const StartupKey media[]={{25.f/30,20},{1,12},{32.f/30,7},{33.f/30,4},{34.f/30,1},{35.f/30,0}};
    static const StartupKey system[]={{.9f,80},{1,68},{32.f/30,30},{34.f/30,7},{35.f/30,0}};
    static const StartupKey live[]={{.6f,-110},{.7f,-85},{23.f/30,-38},{.8f,-15},{25.f/30,0}};
    static const StartupKey market[]={{.8f,-60},{1,-42},{32.f/30,-26},{34.f/30,-9},{35.f/30,0}};
#define SAMPLE(keys) startup_curve(seconds,keys,sizeof(keys)/sizeof((keys)[0]))
    StartupFrame f={.left=SAMPLE(left),.right=SAMPLE(right),
        .center=anim_smooth(anim_phase(seconds,.36f,.66f)),.glow=1-SAMPLE(green),
        .content=SAMPLE(content),.shutters=1-anim_smooth(anim_phase(seconds,31.f/30,35.f/30)),
        .runners=anim_clamp(anim_phase(seconds,23.f/30,24.f/30))};
    f.offset[BLADE_GAMES]=SAMPLE(games)*(2.f/3);
    f.offset[BLADE_MEDIA]=fmaxf(SAMPLE(games)-71,SAMPLE(media))*(2.f/3);
    f.offset[BLADE_SYSTEM]=SAMPLE(system)*(2.f/3);
    f.offset[BLADE_LIVE]=SAMPLE(live)*(2.f/3);
    f.offset[BLADE_MARKETPLACE]=SAMPLE(market)*(2.f/3);
#undef SAMPLE
    return f;
}
#endif
