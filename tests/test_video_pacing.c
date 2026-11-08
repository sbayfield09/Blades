#define _DEFAULT_SOURCE
#include "video.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void log_line(const char *s){puts(s);fflush(stdout);}
static void audio_due(VideoPlayer *v,double *next,bool *heard){
 double time=now();while(*next<=time){float pcm[512];video_audio_render(v,pcm,256);for(unsigned i=0;i<512;i++)if(fabsf(pcm[i])>.0001f)*heard=true;*next+=256./48000;}
}
int main(int argc,char **argv){
 assert(argc==3);bool startup=!strcmp(argv[1],"startup");
 VideoPlayer *v=video_create(&(VideoConfig){.channels=2,.audio_available=true,.gpu_yuv=true,.log=log_line});assert(v&&video_play(v,argv[2]));
 double start=now(),next=start,last_picture=0,first=0,worst_gap=0;unsigned pictures=0,max_catchup=0;bool heard=false;
 while(now()-start<3.2){
  audio_due(v,&next,&heard);
  if(startup&&now()-start<.35){
   assert(!heard&&"PCM played before renderer received its first picture");
  }else{
   VideoPicture p;if(video_take_picture(v,&p)){
    double at=now();if(!first)first=at;else if(at-last_picture>worst_gap)worst_gap=at-last_picture;
    last_picture=at;pictures++;video_release_picture(v,p.slot,p.revision);
   }
  }
  VideoSnapshot s;video_poll(v,&s);assert(s.state!=VIDEO_FAILED);
  if(s.catchup_level>max_catchup)max_catchup=s.catchup_level;
  if(!startup&&first&&now()-first>.5)assert(now()-last_picture<.25&&"decoder continues, but late-frame discard freezes presentation");
  usleep(1000);
 }
 assert(first&&heard&&pictures>=45&&worst_gap<.25);
 if(!startup)assert(max_catchup>=1&&"slow decoder must activate Simple360 catch-up");
 printf("PASS: %s, pictures=%u max_gap=%.3f max_catchup=%u, slow decode40ms/frame remains visible\n",argv[1],pictures,worst_gap,max_catchup);
 video_close(v);
}
