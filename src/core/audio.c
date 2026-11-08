#include "blades.h"
#include "resources.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { RATE=48000, VOICES=8, COMMANDS=32, MAX_FRAMES=RATE*2 };
typedef struct { unsigned char *pcm; unsigned frames; } Clip;
typedef struct { UiSound sound; unsigned position; } Voice;
static Clip clips[UI_SOUND_COUNT];
static Voice voices[VOICES];
static UiSound commands[COMMANDS];
static unsigned produced,consumed,next_voice;
static const char *const names[UI_SOUND_COUNT]={
    [UI_SOUND_MENU_OPEN]="audio/menu-open.wav",
    [UI_SOUND_MENU_CLOSE]="audio/menu-close.wav",
    [UI_SOUND_MARKETPLACE_LIVE]="audio/marketplace-live.wav",
    [UI_SOUND_LIVE_GAMES]="audio/live-games.wav",
    [UI_SOUND_GAMES_MEDIA]="audio/games-media.wav",
    [UI_SOUND_MEDIA_SYSTEM]="audio/media-system.wav",
    [UI_SOUND_STARTUP]="audio/menu-close.wav"
};
static unsigned le16(const unsigned char *p) {return (unsigned)p[0]|(unsigned)p[1]<<8;}
static uint32_t le32(const unsigned char *p) {return le16(p)|(uint32_t)le16(p+2)<<16;}
/* Bounded PCM16 stereo WAV reader. The importer resamples assets to 48 kHz. */
static Clip load_clip(const unsigned char *p,size_t size) {
    Clip empty={0};
    if(!p||size<12||size>MAX_FRAMES*4+4096||memcmp(p,"RIFF",4)||memcmp(p+8,"WAVE",4)||le32(p+4)!=size-8)return empty;
    bool format=false;const unsigned char *data=NULL;size_t bytes=0;
    for(size_t at=12;at<size;) {
        if(size-at<8)return empty;
        size_t n=le32(p+at+4),start=at+8;
        if(n>size-start||n+(n&1)>size-start)return empty;
        if(!memcmp(p+at,"fmt ",4)) {
            if(format||n<16||le16(p+start)!=1||le16(p+start+2)!=2||le32(p+start+4)!=RATE||
               le32(p+start+8)!=RATE*4||le16(p+start+12)!=4||le16(p+start+14)!=16)return empty;
            format=true;
        } else if(!memcmp(p+at,"data",4)) {
            if(data||!n||n%4||n/4>MAX_FRAMES)return empty;
            data=p+start;bytes=n;
        }
        at=start+n+(n&1);
    }
    if(!format||!data)return empty;
    unsigned char *copy=malloc(bytes);if(!copy)return empty;
    memcpy(copy,data,bytes);return (Clip){copy,(unsigned)(bytes/4)};
}
void ui_audio_close(void) {
    for(unsigned i=0;i<UI_SOUND_COUNT;i++){free(clips[i].pcm);clips[i]=(Clip){0};}
    memset(voices,0,sizeof voices);produced=consumed=next_voice=0;
}
void ui_audio_init(void) {
    ui_audio_close();
    for(unsigned i=0;i<UI_SOUND_COUNT;i++)if(names[i]) {
        size_t size=0;const unsigned char *p=resources_blob(names[i],&size);
        clips[i]=load_clip(p,size);
    }
}
void ui_audio_play(UiSound sound) {
    if(sound<=UI_SOUND_NONE||sound>=UI_SOUND_COUNT)return;
    unsigned head=__atomic_load_n(&produced,__ATOMIC_RELAXED);
    if(head-__atomic_load_n(&consumed,__ATOMIC_ACQUIRE)>=COMMANDS)return;
    commands[head%COMMANDS]=sound;
    __atomic_store_n(&produced,head+1,__ATOMIC_RELEASE);
}
/* Retained for missing-pack feedback; retail whooshes are never synthesised. */
float selection_sample(unsigned action,unsigned index) {
    if(index>=1536)return 0;
    float t=(float)index/RATE,envelope=1-(float)index/1536;
    float hz=(action&SELECT)?1040:(action&BACK)?520:780;
    return 0.065f*envelope*envelope*sinf(t*hz*6.2831853f);
}
void ui_audio_render(float *stereo,size_t frames) {
    unsigned tail=__atomic_load_n(&consumed,__ATOMIC_RELAXED);
    unsigned head=__atomic_load_n(&produced,__ATOMIC_ACQUIRE);
    while(tail!=head) {
        voices[next_voice++%VOICES]=(Voice){commands[tail%COMMANDS],0};tail++;
    }
    __atomic_store_n(&consumed,tail,__ATOMIC_RELEASE);
    for(size_t i=0;i<frames;i++) {
        float sum[2]={0};
        for(unsigned v=0;v<VOICES;v++) {
            Voice *voice=&voices[v];if(voice->sound==UI_SOUND_NONE)continue;
            Clip *clip=&clips[voice->sound];
            unsigned length=clip->pcm?clip->frames:1536;
            if(voice->position>=length){voice->sound=UI_SOUND_NONE;continue;}
            for(unsigned ch=0;ch<2;ch++) {
                if(clip->pcm) {
                    unsigned raw=le16(clip->pcm+voice->position*4+ch*2);
                    int sample=raw>32767?(int)raw-65536:(int)raw;
                    sum[ch]+=(float)sample*(0.7f/32768);
                } else {
                    unsigned action=voice->sound==UI_SOUND_BACK||voice->sound==UI_SOUND_MENU_CLOSE?BACK:
                        voice->sound==UI_SOUND_SELECT||voice->sound==UI_SOUND_MENU_OPEN?SELECT:DOWN;
                    sum[ch]+=selection_sample(action,voice->position);
                }
            }
            voice->position++;
        }
        for(unsigned ch=0;ch<2;ch++)stereo[i*2+ch]=fmaxf(-1,fminf(1,sum[ch]));
    }
}
