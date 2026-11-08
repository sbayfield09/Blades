#define _DEFAULT_SOURCE
#include "video.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char **argv){
 assert(argc==2);VideoPlayer *p=video_create(&(VideoConfig){.channels=2,.audio_available=true,.gpu_yuv=true});assert(p&&video_play(p,argv[1]));
 double deadline=now()+8;unsigned heard=0,shown=0;bool bypass=false;VideoSnapshot s={0};
 while(now()<deadline){
  float pcm[512];video_audio_render(p,pcm,256);for(unsigned i=0;i<512;i++)heard+=fabsf(pcm[i])>.001f;
  video_poll(p,&s);assert(s.state!=VIDEO_FAILED);bypass|=s.audio_preroll;
  VideoPicture f;if(video_take_picture(p,&f)){assert(f.pts>=4.9);assert(s.position+.05>=f.pts);shown++;video_release_picture(p,f.slot,f.revision);}
  if(s.state==VIDEO_ENDED)break;
  usleep(1000);
 }
 assert(s.state==VIDEO_ENDED&&bypass&&heard>100&&shown>10);video_close(p);
 puts("PASS: audio-only lead breaks bounded prebuffer dependency; first picture remains timestamped, playback reaches EOF");
}
