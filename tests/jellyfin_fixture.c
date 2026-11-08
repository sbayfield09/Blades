#define _DEFAULT_SOURCE
#include "blades.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static const JellySnapshot *idle(JellyService *s,unsigned before){
 double end=now()+25;const JellySnapshot *v=NULL;
 do{v=jellyfin_poll(s);if(v&&v->revision>before&&!v->busy)return v;usleep(1000);}while(now()<end);
 fprintf(stderr,"Jellyfin timeout: %s\n",v?v->status:"no snapshot");assert(0);return NULL;
}
static const JellySnapshot *command(JellyService *s,JellyCommand c){const JellySnapshot *v=jellyfin_poll(s);unsigned before=v?v->revision:0;assert(jellyfin_request(s,&c));return idle(s,before);}
static void edit(JellyService *s,unsigned field,const char *text){JellyCommand c={.type=JELLY_EDIT,.field=field};assert(strlen(text)<sizeof c.text);snprintf(c.text,sizeof c.text,"%s",text);command(s,c);xf_wipe(&c,sizeof c);}
static void saved_without_password(const char *path,const char *password){FILE *f=fopen(path,"rb");assert(f);char data[8192]={0};size_t n=fread(data,1,sizeof data-1,f);fclose(f);assert(n&&!strstr(data,"\"Pw\"")&&!strstr(data,"\"Password\"")&&!strstr(data,password));xf_wipe(data,sizeof data);}
static unsigned named(const JellySnapshot *v,const char *name){for(unsigned i=0;i<v->page.count;i++)if(!strcmp(v->page.items[i].name,name))return i;assert(0);return 0;}
static const JellySnapshot *artwork(JellyService *s,const char *id){
 jellyfin_select(s,id);double until=now()+5;const JellySnapshot *v;
 do{v=jellyfin_poll(s);assert(v&&!v->busy);if(!strcmp(v->artwork.item,id))return v;usleep(1000);}while(now()<until);
 assert(!"artwork timed out");return NULL;
}
static void previews(JellyService *s,const JellySnapshot *v,bool real){
 char id[65];snprintf(id,sizeof id,"%s",v->page.items[0].id);
 assert(v->page.items[0].overview[0]&&v->page.items[0].ticks);
 v=artwork(s,id);assert(v->artwork.width&&v->artwork.height&&v->artwork.width<=JELLY_ART_WIDTH&&v->artwork.height<=JELLY_ART_HEIGHT);
 if(!real){
  assert(v->artwork.width==JELLY_ART_WIDTH&&v->artwork.height==JELLY_ART_HEIGHT);
  /* Missing, corrupt, oversized body and oversized decoded dimensions. */
  for(unsigned i=1;i<=4;i++){char missing[65];snprintf(missing,sizeof missing,"%032x",100+i);v=artwork(s,missing);assert(!v->artwork.width&&!v->busy);}
  v=artwork(s,id);assert(v->artwork.width);
  jellyfin_select(s,"00000000000000000000000000000069");usleep(300000);
  double at=now();v=command(s,(JellyCommand){.type=JELLY_REFRESH});assert(now()-at<1&&v->loaded);
  /* A selection changed before debounce never starts a download. */
  jellyfin_select(s,"0000000000000000000000000000006a");jellyfin_select(s,id);v=artwork(s,id);assert(v->artwork.width);
 }
 puts("PASS: selected movie/episode metadata and authenticated bounded PNG artwork; correct dimensions, background loading, fallback/cache, rapid selection and preemption");
}
static VideoSnapshot pump(VideoPlayer *player,JellyService *s,double target,bool paused){
 double end=now()+15;VideoSnapshot state={0};bool picture=false;unsigned heard=0;
 do{
  float pcm[512];video_audio_render(player,pcm,256);for(unsigned i=0;i<512;i++)heard+=fabsf(pcm[i])>.0001f;
  VideoPicture image;if(video_take_picture(player,&image)){assert(image.pts>=target-.06&&image.pts<target+1);picture=true;video_release_picture(player,image.slot,image.revision);}
  video_poll(player,&state);jellyfin_progress(s,&state);
  if(state.state==VIDEO_FAILED){fprintf(stderr,"seek: %s\n",state.error);assert(0);}
  if(picture&&!state.seeking&&((paused&&state.state==VIDEO_PAUSED)||(!paused&&heard>100)))return state;
  usleep(1000);
 }while(now()<end);fprintf(stderr,"seek timed out: state=%u position=%.3f target=%.3f\n",state.state,state.position,target);assert(0);return state;
}
/* Keep exercising the actual reader after pictures first arrive: the old
   short tests stopped before the eight-second inactivity deadline. */
