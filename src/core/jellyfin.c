/* Blades worker and persistence around XenonFin's native Jellyfin protocol.
 * GPLv3 combined distribution; XenonFin source is GPL-2.0-or-later. */
#include "jellyfin.h"
#include "http_client.h"
#include "resources.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
typedef struct {char server[JELLY_TEXT],username[JELLY_TEXT],token[2049],user[65],device[65];} Settings;
typedef struct {char item[65],source[65],session[65];bool active,started;double position;bool paused;} Playing;
struct JellyService {
    pthread_t worker;pthread_mutex_t lock;pthread_cond_t wake;unsigned stop,epoch,request_epoch;
    JellyCommand command;bool pending,busy,play_ready,report_due;
    JellyCommandType current_type;
    JellyHooks hooks;char config[1024],password[JELLY_TEXT];Settings settings;
    JellySnapshot view,*pending_view,*front;xf_browser browser;
    VideoHttpInput playback;Playing playing;VideoSnapshot progress;
    double next_report;
    char selected_art[65];bool preview_pending;double preview_at;
    JellyArtwork cache[3];unsigned cache_next,art_revision;
};
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static bool stop_request(void *context){JellyService *s=context;return __atomic_load_n(&s->stop,__ATOMIC_ACQUIRE)||__atomic_load_n(&s->epoch,__ATOMIC_ACQUIRE)!=s->request_epoch;}
static bool valid_id(const char *s){size_t n=strlen(s);if(n!=32&&n!=36)return false;for(size_t i=0;i<n;i++)if((n==36&&(i==8||i==13||i==18||i==23))?s[i]!='-':!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')||(s[i]>='A'&&s[i]<='F')))return false;return true;}
void jellyfin_runtime(uint64_t ticks,char *out,size_t bytes){
    uint64_t minutes=(ticks/10000000+59)/60;
    if(!ticks)snprintf(out,bytes,"Length unavailable");
    else if(minutes>=60)snprintf(out,bytes,"%llu hr %llu min",(unsigned long long)(minutes/60),(unsigned long long)(minutes%60));
    else snprintf(out,bytes,"%llu min",(unsigned long long)minutes);
}
static const char *label_separator(const char *s){while(*s==' '||*s=='-'||*s==':')s++;return s;}
void jellyfin_item_label(const xf_item *item,enum xf_view view,char *out,size_t bytes){
    if(!bytes)return;
    if(!item){out[0]=0;return;}
    bool qualify=(view==XF_RESUME||view==XF_NEXTUP)&&!item->folder&&!strcmp(item->type,"Episode")&&item->series[0];
    if(!qualify){snprintf(out,bytes,"%s",item->name);return;}
    size_t n=strlen(item->series);
    bool prefixed=n&&strlen(item->name)>=n&&!strncasecmp(item->name,item->series,n)&&
        (!item->name[n]||item->name[n]==' '||item->name[n]=='-'||item->name[n]==':');
    bool numbered=(item->has_index||item->index)&&(item->has_season||item->season);
    if(!numbered){if(prefixed)snprintf(out,bytes,"%s",item->name);else snprintf(out,bytes,"%s - %s",item->series,item->name);return;}
    const char *title=prefixed?label_separator(item->name+n):item->name;
    /* Normalize titles already prefixed with S01E02, without repeating tags. */
    if(*title=='S'||*title=='s'){
        char *end;errno=0;unsigned long season=strtoul(title+1,&end,10);
        if(!errno&&end>title+1&&(*end=='E'||*end=='e')&&season==item->season){
            const char *digits=end+1;unsigned long episode=strtoul(digits,&end,10);
            if(!errno&&end>digits&&episode==item->index&&(!*end||*end==' '||*end=='-'||*end==':'))title=label_separator(end);
        }
    }
    snprintf(out,bytes,"%s S%uE%u%s%s",item->series,item->season,item->index,*title?" - ":"",title);
}
static void publish(JellyService *s){
    JellySnapshot *v=malloc(sizeof *v);if(!v)return;
    snprintf(s->view.server,sizeof s->view.server,"%s",s->settings.server);
    snprintf(s->view.username,sizeof s->view.username,"%s",s->settings.username);
    snprintf(s->view.title,sizeof s->view.title,"%s",s->browser.current.title);
    s->view.page=s->browser.page;
    memset(s->view.labels,0,sizeof s->view.labels);for(unsigned i=0;i<s->view.page.count;i++)jellyfin_item_label(s->view.page.items+i,s->browser.current.view,s->view.labels[i],sizeof s->view.labels[i]);
    s->view.depth=s->browser.depth;s->view.start=s->browser.current.start;
    s->view.password_set=s->password[0]!=0;s->view.loaded=s->browser.loaded;
    pthread_mutex_lock(&s->lock);s->view.busy=s->busy;s->view.revision++;*v=s->view;free(s->pending_view);s->pending_view=v;pthread_mutex_unlock(&s->lock);
}
static void status(JellyService *s,const char *message){snprintf(s->view.status,sizeof s->view.status,"%s",message);}
static void clear_login(JellyService *s){xf_wipe(s->settings.token,sizeof s->settings.token);xf_wipe(s->settings.user,sizeof s->settings.user);xf_browser_init(&s->browser);s->view.state=JELLY_OFF;
    pthread_mutex_lock(&s->lock);s->selected_art[0]=0;s->preview_pending=false;pthread_mutex_unlock(&s->lock);
    memset(s->cache,0,sizeof s->cache);memset(&s->view.artwork,0,sizeof s->view.artwork);}
static void save(JellyService *s){
    if(!s->config[0])return;
    char user[JELLY_TEXT*6+3],server[JELLY_TEXT*6+3],temporary[1030];
    if(!xf_json_quote(s->settings.username,user,sizeof user)||!xf_json_quote(s->settings.server,server,sizeof server)){s->view.save_error=true;return;}
    char parent[1024];snprintf(parent,sizeof parent,"%s",s->config);char *slash=strrchr(parent,'/');if(slash){*slash=0;(void)mkdir(parent,0700);}
    snprintf(temporary,sizeof temporary,"%s.tmp",s->config);int fd=open(temporary,O_CREAT|O_WRONLY|O_TRUNC,0600);FILE *f=fd<0?NULL:fdopen(fd,"wb");bool ok=f!=NULL;
    if(!f&&fd>=0)close(fd);
    if(f){ok=fprintf(f,"{\"Server\":%s,\"Username\":%s,\"UserId\":\"%s\",\"AccessToken\":\"%s\",\"DeviceId\":\"%s\"}\n",server,user,s->settings.user,s->settings.token,s->settings.device)>0;
        if(fclose(f))ok=false;
        if(ok&&rename(temporary,s->config))ok=false;
        if(!ok)remove(temporary);}
    s->view.save_error=!ok;
}
static bool token_ok(const char *t){if(!*t)return false;for(;*t;t++)if((unsigned char)*t<33||(unsigned char)*t>126||*t=='"'||*t=='\\')return false;return true;}
static void load(JellyService *s){
    uint64_t seed=(uint64_t)(now()*1000000);for(unsigned i=0;i<32;i++){seed=seed*6364136223846793005ULL+1442695040888963407ULL;s->settings.device[i]="0123456789abcdef"[(seed>>32)&15];}s->settings.device[32]=0;
    FILE *f=s->config[0]?fopen(s->config,"rb"):NULL;if(!f)return;
    char data[8192];size_t n=fread(data,1,sizeof data,f);bool ok=n<sizeof data&&!ferror(f);fclose(f);if(!ok)return;
    Settings found={0};char error[192];xf_endpoint endpoint;
    if(xf_json_string(data,n,"Server",found.server,sizeof found.server)!=1||
       (found.server[0]&&!xf_endpoint_parse(found.server,&endpoint,error,sizeof error))||
       xf_json_string(data,n,"Username",found.username,sizeof found.username)!=1||
       xf_json_string(data,n,"UserId",found.user,sizeof found.user)!=1||
       xf_json_string(data,n,"AccessToken",found.token,sizeof found.token)!=1||
       xf_json_string(data,n,"DeviceId",found.device,sizeof found.device)!=1||!valid_id(found.device)||
       (found.token[0]&&(!token_ok(found.token)||!valid_id(found.user)))){xf_wipe(&found,sizeof found);xf_wipe(data,sizeof data);return;}
    s->settings=found;xf_wipe(&found,sizeof found);xf_wipe(data,sizeof data);
}
static bool endpoint(JellyService *s,xf_endpoint *out){char error[192];if(xf_endpoint_parse(s->settings.server,out,error,sizeof error))return true;status(s,error);return false;}
static void preview(JellyService *s,const char *id){
    if(s->view.state!=JELLY_CONNECTED||!valid_id(id))return;
    JellyArtwork *art=NULL;
    for(unsigned i=0;i<3;i++)if(!strcmp(s->cache[i].item,id)){art=s->cache+i;break;}
    if(!art){
        art=s->cache+(s->cache_next++%3);memset(art,0,sizeof *art);snprintf(art->item,sizeof art->item,"%s",id);
        char request[XF_REQUEST_LIMIT],path[256],error[192];xf_endpoint ep;size_t size=0;
        snprintf(path,sizeof path,"/Items/%s/Images/Primary?MaxWidth=%u&MaxHeight=%u&Format=Png&EnableImageEnhancers=false",id,JELLY_ART_WIDTH,JELLY_ART_HEIGHT);
        unsigned char *data=NULL;
        if(xf_endpoint_parse(s->settings.server,&ep,error,sizeof error)&&xf_api_request(request,sizeof request,&ep,s->settings.device,s->settings.token,"GET",path,NULL,3)>0)
            data=http_image_request(&ep,request,&size,stop_request,s);
        xf_wipe(request,sizeof request);
        if(data){int w=0,h=0;unsigned char *pixels=resource_png_decode_bounded(data,(int)size,&w,&h,JELLY_ART_WIDTH,JELLY_ART_HEIGHT);
            if(pixels){art->width=(unsigned)w;art->height=(unsigned)h;memcpy(art->pixels,pixels,(size_t)w*h*4);resource_png_free(pixels);}free(data);}
        if(stop_request(s)){art->item[0]=0;return;}
    }
    if(stop_request(s))return;
    s->view.artwork=*art;s->view.artwork.revision=++s->art_revision;
}
static bool api(JellyService *s,const char *method,const char *path,const char *body,xf_response *response){
    char request[XF_REQUEST_LIMIT],error[192];xf_endpoint ep;
    if(!endpoint(s,&ep))return false;
    if(xf_api_request(request,sizeof request,&ep,s->settings.device,s->settings.token,method,path,body,0)<0){status(s,"The Jellyfin request is invalid or too large.");return false;}
    bool ok=http_request(&ep,request,response,stop_request,s,error,sizeof error);xf_wipe(request,sizeof request);
    if(!ok){status(s,error);return false;}
    if(response->status==401){clear_login(s);save(s);status(s,"Session expired. Enter your password in Media Center.");return false;}
    if(response->status<200||response->status>=300){snprintf(s->view.status,sizeof s->view.status,"Jellyfin returned HTTP %d.",response->status);return false;}
    return true;
}
static bool connect_server(JellyService *s){
    xf_endpoint ep;char request[XF_REQUEST_LIMIT],error[192],version[80];xf_response *r=malloc(sizeof *r);
    if(!r){status(s,"Not enough memory to connect.");return false;}
    bool ok=false;if(!endpoint(s,&ep))goto done;
    if(xf_request(request,sizeof request,&ep,s->settings.device,NULL,NULL)<0)goto done;
    if(!http_request(&ep,request,r,stop_request,s,error,sizeof error)){status(s,error);goto done;}
    if(!xf_public_info(r,s->view.server_name,sizeof s->view.server_name,version,sizeof version)){status(s,"This address did not return Jellyfin server information.");goto done;}
    if(s->settings.token[0]){
        char path[128],user[65];snprintf(path,sizeof path,"/Users/%s",s->settings.user);
        bool accepted=api(s,"GET",path,NULL,r);
        if(accepted&&xf_json_string(r->body,r->body_size,"Id",user,sizeof user)==1&&!strcmp(user,s->settings.user)){ok=true;goto done;}
        if(stop_request(s))goto done;
        /* A temporary connection/server failure must not erase a usable saved
           token or fall through to an empty-password sign-in attempt. */
        if(accepted){clear_login(s);save(s);status(s,"Saved session is invalid. Enter your password again.");goto done;}
        if(s->settings.token[0]||!s->password[0])goto done;
    }
    if(xf_request(request,sizeof request,&ep,s->settings.device,s->settings.username,s->password)<0){status(s,"Enter your Jellyfin username and password in Media Center.");goto done;}
    if(!http_request(&ep,request,r,stop_request,s,error,sizeof error)){status(s,error);goto done;}
    char token[2049],user[65],name[JELLY_TEXT];
    if(!xf_auth_info(r,token,sizeof token,user,sizeof user,name,sizeof name)||!token_ok(token)||!valid_id(user)){
        status(s,r->status==401?"Incorrect username or password.":"Jellyfin returned invalid sign-in information.");xf_wipe(token,sizeof token);goto done;
    }
    snprintf(s->settings.token,sizeof s->settings.token,"%s",token);snprintf(s->settings.user,sizeof s->settings.user,"%s",user);xf_wipe(token,sizeof token);ok=true;
done:
    xf_wipe(request,sizeof request);xf_wipe(r,sizeof *r);free(r);
    if(ok){s->view.state=JELLY_CONNECTED;xf_wipe(s->password,sizeof s->password);status(s,"Connected. Browse Video > Jellyfin.");save(s);}
    else if(!stop_request(s))s->view.state=JELLY_ERROR;
    return ok;
}
bool jellyfin_playable(const xf_item *i){return i&&!i->folder&&(!strcmp(i->type,"Movie")||!strcmp(i->type,"Episode")||!strcmp(i->type,"Video")||!strcmp(i->media,"Video"));}
static bool field(xf_json_ref o,const char *name,char *text,size_t cap){return xf_json_string(o.data,o.size,name,text,cap)==1;}
static bool flag(xf_json_ref o,const char *name){xf_json_ref v;int value=0;return xf_json_get(o,name,&v)==1&&xf_json_bool(v,&value)&&value;}
static uint64_t number(xf_json_ref o,const char *name){xf_json_ref v;uint64_t value=0;if(xf_json_get(o,name,&v)==1)xf_json_uint(v,&value);return value;}
static void cleanup(JellyService *s){
    if(!s->playing.active)return;
    pthread_mutex_lock(&s->lock);s->play_ready=false;xf_wipe(&s->playback,sizeof s->playback);pthread_mutex_unlock(&s->lock);
    xf_response *r=malloc(sizeof *r);if(!r)return;
    if(s->settings.token[0]){
        char body[640],path[256];uint64_t ticks=(uint64_t)(fmax(0,s->playing.position)*10000000);
        if(s->playing.started){snprintf(body,sizeof body,"{\"ItemId\":\"%s\",\"MediaSourceId\":\"%s\",\"PlaySessionId\":\"%s\",\"PositionTicks\":%llu}",s->playing.item,s->playing.source,s->playing.session,(unsigned long long)ticks);(void)api(s,"POST","/Sessions/Playing/Stopped",body,r);}
        snprintf(path,sizeof path,"/Videos/ActiveEncodings?DeviceId=%s&PlaySessionId=%s",s->settings.device,s->playing.session);(void)api(s,"DELETE",path,NULL,r);
    }
    xf_wipe(&s->playing,sizeof s->playing);free(r);
    pthread_mutex_lock(&s->lock);s->play_ready=false;pthread_mutex_unlock(&s->lock);
}
static void play(JellyService *s,unsigned row){
    if(row>=s->browser.page.count||!jellyfin_playable(s->browser.page.items+row)){status(s,"Select a movie or episode to play.");return;}
    const xf_item *item=s->browser.page.items+row;xf_response *r=malloc(sizeof *r);if(!r){status(s,"Not enough memory for playback.");return;}
    char body[2301],path[1501];VideoHttpInput prepared_input={0};
    /* Read fresh user data: a cached page may predate a playback or a second
       Jellyfin client. End-of-item positions start over. */
    snprintf(path,sizeof path,"/Users/%s/Items/%s",s->settings.user,item->id);
    if(!api(s,"GET",path,NULL,r))goto done;
    xf_json_ref detail={r->body,r->body_size},user_data={0};char detail_id[65];
    if(!field(detail,"Id",detail_id,sizeof detail_id)||strcmp(detail_id,item->id)||xf_json_get(detail,"UserData",&user_data)<0){status(s,"Jellyfin returned invalid resume information.");goto done;}
    uint64_t runtime=number(detail,"RunTimeTicks");if(!runtime)runtime=item->ticks;
    uint64_t resume=user_data.data?number(user_data,"PlaybackPositionTicks"):0;
    if(!runtime||resume>=runtime)resume=0;
    if(resume>100000000000000000ULL){status(s,"Invalid Jellyfin playback position.");goto done;}
    prepared_input.start_position=resume/10000000.;
    snprintf(body,sizeof body,"{\"UserId\":\"%s\",\"IsPlayback\":true,\"AutoOpenLiveStream\":false,\"StartTimeTicks\":%llu,"
        "\"EnableDirectPlay\":false,\"EnableDirectStream\":false,\"EnableTranscoding\":true,\"AllowVideoStreamCopy\":false,\"AllowAudioStreamCopy\":false,"
        "\"MaxStreamingBitrate\":8500000,\"DeviceProfile\":{\"Name\":\"Blades MPEG2 720p\",\"MaxStreamingBitrate\":8500000,\"DirectPlayProfiles\":[],"
        "\"TranscodingProfiles\":[{\"Type\":\"Video\",\"Container\":\"ts\",\"Protocol\":\"http\",\"VideoCodec\":\"mpeg2video\",\"AudioCodec\":\"mp2\",\"MaxAudioChannels\":\"2\",\"Context\":\"Streaming\"}],"
        "\"CodecProfiles\":[{\"Type\":\"Video\",\"Codec\":\"mpeg2video\",\"Conditions\":["
        "{\"Condition\":\"LessThanEqual\",\"Property\":\"Width\",\"Value\":\"1280\"},"
        "{\"Condition\":\"LessThanEqual\",\"Property\":\"Height\",\"Value\":\"720\"},"
        "{\"Condition\":\"LessThanEqual\",\"Property\":\"VideoFramerate\",\"Value\":\"30\"}]}],\"SubtitleProfiles\":[]}}",s->settings.user,(unsigned long long)resume);
    snprintf(path,sizeof path,"/Items/%s/PlaybackInfo",item->id);
    if(!api(s,"POST",path,body,r))goto done;
    xf_json_ref root={r->body,r->body_size},array,sources[8];size_t count=0;
    char session[65];if(!field(root,"PlaySessionId",session,sizeof session)||!valid_id(session)||xf_json_get(root,"MediaSources",&array)!=1||!xf_json_elements(array,0,sources,8,&count)||!count||count>8){status(s,"Jellyfin returned invalid playback information.");goto done;}
    bool prepared=false;
    for(size_t i=0;i<count;i++){
        char source[65],protocol[32],container[40];xf_json_ref list,streams[32];size_t n=0;unsigned vi=0,ai=0;bool video=false,audio=false;
        if(!flag(sources[i],"SupportsTranscoding")||flag(sources[i],"IsInfiniteStream")||!field(sources[i],"Id",source,sizeof source)||!valid_id(source)||
            !field(sources[i],"Protocol",protocol,sizeof protocol)||strcmp(protocol,"File")||
            !field(sources[i],"TranscodingSubProtocol",protocol,sizeof protocol)||strcmp(protocol,"http")||
            !field(sources[i],"TranscodingContainer",container,sizeof container)||strcmp(container,"ts")||
            xf_json_get(sources[i],"MediaStreams",&list)!=1||!xf_json_elements(list,0,streams,32,&n)||n>32)continue;
        for(size_t j=0;j<n;j++){char type[24];if(!field(streams[j],"Type",type,sizeof type))continue;
            uint64_t index=number(streams[j],"Index");if(index>255)continue;
            if(!strcmp(type,"Video")&&!flag(streams[j],"IsAttachedPic")&&!video){video=true;vi=(unsigned)index;}
            else if(!strcmp(type,"Audio")&&!audio){audio=true;ai=(unsigned)index;}}
        if(!video)continue;
        int used=snprintf(path,sizeof path,"/Videos/%s/stream.ts?Static=false&MediaSourceId=%s&DeviceId=%s&PlaySessionId=%s&VideoCodec=mpeg2video&VideoStreamIndex=%u"
            "&AudioCodec=mp2&AudioBitRate=192000&AudioSampleRate=48000&MaxAudioChannels=2&SubtitleStreamIndex=-1&EnableAutoStreamCopy=false&AllowVideoStreamCopy=false&AllowAudioStreamCopy=false"
            "&MaxWidth=1280&MaxHeight=720&Framerate=30&VideoBitRate=8000000&RequireNonAnamorphic=true&StartTimeTicks=%llu",item->id,source,s->settings.device,session,vi,(unsigned long long)resume);
        if(used<0||(size_t)used>=sizeof path)continue;
        if(audio)snprintf(path+used,sizeof path-(size_t)used,"&AudioStreamIndex=%u",ai);
        if(!endpoint(s,&prepared_input.endpoint)||xf_api_request(prepared_input.request,sizeof prepared_input.request,&prepared_input.endpoint,s->settings.device,s->settings.token,"GET",path,NULL,2)<0)continue;
        prepared_input.duration=runtime/10000000.;
        snprintf(path,sizeof path,"/Videos/ActiveEncodings?DeviceId=%s&PlaySessionId=%s",s->settings.device,session);
        if(xf_api_request(prepared_input.stop_request,sizeof prepared_input.stop_request,&prepared_input.endpoint,s->settings.device,s->settings.token,"DELETE",path,NULL,0)<0)continue;
        s->playing=(Playing){.active=true,.position=prepared_input.start_position};snprintf(s->playing.item,sizeof s->playing.item,"%s",item->id);snprintf(s->playing.source,sizeof s->playing.source,"%s",source);snprintf(s->playing.session,sizeof s->playing.session,"%s",session);
        prepared=true;break;
    }
    if(prepared&&!stop_request(s)){pthread_mutex_lock(&s->lock);s->playback=prepared_input;s->play_ready=true;memset(&s->progress,0,sizeof s->progress);pthread_mutex_unlock(&s->lock);status(s,"Starting Jellyfin video...");}
    else status(s,"No compatible progressive stream. Enable video transcoding for this user.");
done:
    xf_wipe(&prepared_input,sizeof prepared_input);xf_wipe(r,sizeof *r);free(r);
}
static void browse(JellyService *s,JellyCommand c){
    char path[1501];s->browser.current.selected=c.row;
    enum xf_browse_action a=c.type==JELLY_ROOT?XF_ROOT:c.type==JELLY_REFRESH?XF_REFRESH:c.type==JELLY_BACK?XF_BACK:c.type==JELLY_NEXT?XF_NEXT:c.type==JELLY_PREVIOUS?XF_PREVIOUS:XF_OPEN;
    int ready=xf_browser_request(&s->browser,a,s->settings.user,path,sizeof path);
    if(ready<0){status(s,s->browser.message);return;}if(!ready)return;
    xf_response *r=malloc(sizeof *r);if(!r){s->browser.busy=false;status(s,"Not enough memory for library data.");return;}
    if(api(s,"GET",path,NULL,r)){int result=xf_browser_result(&s->browser,r,NULL);if(result>0)status(s,s->browser.page.count?"Select a folder or video.":"No videos in this folder.");else status(s,s->browser.message);}
    else s->browser.busy=false;
    xf_wipe(r,sizeof *r);free(r);
}
static void report(JellyService *s,const VideoSnapshot *p){
    if(!s->playing.active||!p->presented||!s->settings.token[0])return;
    char body[800];xf_response *r=malloc(sizeof *r);if(!r)return;
    s->playing.position=p->position;s->playing.paused=p->state==VIDEO_PAUSED;
    snprintf(body,sizeof body,"{\"ItemId\":\"%s\",\"MediaSourceId\":\"%s\",\"PlaySessionId\":\"%s\",\"PositionTicks\":%llu,\"IsPaused\":%s,\"CanSeek\":true,\"PlayMethod\":\"Transcode\",\"VolumeLevel\":100,\"IsMuted\":false,\"RepeatMode\":\"RepeatNone\"}",s->playing.item,s->playing.source,s->playing.session,(unsigned long long)(fmax(0,p->position)*10000000),s->playing.paused?"true":"false");
    if(api(s,"POST",s->playing.started?"/Sessions/Playing/Progress":"/Sessions/Playing",body,r))s->playing.started=true;
    free(r);
}
static void *worker(void *context){
    JellyService *s=context;if(s->hooks.worker_start)s->hooks.worker_start(s->hooks.context);
    pthread_mutex_lock(&s->lock);s->current_type=JELLY_CONNECT;pthread_mutex_unlock(&s->lock);
    load(s);xf_browser_init(&s->browser);status(s,"Connect to Jellyfin in Media Center.");
    if(s->settings.token[0]){s->view.state=JELLY_CONNECTING;publish(s);s->request_epoch=__atomic_load_n(&s->epoch,__ATOMIC_ACQUIRE);connect_server(s);}
    pthread_mutex_lock(&s->lock);s->busy=s->pending;s->current_type=JELLY_NONE;pthread_mutex_unlock(&s->lock);publish(s);
    for(;;){
        pthread_mutex_lock(&s->lock);while(!s->pending&&!s->report_due&&!s->stop){
            if(s->preview_pending){double remaining=s->preview_at-now();if(remaining<=0)break;
                struct timespec until;clock_gettime(CLOCK_REALTIME,&until);until.tv_nsec+=(long)(remaining*1e9);until.tv_sec+=until.tv_nsec/1000000000;until.tv_nsec%=1000000000;
                pthread_cond_timedwait(&s->wake,&s->lock,&until);
            }else pthread_cond_wait(&s->wake,&s->lock);
        }
        if(s->stop){pthread_mutex_unlock(&s->lock);break;}
        if(!s->pending&&!s->report_due&&s->preview_pending){
            char id[65];snprintf(id,sizeof id,"%s",s->selected_art);s->preview_pending=false;s->request_epoch=s->epoch;s->current_type=JELLY_PREVIEW;pthread_mutex_unlock(&s->lock);
            preview(s,id);bool accepted=!stop_request(s);
            pthread_mutex_lock(&s->lock);s->current_type=JELLY_NONE;pthread_mutex_unlock(&s->lock);if(accepted)publish(s);continue;
        }
        JellyCommand c=s->command;xf_wipe(&s->command,sizeof s->command);bool command=s->pending;s->pending=false;s->report_due=false;
        VideoSnapshot progress=s->progress;s->request_epoch=s->epoch;s->busy=true;s->current_type=command?c.type:JELLY_NONE;pthread_mutex_unlock(&s->lock);
        s->view.state=c.type==JELLY_CONNECT&&command?JELLY_CONNECTING:s->view.state;publish(s);
        if(!command)report(s,&progress);
        else if(c.type==JELLY_EDIT){
            cleanup(s);clear_login(s);
            if(c.field==1){snprintf(s->settings.server,sizeof s->settings.server,"%s",c.text);xf_wipe(s->password,sizeof s->password);}
            if(c.field==2){snprintf(s->settings.username,sizeof s->settings.username,"%s",c.text);xf_wipe(s->password,sizeof s->password);}
            if(c.field==3)snprintf(s->password,sizeof s->password,"%s",c.text);
            save(s);status(s,"Settings updated. Select Connect to sign in.");
        }else if(c.type==JELLY_CONNECT)connect_server(s);
        else if(c.type==JELLY_DISCONNECT||c.type==JELLY_REMOVE){cleanup(s);clear_login(s);xf_wipe(s->password,sizeof s->password);if(c.type==JELLY_REMOVE){s->settings.server[0]=s->settings.username[0]=s->view.server_name[0]=0;}save(s);status(s,"Disconnected.");}
        else if(c.type==JELLY_STOP){s->playing.position=c.position;cleanup(s);
            if(s->browser.loaded&&s->view.state==JELLY_CONNECTED&&!stop_request(s))browse(s,(JellyCommand){.type=JELLY_REFRESH,.row=s->browser.current.selected});
            else status(s,s->view.state==JELLY_CONNECTED?"Select a folder or video.":"Connect to Jellyfin in Media Center.");}
        else if(c.type==JELLY_CANCEL)status(s,s->view.state==JELLY_CONNECTED?"Select a folder or video.":"Connect to Jellyfin in Media Center.");
        else if(s->view.state!=JELLY_CONNECTED)status(s,"Connect to Jellyfin under Media Center first.");
        else if(c.type==JELLY_PLAY)play(s,c.row);
        else browse(s,c);
        xf_wipe(&c,sizeof c);
        if(stop_request(s)){s->browser.busy=false;status(s,"Request cancelled.");if(s->view.state==JELLY_CONNECTING)s->view.state=JELLY_OFF;}
        pthread_mutex_lock(&s->lock);s->busy=s->pending;s->current_type=JELLY_NONE;pthread_cond_broadcast(&s->wake);pthread_mutex_unlock(&s->lock);publish(s);
    }
    return NULL;
}
JellyService *jellyfin_open(const char *config,const JellyHooks *hooks){
    JellyService *s=calloc(1,sizeof *s);if(!s)return NULL;if(config)snprintf(s->config,sizeof s->config,"%s",config);if(hooks)s->hooks=*hooks;
    s->busy=true;pthread_mutex_init(&s->lock,NULL);pthread_cond_init(&s->wake,NULL);
    pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,512*1024);int result=pthread_create(&s->worker,&attr,worker,s);pthread_attr_destroy(&attr);
    if(result){pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->lock);free(s);return NULL;}
    return s;
}
const JellySnapshot *jellyfin_poll(JellyService *s){
    if(!s)return NULL;
    pthread_mutex_lock(&s->lock);if(s->pending_view){free(s->front);s->front=s->pending_view;s->pending_view=NULL;}const JellySnapshot *v=s->front;pthread_mutex_unlock(&s->lock);return v;
}
bool jellyfin_request(JellyService *s,const JellyCommand *c){
    if(!s||!c||c->type==JELLY_NONE||c->type==JELLY_PREVIEW)return false;
    pthread_mutex_lock(&s->lock);
    bool ok=!s->stop&&!s->pending&&!s->busy;
    if(ok){if(s->current_type==JELLY_PREVIEW)__atomic_add_fetch(&s->epoch,1,__ATOMIC_RELEASE);
        s->selected_art[0]=0;s->preview_pending=false;s->command=*c;s->pending=true;s->busy=true;pthread_cond_broadcast(&s->wake);}
    pthread_mutex_unlock(&s->lock);return ok;
}
void jellyfin_select(JellyService *s,const char *id){
    if(!s)return;
    if(!id||!valid_id(id))id="";
    pthread_mutex_lock(&s->lock);
    if(strcmp(s->selected_art,id)){
        if(s->current_type==JELLY_PREVIEW)__atomic_add_fetch(&s->epoch,1,__ATOMIC_RELEASE);
        snprintf(s->selected_art,sizeof s->selected_art,"%s",id);s->preview_pending=*id!=0;s->preview_at=now()+.2;pthread_cond_broadcast(&s->wake);
    }
    pthread_mutex_unlock(&s->lock);
}
static bool cancellable(JellyCommandType c){return c==JELLY_CONNECT||(c>=JELLY_ROOT&&c<=JELLY_PLAY);}
void jellyfin_cancel(JellyService *s){
    if(!s)return;
    pthread_mutex_lock(&s->lock);
    s->selected_art[0]=0;s->preview_pending=false;if(s->current_type==JELLY_PREVIEW)__atomic_add_fetch(&s->epoch,1,__ATOMIC_RELEASE);
    if(s->play_ready){s->command=(JellyCommand){.type=JELLY_STOP};s->pending=s->busy=true;}
    else if(cancellable(s->current_type)||(s->pending&&cancellable(s->command.type))){
        __atomic_add_fetch(&s->epoch,1,__ATOMIC_RELEASE);s->command=(JellyCommand){.type=JELLY_CANCEL};s->pending=s->busy=true;
    }
    s->play_ready=false;pthread_cond_broadcast(&s->wake);pthread_mutex_unlock(&s->lock);
}
bool jellyfin_take_playback(JellyService *s,VideoHttpInput *out){if(!s||!out)return false;pthread_mutex_lock(&s->lock);bool ready=s->play_ready;if(ready){*out=s->playback;s->play_ready=false;}pthread_mutex_unlock(&s->lock);return ready;}
void jellyfin_progress(JellyService *s,const VideoSnapshot *v){
    if(!s||!v)return;
    pthread_mutex_lock(&s->lock);bool change=s->progress.state!=v->state||(!s->progress.presented&&v->presented);s->progress=*v;
    if(v->presented&&(change||now()>=s->next_report)){s->next_report=now()+5;s->report_due=true;pthread_cond_broadcast(&s->wake);}pthread_mutex_unlock(&s->lock);
}
void jellyfin_finish(JellyService *s,double position){
    if(!s)return;
    pthread_mutex_lock(&s->lock);__atomic_add_fetch(&s->epoch,1,__ATOMIC_RELEASE);s->command=(JellyCommand){.type=JELLY_STOP,.position=position};s->pending=true;s->play_ready=false;s->report_due=false;pthread_cond_broadcast(&s->wake);pthread_mutex_unlock(&s->lock);
}
void jellyfin_close(JellyService *s){
    if(!s)return;
    pthread_mutex_lock(&s->lock);double position=s->progress.position;pthread_mutex_unlock(&s->lock);
    jellyfin_finish(s,position);
    struct timespec deadline;clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=2;
    pthread_mutex_lock(&s->lock);
    while((s->pending||s->busy)&&pthread_cond_timedwait(&s->wake,&s->lock,&deadline)!=ETIMEDOUT){}
    __atomic_store_n(&s->stop,1,__ATOMIC_RELEASE);pthread_cond_broadcast(&s->wake);pthread_mutex_unlock(&s->lock);
    pthread_join(s->worker,NULL);free(s->front);free(s->pending_view);pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->lock);xf_wipe(s,sizeof *s);free(s);
}
const char *jellyfin_status(const JellySnapshot *s){return !s?"Unavailable":s->busy?"Working...":s->state==JELLY_CONNECTED?"Connected":s->state==JELLY_CONNECTING?"Connecting...":s->state==JELLY_ERROR?"Connection error":"Disconnected";}
