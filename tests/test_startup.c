#include "blades.h"
#include "startup.h"
#include "draw_list.h"
#include "resources.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    const char *pack=argc>1?argv[1]:getenv("BLADES_RESOURCE_PACK");bool assets=pack&&resources_open(pack);
    /* The late cue must neither fire at launch nor wait for completion.
       Check both sides of the boundary and a stalled frame crossing it. */
    Dashboard timing;dashboard_begin_startup(&timing);
    dashboard_update(&timing,0,.999f);assert(timing.sound==UI_SOUND_NONE);
    dashboard_update(&timing,0,.002f);
    assert(timing.sound==UI_SOUND_STARTUP&&timing.startup_active);
    dashboard_update(&timing,0,.01f);assert(timing.sound==UI_SOUND_NONE);
    dashboard_begin_startup(&timing);dashboard_update(&timing,0,.95f);
    dashboard_update(&timing,0,.2f);
    assert(timing.sound==UI_SOUND_STARTUP&&timing.startup_active);
    dashboard_update(&timing,0,.1f);
    assert(timing.sound==UI_SOUND_NONE&&!timing.startup_active);
    Dashboard d;dashboard_begin_startup(&d);assert(d.startup_active&&d.blade==BLADE_GAMES&&!d.depth);
    dashboard_update(&d,SELECT|RIGHT,NAN);assert(d.startup_elapsed==0&&d.blade==BLADE_GAMES&&!d.depth);
    unsigned cues=0;for(unsigned i=0;i<90;i++) {
        dashboard_update(&d,d.startup_active?(SELECT|RIGHT|PROFILE|EXIT):0,1.f/60);
        cues+=d.sound==UI_SOUND_STARTUP;assert(d.blade==BLADE_GAMES&&!d.depth&&!d.dialog&&!d.exit_menu&&!d.exit_requested);
    }
    assert(!d.startup_active&&cues==1&&d.selected[BLADE_GAMES]==0);
    dashboard_update(&d,SELECT,1.f/60);assert(!d.depth&&d.profile_command.type==PROFILE_SIGN_IN); /* Achievements asks for sign-in. */
    dashboard_begin_startup(&d);dashboard_update(&d,SELECT,10);assert(!d.startup_active&&d.depth==0&&d.sound==UI_SOUND_STARTUP);
    dashboard_update(&d,0,10);assert(d.sound==UI_SOUND_NONE);
    /* Independently observed centre-row edges in the supplied 29.97fps video.
       Seven pixels cover antialiasing, curve reference point and frame phase. */
    const struct {float t,l,r;} observed[]={{.1001f,484,808},{.3003f,392,906},{.5005f,302,994},{.7674f,196,1095}};
    for(unsigned i=0;i<sizeof observed/sizeof observed[0];i++) {
        StartupFrame f=startup_sample(observed[i].t);
        assert(fabsf(f.left-observed[i].l)<7&&fabsf(f.right-observed[i].r)<7);
    }
    uint32_t *a=malloc((WIDTH*HEIGHT+2)*4),*b=malloc((WIDTH*HEIGHT+2)*4);assert(a&&b);
    a[0]=b[0]=0x123456;a[WIDTH*HEIGHT+1]=b[WIDTH*HEIGHT+1]=0xabcdef;
    Canvas direct={.pixels=a+1},replay={.pixels=b+1};DrawList list={0};
    dashboard_begin_startup(&d);
    for(unsigned frame=0;frame<=35;frame++) {
        d.startup_elapsed=frame/30.f;d.elapsed=d.startup_elapsed;
        dashboard_render(&direct,&d);assert(draw_list_update(&list,&d)&&list.valid);
        assert(draw_list_software(&list,&replay,d.elapsed));
        assert(!memcmp(direct.pixels,replay.pixels,WIDTH*HEIGHT*4));
    }
    /* Last startup pose must settle into the actual main root, not onboarding
       or a separate approximation of the root's chrome and profile content. */
    if(assets) {
        d.startup_elapsed=STARTUP_SECONDS;dashboard_render(&direct,&d);
        d.startup_active=false;dashboard_render(&replay,&d);
        if(memcmp(direct.pixels,replay.pixels,WIDTH*HEIGHT*4)) {
            unsigned count=0;for(int i=0;i<WIDTH*HEIGHT;i++)count+=direct.pixels[i]!=replay.pixels[i];
            fprintf(stderr,"startup endpoint differs at %u pixels\n",count);
        }
        assert(!memcmp(direct.pixels,replay.pixels,WIDTH*HEIGHT*4));
        ui_audio_init();size_t bytes;assert(resources_blob("audio/menu-close.wav",&bytes));size_t frames=(bytes-44)/4;
        float *first=malloc(frames*2*sizeof(float)),*second=malloc(frames*2*sizeof(float));assert(first&&second);
        ui_audio_play(UI_SOUND_STARTUP);ui_audio_render(first,frames);
        ui_audio_init();ui_audio_play(UI_SOUND_MENU_CLOSE);ui_audio_render(second,frames);
        assert(!memcmp(first,second,frames*2*sizeof(float)));free(first);free(second);ui_audio_close();
    }
    assert(a[0]==0x123456&&b[0]==0x123456&&a[WIDTH*HEIGHT+1]==0xabcdef&&b[WIDTH*HEIGHT+1]==0xabcdef);
    draw_list_close(&list);resources_close();free(a);free(b);
    puts("PASS: startup timing, one-shot whoosh, input gating, measured shutter edges, 36 bounded command frames and main-menu landing");
}