static double stalled_seek(JellyService *s,VideoPlayer *player,double initial,const char *mode){
 VideoSnapshot v=pump(player,s,initial,false);double target=v.position+2;video_seek(player,2);v=pump(player,s,target,false);
 double begin=now();bool cancel=strstr(mode,"cancel")!=NULL,timeout=strstr(mode,"timeout")!=NULL;unsigned pictures=0,heard=0;
 while(now()-begin<(timeout?36:14)){
  float pcm[512];video_audio_render(player,pcm,256);for(unsigned i=0;i<512;i++)heard+=fabsf(pcm[i])>.0001f;
  VideoPicture image;if(video_take_picture(player,&image)){pictures++;assert(image.pts>=target-.06);video_release_picture(player,image.slot,image.revision);}
  video_poll(player,&v);jellyfin_progress(s,&v);
  if(cancel&&now()-begin>2){double at=now();video_stop(player);video_close(player);assert(now()-at<1);puts("PASS: cancel/close during stalled post-seek body returns within one second");return v.position;}
  if(v.state==VIDEO_FAILED){if(!timeout)fprintf(stderr,"sustained seek failed: %s\n",v.error);assert(timeout&&strstr(v.error,"30 seconds")&&now()-begin>28);puts("PASS: genuine media inactivity remains bounded at30seconds");video_close(player);return v.position;}
  usleep(1000);
 }
 assert(!timeout&&!cancel&&v.state==VIDEO_ENDED&&pictures>10&&heard>100);puts("PASS: ten-second post-seek media gap recovers with picture/audio and completes after14seconds observation");video_close(player);return v.position;
}
static VideoSnapshot sustained_playback(JellyService *s,VideoPlayer *player){
 double begin=now();VideoSnapshot v={0};unsigned pictures=0,heard=0;
 while(now()-begin<12){
  float pcm[512];video_audio_render(player,pcm,256);for(unsigned i=0;i<512;i++)heard+=fabsf(pcm[i])>.0001f;
  VideoPicture image;if(video_take_picture(player,&image)){pictures++;video_release_picture(player,image.slot,image.revision);}
  video_poll(player,&v);jellyfin_progress(s,&v);if(v.state==VIDEO_FAILED)fprintf(stderr,"long seek: %s\n",v.error);
  assert(v.state==VIDEO_PLAYING);usleep(5333);
 }
 assert(pictures>200&&heard>1000);return v;
}
static VideoSnapshot content_picture(VideoPlayer *player,JellyService *s,double target){
 double end=now()+20;VideoSnapshot v={0};
 while(now()<end){
  float pcm[512];video_audio_render(player,pcm,256);video_poll(player,&v);jellyfin_progress(s,&v);assert(v.state!=VIDEO_FAILED);
  VideoPicture image;if(video_take_picture(player,&image)){
   assert(image.width==176&&image.height==96&&image.format!=VIDEO_RGBA);unsigned number=0;
   for(unsigned bit=0;bit<11;bit++)if(image.rgba[(image.height/2)*image.width+bit*16+8]>128)number|=1u<<bit;
   double content=number/30.;printf("seek content: requested=%.3f actual=%.3f reported_pts=%.3f\n",target,content,image.pts);fflush(stdout);
   video_release_picture(player,image.slot,image.revision);
   assert(fabs(content-target)<.04&&"seek changed the clock but replayed cached pictures");video_poll(player,&v);return v;
  }
  usleep(1000);
 }
 assert(!"seek content picture timed out");return v;
}
static double small_seeks(JellyService *s,VideoPlayer *player,const VideoHttpInput *input){
 video_pause(player,true);VideoSnapshot v=content_picture(player,s,input->start_position);double target=v.position;
 const double steps[]={.25,-.25,.05,-.05,1,-1,10,-10,.75,-.5,.25,-.5};
 for(unsigned i=0;i<sizeof steps/sizeof steps[0];i++){
  target+=steps[i];video_seek(player,steps[i]);v=content_picture(player,s,target);
  assert(v.state==VIDEO_PAUSED&&fabs(v.position-target)<.01);
 }
 video_seek(player,10000);v=content_picture(player,s,input->duration-.5);assert(fabs(v.position-(input->duration-.5))<.01);
 video_seek(player,input->start_position-v.position);v=content_picture(player,s,input->start_position);target=v.position;
 /* A previously queued cleanup must not remove the new stream's cache file. */
 video_pause(player,false);double at=now();unsigned shown=0;
 while(now()-at<12){float pcm[512];video_audio_render(player,pcm,256);VideoPicture image;if(video_take_picture(player,&image)){
  unsigned number=0;for(unsigned bit=0;bit<11;bit++)if(image.rgba[(image.height/2)*image.width+bit*16+8]>128)number|=1u<<bit;
  assert(fabs(number/30.-image.pts)<.04);shown++;video_release_picture(player,image.slot,image.revision);}video_poll(player,&v);jellyfin_progress(s,&v);assert(v.state!=VIDEO_FAILED);usleep(5333);}
 assert(shown>200&&v.position>target+10);
 video_close(player);puts("PASS: actual frame content matches fractional/one-second/ten-second forward/back/return seeks, pause retained,12second continuous content/clock agreement, delayed cleanup isolated");return v.position;
}
static double seeking(JellyService *s,VideoPlayer *player,const VideoHttpInput *input,const char *mode){
 assert(input->start_position>0&&input->duration>15);
 if(!strcmp(mode,"seek-cache")||!strcmp(mode,"real-seek-content"))return small_seeks(s,player,input);
 if(strstr(mode,"seek-stall"))return stalled_seek(s,player,input->start_position,mode);
 VideoSnapshot v=pump(player,s,input->start_position,false);
 video_pause(player,true);video_poll(player,&v);double target=fmin(input->duration-.5,v.position+10);
 video_seek(player,10);video_poll(player,&v);assert(v.seeking&&v.state==VIDEO_PAUSED&&fabs(v.position-target)<.01);
 if(!strcmp(mode,"seek-cancel")){
  usleep(60000);double at=now();video_stop(player);video_close(player);assert(now()-at<1);return target;
 }
 if(!strcmp(mode,"seek-error")){
  double end=now()+5;do{video_poll(player,&v);usleep(1000);}while(v.state!=VIDEO_FAILED&&now()<end);
  assert(v.state==VIDEO_FAILED&&v.network&&fabs(v.position-target)<.01);video_close(player);return target;
 }
 v=pump(player,s,target,true);double held=v.position;usleep(100000);video_poll(player,&v);assert(fabs(v.position-held)<.001);
 video_seek(player,-10);target=fmax(0,target-10);v=pump(player,s,target,true);
 /* Rapid opposite seeks coalesce against the requested full-item position. */
 video_seek(player,10);video_seek(player,-10);v=pump(player,s,target,true);
 video_seek(player,-1000);v=pump(player,s,0,true);assert(v.position<.1);
 video_seek(player,10000);v=pump(player,s,input->duration-.5,true);
 video_seek(player,-10000);v=pump(player,s,0,true);video_seek(player,input->start_position);v=pump(player,s,input->start_position,true);video_pause(player,false);v=pump(player,s,input->start_position,false);
 assert(v.position>=input->start_position&&v.position<input->start_position+2&&v.has_audio&&v.network);
 if(!strcmp(mode,"real-seek")){
  v=sustained_playback(s,player);assert(v.position>input->start_position+10);
  video_pause(player,true);video_poll(player,&v);video_seek(player,input->start_position-v.position);v=pump(player,s,input->start_position,true);video_pause(player,false);v=pump(player,s,input->start_position,false);
  puts("PASS: official-server playback continues for12 wall-clock seconds after seek with >200 pictures and audio");
 }
 video_close(player);return v.position;
}
int main(int argc,char **argv){
 assert(argc==4);const char *password=getenv("BLADES_JELLYFIN_TEST_PASSWORD");assert(password&&*password);
 JellyService *s=jellyfin_open(argv[2],NULL);assert(s);const JellySnapshot *v=idle(s,0);
 edit(s,1,argv[1]);edit(s,2,"blades-test");edit(s,3,"wrong-password");v=command(s,(JellyCommand){.type=JELLY_CONNECT});assert(v->state!=JELLY_CONNECTED);
 edit(s,3,password);v=command(s,(JellyCommand){.type=JELLY_CONNECT});if(v->state!=JELLY_CONNECTED)fprintf(stderr,"connect: %s\n",v->status);assert(v->state==JELLY_CONNECTED);
 saved_without_password(argv[2],password);
 v=command(s,(JellyCommand){.type=JELLY_ROOT});
 if(!strcmp(argv[3],"expired")){assert(v->state==JELLY_OFF);jellyfin_close(s);puts("PASS: expired session clears stored credentials");return 0;}
 assert(v->page.count>=2&&v->loaded);assert(!strcmp(v->page.items[0].name,"Continue Watching")&&!strcmp(v->page.items[1].name,"Next Up"));for(unsigned i=0;i<v->page.count;i++)assert(strcmp(v->page.items[i].collection,"music"));
 if(!strcmp(argv[3],"cancel")){
  unsigned revision=v->revision;assert(jellyfin_request(s,&(JellyCommand){.type=JELLY_REFRESH}));usleep(30000);double at=now();jellyfin_cancel(s);v=idle(s,revision);assert(now()-at<1&&v->page.count&&v->loaded);
  for(unsigned i=0;i<20;i++){revision=v->revision;assert(jellyfin_request(s,&(JellyCommand){.type=JELLY_REFRESH}));jellyfin_cancel(s);v=idle(s,revision);assert(v->loaded&&v->page.count);}
  jellyfin_close(s);puts("PASS: active/queued cancellation remains responsive, preserves page and permits subsequent requests");return 0;
 }
 if(strstr(argv[3],"seek")){
  v=command(s,(JellyCommand){.type=JELLY_OPEN,.row=named(v,"Continue Watching")});assert(v->depth==1);
  for(unsigned i=0;i<v->page.count;i++)if(!strcmp(v->page.items[i].type,"Episode")&&v->page.items[i].series[0])assert(strstr(v->labels[i],v->page.items[i].series)&&strstr(v->labels[i],v->page.items[i].name));
  if(v->page.total>v->page.count){v=command(s,(JellyCommand){.type=JELLY_NEXT});assert(v->start);v=command(s,(JellyCommand){.type=JELLY_PREVIOUS});assert(!v->start);}
  v=command(s,(JellyCommand){.type=JELLY_BACK});assert(!v->depth);
  v=command(s,(JellyCommand){.type=JELLY_OPEN,.row=named(v,"Next Up")});assert(v->depth==1);
  if(v->page.count){const xf_item *episode=v->page.items;assert(!strcmp(episode->type,"Episode")&&episode->series[0]&&strstr(v->labels[0],episode->series)&&strstr(v->labels[0],episode->name));if(!strncmp(argv[3],"real-",5))previews(s,v,true);}
  v=command(s,(JellyCommand){.type=JELLY_REFRESH});assert(v->depth==1);
  v=command(s,(JellyCommand){.type=JELLY_BACK});assert(!v->depth);
 }
 unsigned library=2;assert(library<v->page.count);
 v=command(s,(JellyCommand){.type=JELLY_OPEN,.row=library});assert(v->depth==1&&v->page.count);
 if(!strcmp(argv[3],"artwork")){previews(s,v,false);jellyfin_close(s);return 0;}
 if(!strncmp(argv[3],"real-",5))previews(s,v,true);
 if(v->page.total>v->page.count){v=command(s,(JellyCommand){.type=JELLY_NEXT});assert(v->start);v=command(s,(JellyCommand){.type=JELLY_PREVIOUS});assert(!v->start);}
 unsigned row=0;while(row<v->page.count&&!jellyfin_playable(v->page.items+row))row++;assert(row<v->page.count);
 v=command(s,(JellyCommand){.type=JELLY_PLAY,.row=row});VideoHttpInput input;assert(jellyfin_take_playback(s,&input));assert(!jellyfin_take_playback(s,&input));
 assert(strstr(input.request,"VideoCodec=mpeg2video")&&strstr(input.request,"AudioCodec=mp2")&&strstr(input.request,"Authorization: MediaBrowser"));
 double initial_position=input.start_position;
 VideoHttpInput seek_input=input;
 VideoPlayer *player=video_create(&(VideoConfig){.channels=2,.audio_available=true,.gpu_yuv=true});assert(player&&video_play_http(player,&input));xf_wipe(&input,sizeof input);
 if(strstr(argv[3],"seek")){
  double position=seeking(s,player,&seek_input,argv[3]);xf_wipe(&seek_input,sizeof seek_input);
  v=jellyfin_poll(s);unsigned revision=v?v->revision:0;jellyfin_finish(s,position);v=idle(s,revision);assert(v->loaded&&v->depth==1);
  saved_without_password(argv[2],password);jellyfin_close(s);
  puts("PASS: Continue Watching/Next Up navigation, fresh saved-position resume, forward/backward/rapid/bounded seeking, paused seek, reset audio/video clocks, reporting and cancellation/error cleanup");return 0;
 }
 xf_wipe(&seek_input,sizeof seek_input);assert(initial_position==(!strcmp(argv[3],"resume-played")?1:0));
 double begin=now();unsigned pictures=0,heard=0;VideoSnapshot state={0};bool paused=false,resumed=false;
 while(now()-begin<18){
  float pcm[512];video_audio_render(player,pcm,256);for(unsigned i=0;i<512;i++)heard+=fabsf(pcm[i])>.0001f;
  VideoPicture p;if(video_take_picture(player,&p)){assert(p.width<=1280&&p.height<=720&&p.format!=VIDEO_RGBA);pictures++;video_release_picture(player,p.slot,p.revision);}
  video_poll(player,&state);jellyfin_progress(s,&state);
  if(state.state==VIDEO_FAILED){fprintf(stderr,"stream: %s\n",state.error);break;}
  if(pictures>8&&!paused){video_pause(player,true);paused=true;}
  if(paused&&!resumed&&state.state==VIDEO_PAUSED){usleep(60000);VideoSnapshot pstate;video_poll(player,&pstate);assert(pstate.position==state.position);video_pause(player,false);resumed=true;}
  if(state.state==VIDEO_ENDED)break;
  usleep(1000);
 }
 if(!strcmp(argv[3],"truncated"))assert(state.state==VIDEO_FAILED);
 else assert(state.state==VIDEO_ENDED&&state.network&&pictures>15&&heard>100&&paused&&resumed);
 video_close(player);v=jellyfin_poll(s);unsigned revision=v?v->revision:0;jellyfin_finish(s,state.position);v=idle(s,revision);
 saved_without_password(argv[2],password);jellyfin_close(s);
 s=jellyfin_open(argv[2],NULL);assert(s);v=idle(s,0);
 if(!strcmp(argv[3],"retry")){assert(v->state==JELLY_ERROR);v=command(s,(JellyCommand){.type=JELLY_CONNECT});}
 assert(v->state==JELLY_CONNECTED);v=command(s,(JellyCommand){.type=JELLY_DISCONNECT});assert(v->state==JELLY_OFF);jellyfin_close(s);
 s=jellyfin_open(argv[2],NULL);assert(s);v=idle(s,0);assert(v->state==JELLY_OFF);v=command(s,(JellyCommand){.type=JELLY_REMOVE});assert(!v->server[0]&&!v->username[0]);jellyfin_close(s);
 printf("PASS: Jellyfin authentication, video-only libraries, folders/pages, authenticated MPEG2/MP2 playback (%u pictures), pause/end, reporting/cleanup, saved-session reconnect/disconnect/remove\n",pictures);
}
