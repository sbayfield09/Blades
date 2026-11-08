#include "blades.h"
#include "animation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef struct {float seconds,top,bottom;} Observation;
int main(void) {
    /* Independently sampled 720p edges from the 29.97 fps reference, aligned
       to the last unchanged content frame. Tolerance covers a captured frame
       and soft/rounded texture edges; these are not implementation constants. */
    const Observation open[]={
        {.0667f,94,594},{.1001f,129,575},{.1335f,163,555},
        {.1668f,198,536},{.2002f,234,516},{.2670f,194,538},
        {.3337f,144,566},{.4338f,68,608},{.4672f,51,618},{.5339f,66,609}};
    const Observation close[]={
        {.1334f,108,586},{.1668f,146,565},{.2002f,186,543},
        {.2335f,223,522},{.2669f,227,520},{.3337f,182,545},
        {.4337f,116,582},{.5005f,67,609},{.5338f,50,619}};
    const Observation blade[]={
        {.2336f,186,480},{.2670f,166,501},{.3003f,146,520},
        {.3337f,127,540},{.3670f,107,559},{.4004f,88,579},
        {.4338f,68,599},{.4671f,50,619},{.5005f,60,613}};
    for(unsigned kind=0;kind<3;kind++) {
        const Observation *observed=kind==0?open:kind==1?close:blade;
        unsigned count=kind==0?sizeof open/sizeof open[0]:kind==1?sizeof close/sizeof close[0]:sizeof blade/sizeof blade[0];
        for(unsigned n=0;n<count;n++) {
            AnimationCaps c=anim_caps(observed[n].seconds,kind<2,kind==1);
            assert(fabsf(c.top-observed[n].top)<18);
            assert(fabsf(c.bottom-observed[n].bottom)<18);
        }
    }
    assert(fabsf(anim_menu_depth(.1f,0,1)-.122f)<.05f);
    assert(fabsf(anim_menu_depth(.267f,0,1)-.469f)<.05f);
    assert(fabsf(anim_menu_depth(.434f,0,1)-.796f)<.05f);
    assert(fabsf(anim_menu_depth(.334f,1,0)-.55f)<.05f);
    for(int next=0;next<=1;next++) {
        float previous=(float)(1-next);
        for(unsigned frame=0;frame<80;frame++) {
            float depth=anim_menu_depth(frame/60.0f,1-next,next);
            assert(depth>=0&&depth<=1);
            assert(next?depth>=previous:depth<=previous);previous=depth;
        }
    }
    Dashboard d;dashboard_init(&d);d.selected[BLADE_GAMES]=1;dashboard_update(&d,SELECT,0);
    for(unsigned n=0;n<5;n++)dashboard_update(&d,0,.1f);
    assert(d.depth==1&&d.menu_transition<1); /* Old 420 ms shortcut must not return. */
    for(unsigned n=0;n<5;n++)dashboard_update(&d,0,.1f);
    assert(d.menu_transition==1);
    /* Every media library uses the same continuous cursor as ordinary rows. */
    const LibraryPage pages[]={LIB_UI_LIST,LIB_UI_ROOTS,LIB_UI_BROWSE,LIB_UI_CATEGORY,LIB_UI_ROOT_ACTION,LIB_UI_DEPTH,LIB_UI_FILES,LIB_UI_VIDEO_SOURCES};
    for(unsigned i=0;i<sizeof pages/sizeof pages[0];i++){
        dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;d.depth=1;dashboard_library_open(&d,pages[i],LIB_VIDEO);d.library_row=1;
        assert(d.highlight==0&&d.list_transition==0);dashboard_update(&d,0,.016f);assert(d.highlight>0&&d.highlight<1&&d.list_transition>0&&d.list_transition<1);
        for(unsigned frame=0;frame<30;frame++)dashboard_update(&d,0,.016f);assert(d.highlight>.99f&&d.list_transition==1);
    }
    dashboard_init(&d);d.blade=BLADE_MEDIA;d.depth=1;d.jelly_page=JELLY_UI_BROWSE;d.jelly_row=1;dashboard_update(&d,0,.016f);assert(d.highlight>0&&d.highlight<1);
    puts("PASS: measured panel edges, linear rail motion, overshoot and complete submenu timing");
}
