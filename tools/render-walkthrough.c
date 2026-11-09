/* Production renderer/navigation, explicitly simulated filesystem snapshots.
   This tool does not read console storage or launch Xbox software. */
#include "blades.h"
#include "resources.h"
#ifdef BLADES_GPU_CAPTURE
#include "gl_renderer.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {int second;unsigned action;} Step;
static void video_log(const char *line){fprintf(stderr,"%s\n",line);}
static const Step steps[]={
 {2,UP},{3,SELECT},{7,SELECT},{9,BACK},{10,PROFILE},{14,SELECT},{18,DOWN},{19,SELECT},
 {21,DOWN},{22,DOWN},{23,UP},{24,UP},{25,RIGHT},{26,RIGHT},{27,SELECT},{29,BACK},{30,LEFT},{31,LEFT},{32,BACK},
 {34,RIGHT},{35,RIGHT},{36,DOWN},{37,DOWN},{38,DOWN},{39,DOWN},{40,SELECT},
 {42,DOWN},{43,SELECT},{45,BACK},{47,DOWN},{48,SELECT},{50,BACK},{51,UP},{52,UP},{53,SELECT},{55,SELECT},{57,BACK},
 {59,LEFT},{60,LEFT},{61,DOWN},{62,SELECT},{63,DOWN},{64,DOWN},{65,DOWN},{66,SELECT},{67,SELECT},{68,PROFILE},
 {72,LEFT},{74,DOWN},{75,DOWN},{76,DOWN},{77,UP},{79,LEFT},
 {84,RIGHT},{85,RIGHT},{86,RIGHT},{87,DOWN},{88,DOWN},{89,SELECT},
 {90,DOWN},{91,SELECT},{93,SELECT},{94,SELECT},{95,SELECT},{97,BACK},{98,DOWN},{100,SELECT},{101,SELECT},{102,SELECT},{104,BACK},
 {105,BACK},{106,UP},{107,SELECT},{110,SELECT},{113,SELECT},{114,SELECT},{115,RIGHT},{117,SELECT},{119,LEFT},{137,BACK},
 {140,BACK},{141,DOWN},{142,SELECT},{145,SELECT},{147,SELECT},{148,SELECT},{151,BACK},{154,BACK},{156,BACK},{158,BACK},
 {159,DOWN},{160,DOWN},{161,SELECT},{163,DOWN},{164,DOWN},{165,DOWN},{167,BACK},{169,RIGHT},
 {171,LEFT},{172,UP},{173,SELECT},{175,DOWN},{176,SELECT},{177,SELECT},{178,SELECT},{179,PROFILE},
 {180,UP},{181,UP},{182,UP},{183,UP},{184,SELECT},{186,DOWN},{188,BACK},
 {189,UP},{190,UP},{191,UP},{192,SELECT},{194,BACK},{195,DOWN},{196,SELECT},{198,BACK},
 {199,DOWN},{200,SELECT},{201,DOWN},{202,SELECT},{204,BACK},{205,BACK},{207,RIGHT},
 {209,DOWN},{210,DOWN},{211,SELECT},{214,DOWN},{215,SELECT},{217,PROFILE},{219,SELECT},{229,BACK},{231,LEFT},{235,RIGHT}
};
static void files(LibrarySnapshot *s,LibraryFolder *entries,const char *path){
 memset(entries,0,6*sizeof *entries);snprintf(s->browse_path,LIBRARY_PATH,"%s",path);
 if(!path[0]){entries[0]=(LibraryFolder){.name="USB0",.path="usb0:/"};s->folder_count=1;}
 else if(!strcmp(path,"usb0:/Assets")){
  entries[0]=(LibraryFolder){.name="background.png",.path="usb0:/Assets/background.png",.is_file=true};s->folder_count=1;
 }else{
  const char *names[]={"Assets","App.XEX","cover.png","file.dll.xex","notes.txt","README"};
  s->folder_count=6;
  for(unsigned i=0;i<6;i++){snprintf(entries[i].name,sizeof entries[i].name,"%s",names[i]);snprintf(entries[i].path,LIBRARY_PATH,"usb0:/%s",names[i]);entries[i].is_file=i!=0;entries[i].executable=i==1;}
 }
 s->revision++;
}
static void signed_in(ProfileSnapshot *p){
 p->identity=(ProfileIdentity){.state=PROFILE_LOCAL,.xuid=42,.name="Preview Player"};p->known=7;p->games=3;p->score=50;p->achievements=5;p->has_picture=true;p->revision++;p->picture_revision++;
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){unsigned at=(y*64+x)*4;p->picture[at]=x*3;p->picture[at+1]=80+y*2;p->picture[at+2]=190;p->picture[at+3]=255;}
}
static void game(AchievementSnapshot *s,unsigned at){
 memset(s->tiles,0,sizeof s->tiles);memset(s->items,0,sizeof s->items);s->item_count=12;s->title_id=123+at;
 for(unsigned i=0;i<12;i++){AchievementItem *a=s->items+i;a->id=100+i;a->earned=i<s->titles[at].earned;a->score=10;a->unlocked=a->earned?132223104000000000ULL+(12-i)*864000000000ULL:0;snprintf(a->name,sizeof a->name,"Example Achievement %u",i+1);}
 achievement_prepare(s);s->busy=false;s->revision++;
}
/* Deterministic capture waits for the real worker, rather than advancing the
   scripted controller while a network page/stop is still pending. */
