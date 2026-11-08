#include "blades.h"
#include "resources.h"
#include "audio_boundary.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void le16(unsigned char *p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void le32(unsigned char *p,unsigned n){le16(p,n);le16(p+2,n>>16);}
static void be32(unsigned char *p,unsigned n){p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned char *fixture(size_t *size,size_t *wav_at) {
    const char *name="audio/games-media.wav";size_t chars=strlen(name),table=9+chars*2;
    size_t wav_size=44+4000*4;*size=22+table+wav_size;*wav_at=22+table;
    unsigned char *p=calloc(*size,1);assert(p);
    memcpy(p,"XUIZ",4);be32(p+4,1);be32(p+8,(unsigned)*size);be32(p+16,(unsigned)table);p[21]=1;
    be32(p+22,(unsigned)wav_size);p[30]=(unsigned char)chars;
    for(size_t i=0;i<chars;i++)p[32+i*2]=(unsigned char)name[i];
    unsigned char *w=p+*wav_at;
    memcpy(w,"RIFF",4);le32(w+4,(unsigned)wav_size-8);memcpy(w+8,"WAVEfmt ",8);le32(w+16,16);
    le16(w+20,1);le16(w+22,2);le32(w+24,48000);le32(w+28,192000);le16(w+32,4);le16(w+34,16);
    memcpy(w+36,"data",4);le32(w+40,16000);
    for(unsigned i=0;i<4000;i++){le16(w+44+i*4,8192);le16(w+46+i*4,49152);}
    return p;
}
static void render_constant(unsigned count,float left,float right) {
    float samples[514];samples[0]=123;samples[513]=456;
    while(count){unsigned n=count>256?256:count;ui_audio_render(samples+1,n);
        for(unsigned i=0;i<n;i++){assert(fabsf(samples[i*2+1]-left)<1e-6f);assert(fabsf(samples[i*2+2]-right)<1e-6f);}
        assert(samples[0]==123&&samples[513]==456);count-=n;}
}
static void press(Dashboard *d,unsigned a){dashboard_update(d,a,1.0f/60);}
static void boundary(void){
    AudioBoundary state={0};float samples[512],previous=0;
    for(unsigned block=0;block<16;block++){
        for(unsigned i=0;i<256;i++){samples[i*2]=1;samples[i*2+1]=-1;}
        audio_boundary_apply(&state,samples,256,false);
        for(unsigned i=0;i<256;i++){float value=samples[i*2];assert(value>=previous&&value<=1&&value-previous<=1.f/2048+1e-6f);assert(samples[i*2+1]==-value);previous=value;}
    }
    assert(previous==1);unsigned silent=0;
    for(unsigned block=0;block<26;block++){
        for(unsigned i=0;i<256;i++){samples[i*2]=1;samples[i*2+1]=-1;}
        audio_boundary_apply(&state,samples,256,true);
        for(unsigned i=0;i<256;i++){float value=samples[i*2];assert(value>=0&&value<=previous&&previous-value<=1.f/2048+1e-6f);if(value==0)silent++;previous=value;}
    }
    assert(state.finished&&previous==0&&silent>=4096);
}
int main(int argc,char **argv) {
    boundary();
    Dashboard d;
    for(int from=0;from<BLADE_COUNT;from++)for(int direction=-1;direction<=1;direction+=2) {
        dashboard_init(&d);d.blade=from;press(&d,direction<0?LEFT:RIGHT);
        int to=from+direction;
        assert(d.sound==(to<0||to>=BLADE_COUNT?UI_SOUND_NONE:
            (UiSound)(UI_SOUND_MARKETPLACE_LIVE+(from<to?from:to))));
        press(&d,0);assert(d.sound==UI_SOUND_NONE);
    }
    dashboard_init(&d);d.blade=BLADE_SYSTEM;press(&d,BACK);assert(d.sound==UI_SOUND_NONE);
    press(&d,SELECT);assert(d.depth==1&&d.sound==UI_SOUND_MENU_OPEN);
    press(&d,SELECT);assert(d.dialog&&d.sound==UI_SOUND_SELECT);
    press(&d,RIGHT);assert(d.dialog&&d.sound==UI_SOUND_NONE);
    press(&d,BACK);assert(!d.dialog&&d.sound==UI_SOUND_BACK);
    press(&d,BACK);assert(!d.depth&&d.sound==UI_SOUND_MENU_CLOSE);
    size_t size,at;unsigned char *p=fixture(&size,&at);
    assert(resources_open_memory(p,size));ui_audio_init();resources_close();
    ui_audio_play(UI_SOUND_GAMES_MEDIA);render_constant(4000,.175f,-.35f);render_constant(400,0,0);
    /* Overlap preserves both channels and clamps excessive sums. */
    for(unsigned i=0;i<8;i++)ui_audio_play(UI_SOUND_GAMES_MEDIA);
    render_constant(4000,1,-1);render_constant(16,0,0);
    /* Bad lengths, format, channels and rates must use the short fallback. */
    const unsigned offsets[]={0,4,16,20,22,24,28,32,34,36,40};
    for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];i++) {
        unsigned offset=offsets[i];unsigned char saved=p[at+offset];p[at+offset]^=0x40;
        assert(resources_open_memory(p,size));ui_audio_init();ui_audio_play(UI_SOUND_GAMES_MEDIA);
        float discard[3072];ui_audio_render(discard,1536);render_constant(500,0,0);
        p[at+offset]=saved;
    }
    free(p);ui_audio_close();resources_close();
    /* Queue overflow and absent assets remain bounded, finite and silent at end. */
    ui_audio_init();for(unsigned i=0;i<100;i++)ui_audio_play(UI_SOUND_MENU_OPEN);
    ui_audio_play((UiSound)-1);ui_audio_play(UI_SOUND_COUNT);
    float output[4096];ui_audio_render(output,2048);
    for(unsigned i=0;i<4096;i++)assert(isfinite(output[i])&&fabsf(output[i])<=1);
    render_constant(256,0,0);ui_audio_close();
    if(argc==2) {
        assert(resources_open(argv[1]));ui_audio_init();
        const char *names[]={"menu-open","menu-close","marketplace-live","live-games","games-media","media-system"};
        for(unsigned c=0;c<6;c++) {
            char key[80];snprintf(key,sizeof key,"audio/%s.wav",names[c]);size_t bytes;
            const unsigned char *wav=resources_blob(key,&bytes);assert(wav&&bytes>44&&memcmp(wav+36,"data",4)==0);
            ui_audio_play((UiSound)(UI_SOUND_MENU_OPEN+c));unsigned frames=(unsigned)(bytes-44)/4;
            for(unsigned i=0;i<frames;i++) {
                float pair[2];ui_audio_render(pair,1);
                for(unsigned ch=0;ch<2;ch++) {
                    const unsigned char *s=wav+44+i*4+ch*2;int value=s[0]+s[1]*256;if(value>=32768)value-=65536;
                    assert(fabsf(pair[ch]-value*(.7f/32768))<1e-6f);
                }
            }
            render_constant(256,0,0);
        }
        ui_audio_close();resources_close();
    }
    puts("PASS: transition events, stereo PCM and full tails, overlap, malformed WAV, owned samples and queue bounds");
}
