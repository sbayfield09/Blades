#define _DEFAULT_SOURCE
#include "video.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void line(const char *s){puts(s);}
static void pump(VideoPlayer *p){float audio[512];video_audio_render(p,audio,256);for(unsigned i=0;i<512;i++)assert(isfinite(audio[i]));usleep(1000);}
static VideoPicture picture(VideoPlayer *p){
 double deadline=now()+5;VideoPicture f;
 while(now()<deadline){if(video_take_picture(p,&f)){assert(f.format==VIDEO_RGBA&&f.width==1280&&f.height==720&&f.rgba[3]==255);return f;}pump(p);VideoSnapshot s;video_poll(p,&s);assert(s.state!=VIDEO_FAILED);}
 assert(!"first video frame timed out");return (VideoPicture){0};
}
static unsigned hash(VideoPicture f){unsigned h=2166136261u;for(unsigned i=0;i<f.width*f.height*4;i+=97)h=(h^f.rgba[i])*16777619u;return h;}
static void stop(VideoPlayer *p){video_stop(p);double deadline=now()+5;VideoSnapshot s;do{pump(p);video_poll(p,&s);}while(s.state!=VIDEO_IDLE&&now()<deadline);assert(s.state==VIDEO_IDLE);}
int main(int argc,char **argv){
 assert(argc==3);
 /* The explicit RGBA fallback must still work when switching videos before
    rendering releases the previous held frame. */
 VideoPlayer *p=video_create(&(VideoConfig){.channels=2,.audio_available=true,.gpu_yuv=false,.log=line});assert(p);
 assert(video_play(p,argv[1]));VideoPicture held=picture(p);unsigned original=hash(held);stop(p);
 assert(hash(held)==original&&video_play(p,argv[2]));VideoPicture next=picture(p);assert(next.slot!=held.slot&&hash(held)==original);
 video_release_picture(p,held.slot,held.revision);video_release_picture(p,next.slot,next.revision);stop(p);
 for(unsigned file=1;file<3;file++){
  assert(video_play(p,argv[file]));unsigned frames=0;double deadline=now()+8;VideoSnapshot s;
  do{pump(p);VideoPicture f;if(video_take_picture(p,&f)){assert(f.format==VIDEO_RGBA&&f.width==1280&&f.height==720&&f.rgba[3]==255);frames++;video_release_picture(p,f.slot,f.revision);}video_poll(p,&s);assert(s.state!=VIDEO_FAILED);}while(s.state!=VIDEO_ENDED&&now()<deadline);
  assert(s.state==VIDEO_ENDED&&frames>30);printf("PASS: RGBA fallback file%u frames=%u decoder_frames=%u\n",file,frames,s.decoded);stop(p);
 }
 for(unsigned i=0;i<20;i++){assert(video_play(p,argv[1]));video_stop(p);}stop(p);video_close(p);
 puts("PASS: explicit RGBA fallback, both HD codecs display, held frame survives stop/reopen, EOF and rapid cancellation finish");
}