static void settle_jelly(Dashboard *d,JellyService *s,unsigned before){
 if(!s)return;
 for(unsigned i=0;i<10000;i++){
  const JellySnapshot *v=jellyfin_poll(s);dashboard_set_jellyfin(d,v);
  if(v&&v->revision>before&&!v->busy)return;
  struct timespec delay={0,1000000};nanosleep(&delay,NULL);
 }
 fprintf(stderr,"Jellyfin capture worker did not settle\n");
}
int main(int argc,char **argv){
 const char *pack=getenv("BLADES_RESOURCE_PACK"),*media=getenv("BLADES_TEST_MEDIA");if(argc!=2||!pack||!media||!resources_open(pack))return 1;
 VideoPlayer *player=video_create(&(VideoConfig){.channels=2,.audio_available=true,.log=video_log,.gpu_yuv=true});if(!player)return 1;
 FILE *audio=fopen(argv[1],"wb");if(!audio)return 1;
 Canvas c={.pixels=malloc(WIDTH*HEIGHT*4)};AchievementSnapshot *ach=calloc(1,sizeof *ach);if(!c.pixels||!ach)return 1;
#ifdef BLADES_GPU_CAPTURE
 GlRenderer *gpu=gl_renderer_open();if(!gpu)return 1;
#endif
 LibraryFolder entries[6]={0};LibrarySnapshot library={.folders=entries,.revision=1};
 LibraryEntry videos[5]={
  {.name="1080p30 MPEG-2 test",.category=LIB_VIDEO,.kind=LIB_MEDIA},
  {.name="720p H.264 test",.category=LIB_VIDEO,.kind=LIB_MEDIA},
  {.name="Another clip",.category=LIB_VIDEO,.kind=LIB_MEDIA}};
 snprintf(videos[0].path,LIBRARY_PATH,"%s/sample1080-mpeg2.mkv",media);
 snprintf(videos[1].path,LIBRARY_PATH,"%s/sample720-h264.mp4",media);
 snprintf(videos[2].path,LIBRARY_PATH,"%s/sample720-h264.mp4",media);
 ProfileSnapshot profile={.identity={.state=PROFILE_SIGNED_OUT},.profiles_known=true,.available_profiles=2,.revision=1};
 const char *jelly_config=getenv("BLADES_JELLYFIN_CONFIG");JellyService *jelly=jelly_config?jellyfin_open(jelly_config,NULL):NULL;
 JellySnapshot disconnected={.state=JELLY_OFF,.revision=1,.status="Connect to Jellyfin under Media Center."};unsigned actual_jelly=0;
 Dashboard d;dashboard_begin_startup(&d);d.disc.state=DISC_EMPTY;dashboard_set_profile(&d,&profile);dashboard_set_library(&d,&library);dashboard_set_achievements(&d,ach);ui_audio_init();
 unsigned next=0;bool ok=true;int due=-1;AchievementRequest pending={0};
 int update_started=-1;bool update_loading=false;
 for(int frame=0;frame<240*30;frame++){
  if(frame==88*30){library.entries=videos;library.count=3;library.root_count=1;library.roots[0]=(LibraryRoot){.path="usb0:/Movies",.category=LIB_MEDIA_FOLDER,.max_depth=LIBRARY_DEPTH_ALL};library.revision++;dashboard_set_library(&d,&library);}
  if(frame==5*30||frame==16*30){signed_in(&profile);d.dialog=false;fprintf(stderr,"%02d:%02d SIMULATED Guide sign-in completed\n",frame/1800,frame/30%60);}
  if(frame==12*30){profile.identity=(ProfileIdentity){.state=PROFILE_SIGNED_OUT};profile.known=0;profile.has_picture=false;profile.revision++;d.dialog=false;fprintf(stderr,"00:12 SIMULATED Guide sign-out completed\n");}
  if(frame==4*30){d.ftp=(FtpSnapshot){.state=FTP_RUNNING,.enabled=true,.port=21,.revision=1,.username=FTP_USERNAME,.password=FTP_PASSWORD};strcpy(d.ftp.ip,"192.168.1.50");}
  if(frame==41*30||frame==56*30){d.ftp.devices=1;d.ftp.revision++;}
  if(frame==45*30){strcpy(d.ftp.username,"transfer_user");d.ftp.revision++;}
  if(frame==50*30){strcpy(d.ftp.password,"transfer_pass");d.ftp.revision++;}
  if(frame==due){
   ach->user=0;ach->xuid=42;
   if(pending.type==ACH_LOAD){ach->title_count=3;for(unsigned i=0;i<3;i++){ach->titles[i]=(AchievementTitle){.id=123+i,.earned=5-i,.possible=12,.score_earned=(5-i)*10,.score_possible=120,.played=132223104000000000ULL};snprintf(ach->titles[i].name,sizeof ach->titles[i].name,"Example Game %u",i+1);}game(ach,0);}
   else if(pending.type==ACH_GAME)game(ach,pending.title_id-123);
   else ach->revision++;
  }
  dashboard_set_profile(&d,&profile);dashboard_set_achievements(&d,ach);
  unsigned action=0;if(next<sizeof steps/sizeof steps[0]&&frame==steps[next].second*30)action=steps[next++].action;
  JellyPage old_jelly=d.jelly_page;bool old_stream=d.video_view&&d.video.network;unsigned old_jelly_revision=d.jelly?d.jelly->revision:0;
  dashboard_set_jellyfin(&d,jelly?jellyfin_poll(jelly):&disconnected);
  if(!dashboard_video_update(&d,player,action,frame?1.f/30:0,false))dashboard_update(&d,action,frame?1.f/30:0);
  if(d.video_view&&d.video.network)jellyfin_progress(jelly,&d.video);else if(old_stream)jellyfin_finish(jelly,d.video.position);
  if(old_jelly!=JELLY_UI_NONE&&d.jelly_page==JELLY_UI_NONE)jellyfin_cancel(jelly);
  dashboard_jellyfin_dispatch(&d,jelly);
  /* Updater snapshots are simulated; the real TLS worker is tested separately. */
  if(dashboard_update_page(&d)){
   if(d.update_check){update_started=frame;update_loading=false;d.update=(UpdateSnapshot){.state=UPDATE_CHECKING,.revision=1};snprintf(d.update.message,sizeof d.update.message,"Checking GitHub for updates... (simulation)");fprintf(stderr,"SIMULATED update check; real TLS worker tested separately\n");}
   if(d.update.state==UPDATE_CHECKING&&frame-update_started>=30){d.update.state=UPDATE_AVAILABLE;strcpy(d.update.version,"0.53.0");d.update.total=6*1024*1024;snprintf(d.update.message,sizeof d.update.message,"A newer version is available. (simulation)");d.update.revision++;}
   if(d.update_download&&(d.update.state==UPDATE_AVAILABLE||d.update.state==UPDATE_CANCELLED)){update_started=frame;update_loading=true;d.update.state=UPDATE_DOWNLOADING;snprintf(d.update.message,sizeof d.update.message,"Downloading the USB ZIP... (simulation)");fprintf(stderr,"SIMULATED update download started\n");}
   if(d.update_cancel&&update_loading){update_loading=false;d.update.state=UPDATE_CANCELLED;snprintf(d.update.message,sizeof d.update.message,"Update download cancelled. (simulation)");d.update.revision++;}
   if(update_loading){int elapsed=frame-update_started;d.update.revision++;
    if(elapsed<120){d.update.total=6*1024*1024;d.update.received=d.update.total*(unsigned)elapsed/120;}
    else if(elapsed<180){d.update.state=UPDATE_PREPARING;d.update.total=32;d.update.received=(unsigned)(elapsed-120)*32/60;snprintf(d.update.message,sizeof d.update.message,"Preparing the update... (simulation)");}
    else if(elapsed<210){d.update.state=UPDATE_READY;snprintf(d.update.message,sizeof d.update.message,"Installing and restarting Blades... (simulation)");}
    else{update_loading=false;fprintf(stderr,"SIMULATED installer/restart; real file commit and recovery tested separately\n");dashboard_begin_startup(&d);}}
  }
  if(jelly&&((old_stream&&!d.video_view)||d.jelly_command.type))settle_jelly(&d,jelly,old_jelly_revision);
  if(d.video_view&&d.video.network&&(action&(LEFT|RIGHT))){
   fprintf(stderr,"%02d:%02d ACTUAL Jellyfin seek requested; target=%.3f paused=%u\n",frame/1800,frame/30%60,d.video.position,d.video.state==VIDEO_PAUSED);
   for(unsigned i=0;i<10000&&d.video_view&&(d.video.seeking||!d.video.presented);i++){
    float prime[512];video_audio_render(player,prime,256);struct timespec delay={0,1000000};nanosleep(&delay,NULL);dashboard_video_update(&d,player,0,0,false);
   }
   if(d.video.seeking||!d.video.presented){fprintf(stderr,"Jellyfin capture seek failed\n");ok=false;break;}
   fprintf(stderr,"ACTUAL Jellyfin seek displayed; full-item position=%.3f\n",d.video.position);
  }
  if(d.jelly_page==JELLY_UI_BROWSE&&!d.video_view){VideoHttpInput input;
   if(jellyfin_take_playback(jelly,&input)){
    if(!dashboard_video_http_open(&d,player,&input)){ok=false;break;}xf_wipe(&input,sizeof input);
    for(unsigned n=0;n<5000&&!d.video_has_picture&&d.video_view;n++){
     float prime[512];video_audio_render(player,prime,256);struct timespec delay={0,1000000};nanosleep(&delay,NULL);dashboard_video_update(&d,player,0,0,false);
    }
    if(!d.video_has_picture){fprintf(stderr,"Jellyfin sample failed: %s\n",d.video.error);ok=false;break;}
    actual_jelly++;fprintf(stderr,"%02d:%02d ACTUAL Jellyfin server stream through shipping player; resume=%.3f\n",frame/1800,frame/30%60,d.video.position);
   }
  }
  if(action&&d.jelly_page==JELLY_UI_BROWSE)fprintf(stderr,"Jellyfin capture page=%s row=%d depth=%u busy=%u\n",d.jelly?d.jelly->title:"",d.jelly_row,d.jelly?d.jelly->depth:0,d.jelly?d.jelly->busy:0);
  if(d.profile_command.type){d.dialog=true;d.message=d.profile_command.type==PROFILE_SIGN_IN?"Desktop preview: native Guide sign-in requested.":d.profile_command.type==PROFILE_SIGN_OUT?"Desktop preview: native Guide sign-out menu requested.":"Desktop preview: native Guide profile menu requested.";fprintf(stderr,"%02d:%02d SIMULATED profile Guide command=%u\n",frame/1800,frame/30%60,d.profile_command.type);}
  if(d.achievement_command.type==ACH_GUIDE_GAME||d.achievement_command.type==ACH_GUIDE_DETAIL){d.dialog=true;d.message="Desktop preview: native Guide achievement requested.";fprintf(stderr,"%02d:%02d SIMULATED Guide achievement title=%u id=%u\n",frame/1800,frame/30%60,d.achievement_command.title_id,d.achievement_command.achievement_id);}
  else if(d.achievement_command.type){pending=d.achievement_command;due=frame+15;if(pending.type!=ACH_IMAGES)ach->busy=true;ach->revision++;}
  if(d.ftp_toggle){d.ftp.enabled=!d.ftp.enabled;d.ftp.state=d.ftp.enabled?FTP_RUNNING:FTP_OFF;d.ftp.devices=0;d.ftp.revision++;fprintf(stderr,"%02d:%02d SIMULATED FTP toggle enabled=%u\n",frame/1800,frame/30%60,d.ftp.enabled);}
  if(d.ftp_edit){d.dialog=true;d.message="Desktop preview: Xbox keyboard requested for FTP login details.";fprintf(stderr,"%02d:%02d SIMULATED FTP keyboard field=%u\n",frame/1800,frame/30%60,d.ftp_edit);}
  if(frame>=173*30&&d.library_command.type==LIB_CMD_BROWSE){
   memset(entries,0,sizeof entries);snprintf(library.browse_path,LIBRARY_PATH,"%s",d.library_command.path);
   library.folder_count=1;
   if(!library.browse_path[0])entries[0]=(LibraryFolder){.name="USB0",.path="usb0:/"};
   else if(!strcmp(library.browse_path,"usb0:/"))entries[0]=(LibraryFolder){.name="Mixed Media",.path="usb0:/Mixed Media"};
   else library.folder_count=0;
   library.revision++;dashboard_set_library(&d,&library);
  }
  if(frame>=173*30&&d.library_command.type==LIB_CMD_ADD){
   if(d.library_command.category!=LIB_MEDIA_FOLDER||d.library_command.max_depth!=LIBRARY_DEPTH_ALL){ok=false;break;}
   library.roots[1]=(LibraryRoot){.category=LIB_MEDIA_FOLDER,.max_depth=d.library_command.max_depth};snprintf(library.roots[1].path,LIBRARY_PATH,"%s",d.library_command.path);library.root_count=2;
   if(!library_read_entry(NULL,"usb0:/Mixed Media/Sample.mp3",LIB_MEDIA_FOLDER,NULL,videos+3)||!library_read_entry(NULL,"usb0:/Mixed Media/Image.png",LIB_MEDIA_FOLDER,NULL,videos+4)){ok=false;break;}
   library.count=5;library.revision++;dashboard_set_library(&d,&library);
   fprintf(stderr,"SIMULATED shared storage path added; production extension classification: Sample.mp3 ->%s; Image.png ->%s; no category screen\n",library_category_name(videos[3].category),library_category_name(videos[4].category));
  }
  if(d.library_command.type==LIB_CMD_FILES){files(&library,entries,d.library_command.path);dashboard_set_library(&d,&library);fprintf(stderr,"%02d:%02d SIMULATED file refresh path=%s\n",frame/1800,frame/30%60,library.browse_path);}
  if(d.library_command.type==LIB_CMD_LAUNCH&&d.library_command.entry.category==LIB_VIDEO){
   char path[LIBRARY_PATH];snprintf(path,sizeof path,"%s",d.library_command.entry.path);
   d.library_wait=false;if(!dashboard_video_open(&d,player,d.library_command.entry.path)){ok=false;break;}
   for(unsigned n=0;n<3000&&!d.video_has_picture&&d.video_view;n++){
    float prime[512];video_audio_render(player,prime,256);
    struct timespec delay={0,1000000};nanosleep(&delay,NULL);dashboard_video_update(&d,player,0,0,false);
   }
   if(!d.video_has_picture){fprintf(stderr,"embedded sample did not produce a frame: %u %s\n",d.video.state,d.video.error);ok=false;break;}
   fprintf(stderr,"%02d:%02d ACTUAL embedded sample decode %s\n",frame/1800,frame/30%60,path);
  }
  ui_audio_play(d.sound);if(action)fprintf(stderr,"%02d:%02d blade=%s depth=%d card=%u achievements=%u ftp=%u\n",frame/1800,frame/30%60,blade_names[d.blade],d.depth,d.profile_card_selected,d.achievement_page,dashboard_ftp_page(&d));
#ifdef BLADES_GPU_CAPTURE
  if(!gl_renderer_frame(gpu,&d,&c)){ok=false;break;}
#else
  dashboard_render(&c,&d);
#endif
  float samples[3200];if(!video_audio_render(player,samples,1600))ui_audio_render(samples,1600);if(fwrite(c.pixels,4,WIDTH*HEIGHT,stdout)!=WIDTH*HEIGHT||fwrite(samples,sizeof(float),3200,audio)!=3200){ok=false;break;}
 }
#ifdef BLADES_GPU_CAPTURE
 gl_renderer_close(gpu);
#endif
 if(library.root_count!=2||library.count!=5){fprintf(stderr,"Shared media navigation was not reached\n");ok=false;}
 if(jelly&&actual_jelly<2){fprintf(stderr,"Expected Jellyfin playback was not reached\n");ok=false;}
 jellyfin_close(jelly);
 if(d.video_has_picture)video_release_picture(player,d.video_picture.slot,d.video_picture.revision);video_close(player);
 free(ach);free(c.pixels);ui_audio_close();resources_close();if(fclose(audio)||fflush(stdout))ok=false;return ok?0:1;
}
