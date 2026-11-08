#define _DEFAULT_SOURCE
#include "blades.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void log_line(const char *s){puts(s);fflush(stdout);}
static unsigned audio_samples,pictures;
static VideoSnapshot pump(VideoPlayer *v,bool take){
 float pcm[256*VIDEO_AUDIO_CHANNELS];video_audio_render(v,pcm,256);
 for(unsigned i=0;i<256*video_audio_channels(v);i++){assert(isfinite(pcm[i]));audio_samples+=fabsf(pcm[i])>.00001f;}
 VideoPicture f;if(take&&video_take_picture(v,&f)){
  assert(f.rgba&&f.width==640&&f.height==360&&(f.format!=VIDEO_RGBA||f.rgba[3]==255));assert(f.aspect>1.7&&f.aspect<1.9);pictures++;
  video_release_picture(v,f.slot,f.revision);
 }
 VideoSnapshot s;video_poll(v,&s);if(s.state==VIDEO_FAILED)fprintf(stderr,"unexpected failure: %s\n",s.error);
 usleep(1000);return s;
}
static VideoSnapshot wait_for(VideoPlayer *v,VideoState state){
 double deadline=now()+5;VideoSnapshot s;do{s=pump(v,true);if(s.state==state)return s;}while(now()<deadline);
 fprintf(stderr,"timed out: wanted %d, got %d pos %.2f\n",state,s.state,s.position);assert(0);return s;
}
static void stop(VideoPlayer *v){video_stop(v);wait_for(v,VIDEO_IDLE);}
static unsigned checksum(const VideoPicture *f){unsigned hash=2166136261u;for(unsigned i=0;i<f->width*f->height*(f->format==VIDEO_RGBA?4:3)/ (f->format==VIDEO_RGBA?1:2);i+=97)hash=(hash^f->rgba[i])*16777619u;return hash;}
static void lifecycle(VideoPlayer *v,const char *path){
 assert(video_play(v,path));wait_for(v,VIDEO_PLAYING);
 double deadline=now()+4;VideoPicture held;bool got=false;
 while(now()<deadline){if(video_take_picture(v,&held)){got=true;break;}pump(v,false);}assert(got);
 unsigned hash=checksum(&held);for(unsigned i=0;i<80;i++)pump(v,true);assert(hash==checksum(&held));
 video_release_picture(v,held.slot,held.revision+1);assert(hash==checksum(&held));
 video_pause(v,true);VideoSnapshot paused;video_poll(v,&paused);assert(paused.state==VIDEO_PAUSED);
 for(unsigned i=0;i<20;i++)pump(v,true);
 VideoSnapshot s;video_poll(v,&s);assert(fabs(s.position-paused.position)<.0001);
 float silent[256*VIDEO_AUDIO_CHANNELS];video_audio_render(v,silent,256);for(unsigned i=0;i<256*video_audio_channels(v);i++)assert(silent[i]==0);
 video_seek(v,5);deadline=now()+3;do{s=pump(v,true);}while(now()<deadline&&s.position<paused.position+4.8);
 assert(s.state==VIDEO_PAUSED&&s.position>=paused.position+4.8);
 assert(hash==checksum(&held));video_release_picture(v,held.slot,held.revision);
 video_pause(v,false);video_seek(v,-3);for(unsigned i=0;i<100;i++)pump(v,true);
 video_seek(v,1000);s=wait_for(v,VIDEO_ENDED);assert(s.position>11&&s.position<13&&s.decoded>1);
 assert(pictures>3&&audio_samples>100);stop(v);
 /* Cancelling while opening must never leave the coordinator stuck. */
 for(unsigned i=0;i<30;i++){assert(video_play(v,path));video_stop(v);}wait_for(v,VIDEO_IDLE);
 for(unsigned i=0;i<3;i++){assert(video_play(v,path));wait_for(v,VIDEO_PLAYING);for(unsigned j=0;j<12;j++)pump(v,true);stop(v);}
 assert(video_play(v,"/does/not/exist/video.wmv"));s=wait_for(v,VIDEO_FAILED);assert(s.error[0]);stop(v);
}
static void ui(VideoPlayer *v,const char *path){
 Dashboard d;dashboard_init(&d);d.blade=BLADE_MEDIA;d.depth=1;d.selected[1]=2;
 assert(dashboard_video_open(&d,v,path));
 double deadline=now()+3;while(!d.video_has_picture&&now()<deadline){pump(v,false);dashboard_video_update(&d,v,0,.001,false);}assert(d.video_has_picture);
 assert(dashboard_video_update(&d,v,SELECT,.016,false));assert(d.video.state==VIDEO_PAUSED);
 dashboard_video_update(&d,v,0,.016,true);dashboard_video_update(&d,v,0,.016,false);assert(d.video.state==VIDEO_PAUSED);
 dashboard_video_update(&d,v,SELECT,.016,false);assert(d.video.state==VIDEO_PLAYING);
 dashboard_video_update(&d,v,0,.016,true);assert(d.video.state==VIDEO_PAUSED);
 dashboard_video_update(&d,v,0,.016,false);assert(d.video.state==VIDEO_PLAYING);
 assert(dashboard_video_update(&d,v,BACK,.016,false));assert(!d.video_view&&!d.video_has_picture&&d.depth==1&&d.selected[1]==2&&d.blade==BLADE_MEDIA);stop(v);
}
static void hd(const char *path,unsigned width,unsigned height){
 VideoConfig config={.channels=2,.audio_available=true,.gpu_yuv=true,.log=log_line};VideoPlayer *v=video_create(&config);assert(v&&video_play(v,path));
 double start=now();VideoSnapshot s={0};unsigned shown=0;double preparation=0;
 while(now()-start<10){
  float audio[512];video_audio_render(v,audio,256);VideoPicture f;
  if(video_take_picture(v,&f)){assert(f.width==width&&f.height==height&&f.format==VIDEO_YUV709);assert(checksum(&f));video_release_picture(v,f.slot,f.revision);shown++;}
  video_poll(v,&s);assert(s.state!=VIDEO_FAILED);preparation+=s.prepare_ms;
  if(s.state==VIDEO_ENDED)break;
  usleep(1000);
 }
 assert(s.state==VIDEO_ENDED&&shown>10&&s.decoded>=50);
 printf("PASS: native%ux%u YUV frames; decoded=%u presented=%u elapsed_host=%.3f prepare_last_ms=%.3f\n",width,height,s.decoded,shown,now()-start,s.prepare_ms);
 (void)preparation;video_close(v);
}
int main(int argc,char **argv){
 assert(argc==5);VideoConfig c={.channels=2,.audio_available=true,.log=log_line};VideoPlayer *v=video_create(&c);assert(v);
 lifecycle(v,argv[1]);lifecycle(v,argv[2]);ui(v,argv[1]);video_close(v);
 c.channels=6;v=video_create(&c);assert(v&&video_audio_channels(v)==6);assert(video_play(v,argv[2]));wait_for(v,VIDEO_PLAYING);for(unsigned i=0;i<100;i++)pump(v,true);stop(v);video_close(v);
 c.audio_available=false;v=video_create(&c);assert(v&&video_play(v,argv[2]));wait_for(v,VIDEO_PLAYING);video_seek(v,11);wait_for(v,VIDEO_ENDED);video_close(v);
 c.audio_available=true;c.channels=2;c.gpu_yuv=true;v=video_create(&c);assert(v);lifecycle(v,argv[1]);lifecycle(v,argv[2]);ui(v,argv[1]);video_close(v);
 hd(argv[3],1920,1080);hd(argv[4],1280,720);
 puts("PASS: WMV/H264 decode, WMA/AAC resampling, stereo/5.1, video-only clock, pause/Guide/seek/end, held-frame ownership, repeated opens, cancellation, missing file, chooser preserved");
}
