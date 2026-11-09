#include "blades.h"
#include "resources.h"
#include "xbox_renderer.h"
#include "xbox_disc.h"
#include "xbox_library.h"
#include "xbox_profile.h"
#include "xbox_achievements.h"
#include "xbox_ftp.h"
#include "audio_boundary.h"
#include "video.h"
#include "update_install.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/texture.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include "xsdk/audio.h"
#include "xsdk/thread.h"
static VideoPlayer *app_video;
static unsigned audio_stop;
static unsigned audio_queued;
static uint64_t microseconds(void) {
    struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
    return (uint64_t)now.tv_sec*1000000+(unsigned long)now.tv_nsec/1000;
}
static uint32_t feed_audio(void *context) {
    (void)context;
    AudioBoundary boundary={0};unsigned priming=8,timeouts=0;
    while(!boundary.finished) {
        float *samples=NULL;
        NTSTATUS status=xsdk_audio_next_frame(100,&samples);
        if(status==STATUS_IO_TIMEOUT){if(__atomic_load_n(&audio_stop,__ATOMIC_ACQUIRE)&&++timeouts>=3)return 0;continue;}
        if(!NT_SUCCESS(status))return 0;
        timeouts=0;float stereo[512]={0};
        if(priming)priming--;else ui_audio_render(stereo,256);
        audio_boundary_apply(&boundary,stereo,256,__atomic_load_n(&audio_stop,__ATOMIC_ACQUIRE)!=0);
        float movie[256*VIDEO_AUDIO_CHANNELS]={0};VideoPlayer *video=__atomic_load_n(&app_video,__ATOMIC_ACQUIRE);
        bool playing=video&&video_audio_render(video,movie,256);unsigned channels=video_audio_channels(video);
        for(unsigned channel=0;channel<6;channel++)for(unsigned j=0;j<256;j++)
            samples[channel*256+j]=playing?(channel<channels?movie[j*channels+channel]:0):(channel<2?stereo[j*2+channel]:0);
        xsdk_audio_queue_frame();
        __atomic_add_fetch(&audio_queued,1,__ATOMIC_RELEASE);
    }
    return 0;
}
static HANDLE start_audio(void) {
    __atomic_store_n(&audio_stop,0,__ATOMIC_RELEASE);__atomic_store_n(&audio_queued,0,__ATOMIC_RELEASE);
    ui_audio_init();
    if(!NT_SUCCESS(xsdk_audio_open(8)))return 0;
    HANDLE feeder=0;
    if(!NT_SUCCESS(xsdk_thread_create(&feeder,64*1024,feed_audio,NULL)))xsdk_audio_close();
    return feeder;
}
static void stop_audio(HANDLE feeder) {
    if(feeder) {
        __atomic_store_n(&audio_stop,1,__ATOMIC_RELEASE);
        /* Fade to zero and send silent frames before unregistering the driver. */
        (void)NtWaitForSingleObjectEx(feeder,USER_MODE,0,NULL);
        (void)NtClose(feeder);
        uint64_t deadline=microseconds()+250000;unsigned target=__atomic_load_n(&audio_queued,__ATOMIC_ACQUIRE);XsdkAudioStats stats;
        do {xsdk_audio_stats(&stats);if(stats.played>=target)break;struct timespec pause={0,2000000};nanosleep(&pause,NULL);}while(microseconds()<deadline);
        struct timespec tail={0,12000000};nanosleep(&tail,NULL);
        xsdk_log_write("audio: silent tail drained queued=%u played=%u underruns=%u",target,stats.played,stats.silent);
        xsdk_audio_close();
    }
    ui_audio_close();
}
/* xsdk_launch exits through atexit before handing the GPU back to XAM. */
static XboxRenderer *app_renderer;
static XboxDisc *app_disc;
static XboxLibrary *app_library;
static XboxProfile *app_profile;
static XboxAchievements *app_achievements;
static FtpServer *app_ftp;
static JellyService *app_jelly;
static Updater *app_updater;
static char profile_error[256];
static char update_error[256];
static HANDLE app_audio;
static bool app_resources;
static bool app_present;
/* Present black before releasing anything the last picture could reference.
   Draining two initialized buffers keeps teardown workers off the scanout. */
static void black_handoff(void){
    if(!app_present)return;
    const unsigned words=xsdk_gpu_resolve_bytes(WIDTH,HEIGHT)/4;
    XsdkPresentStats stats;xsdk_present_stats(&stats);uint32_t until_vblank=stats.vblanks+3;
    for(unsigned frame=0;frame<2;frame++){
        uint32_t address;if(!NT_SUCCESS(xsdk_present_begin(&address,250)))break;
        volatile uint32_t *pixels=xsdk_present_pixels(address);
        /* Include the tiled surface's padded rows, not only logical pixels. */
        for(unsigned i=0;i<words;i++)pixels[i]=0x000000ff;
        __asm__ volatile("sync" ::: "memory");xsdk_present_end();
    }
    uint32_t fence=xsdk_gpu_fence();NTSTATUS status=xsdk_gpu_wait(fence,1000);
    uint64_t until=microseconds()+100000;
    do{xsdk_present_stats(&stats);if(stats.vblanks>=until_vblank)break;struct timespec pause={0,1000000};nanosleep(&pause,NULL);}while(microseconds()<until);
    xsdk_log_write("display-handoff: black fence=%08X flips=%u vblanks=%u",(unsigned)status,stats.flips,stats.vblanks);
}
static char disc_launch_error[256];
static void close_app(void) {
    if(app_present){black_handoff();app_present=false;}
    updater_close(app_updater);app_updater=NULL;
    VideoPlayer *video=__atomic_load_n(&app_video,__ATOMIC_ACQUIRE);video_stop(video);
    jellyfin_close(app_jelly);app_jelly=NULL;
    ftp_close(app_ftp);app_ftp=NULL;
    xbox_achievements_close(app_achievements);app_achievements=NULL;
    xbox_profile_close(app_profile);app_profile=NULL;
    xbox_library_close(app_library);app_library=NULL;
    xbox_disc_close(app_disc);app_disc=NULL;
    stop_audio(app_audio);app_audio=0;
    video_close(__atomic_exchange_n(&app_video,NULL,__ATOMIC_ACQ_REL));
    xbox_renderer_close(app_renderer);app_renderer=NULL;
    if(app_resources){resources_close();app_resources=false;}
}
static void video_log(const char *line){xsdk_log_write("%s",line);}
static void keyboard_text(uint16_t *out,size_t count,const char *text){size_t i=0;while(i+1<count&&text[i]){out[i]=(unsigned char)text[i];i++;}out[i]=0;}
static void jelly_worker(void *context){(void)context;(void)xsdk_thread_move(1);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-1,NULL);}
static void update_worker(void *context){(void)context;(void)xsdk_thread_move(5);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-1,NULL);}
static unsigned buttons(const INPUT_GAMEPAD *p) {
    unsigned a=0;
    if((p->buttons&INPUT_GAMEPAD_DPAD_UP)||p->thumb_ly>16000)a|=UP;
    if((p->buttons&INPUT_GAMEPAD_DPAD_DOWN)||p->thumb_ly< -16000)a|=DOWN;
    if((p->buttons&(INPUT_GAMEPAD_DPAD_LEFT|INPUT_GAMEPAD_LEFT_SHOULDER))||p->thumb_lx< -16000)a|=LEFT;
    if((p->buttons&(INPUT_GAMEPAD_DPAD_RIGHT|INPUT_GAMEPAD_RIGHT_SHOULDER))||p->thumb_lx>16000)a|=RIGHT;
    if(p->buttons&INPUT_GAMEPAD_A)a|=SELECT;
    if(p->buttons&INPUT_GAMEPAD_B)a|=BACK;
    if(p->buttons&INPUT_GAMEPAD_X)a|=PROFILE;
    if(p->buttons&INPUT_GAMEPAD_Y)a|=EJECT;
    if(p->buttons&INPUT_GAMEPAD_BACK)a|=EXIT;
    return a;
}
int main(void) {
    xsdk_log_write("Blades 0.53.0: boot; embedded video decoding; no video XEX handoff or adapter");
    if(atexit(close_app)!=0)return 1;
    if(update_install_pending("game:")){
        xsdk_log_write("updater: recovering interrupted installation before loading artwork");
        (void)xsdk_launch("game:/" UPDATE_INSTALL_HELPER,NULL,0);
        snprintf(update_error,sizeof update_error,"The update installer could not start. Reinstall the complete USB package.");
    }else{
        FILE *result=fopen("game:/" UPDATE_INSTALL_WORK "/result.txt","rb");
        if(result){size_t n=fread(update_error,1,sizeof update_error-1,result);update_error[n]=0;fclose(result);remove("game:/" UPDATE_INSTALL_WORK "/result.txt");}
    }
    NTSTATUS status=xsdk_gpu_open();
    if(!NT_SUCCESS(status)) {xsdk_log_write("gpu_open: %08X",(unsigned)status);return 1;}
    uint32_t w=0,h=0;
    status=xsdk_present_open(XSDK_DISPLAY_720P,2,1,&w,&h);
    if(!NT_SUCCESS(status)||w!=WIDTH||h!=HEIGHT) {
        xsdk_log_write("present_open: %08X size %ux%u",(unsigned)status,(unsigned)w,(unsigned)h);
        xsdk_gpu_close();return 1;
    }
    app_present=true;
    app_resources=true;
    if(!resources_open("game:/Assets/blades.xzp"))resources_open("game:/Assets/shrdres.xzp");
    XboxRenderer *renderer=xbox_renderer_open();
    if(!renderer){app_present=false;xsdk_present_close();xsdk_gpu_close();resources_close();app_resources=false;return 1;}
    app_renderer=renderer;
    HANDLE audio=start_audio();app_audio=audio;
    if(!audio)xsdk_log_write("audio: unavailable, continuing silently");
    XsdkAudioSpeakers speakers=XSDK_AUDIO_SPEAKERS_STEREO;(void)xsdk_audio_speakers(&speakers);
    VideoConfig video_config={.channels=speakers==XSDK_AUDIO_SPEAKERS_SURROUND?6:2,.audio_available=audio!=0,.audio_latency_frames=8*256,.gpu_yuv=true};
    video_config.log=video_log;
    __atomic_store_n(&app_video,video_create(&video_config),__ATOMIC_RELEASE);
    Dashboard d;dashboard_begin_startup(&d);
    app_jelly=jellyfin_open("game:/Config/jellyfin-settings.json",&(JellyHooks){.worker_start=jelly_worker});
    app_updater=updater_open("game:/Updates",update_worker,NULL);
    (void)updater_install_to(app_updater,"game:");
    if(update_error[0]){d.dialog=true;d.message=update_error;}
    Input in={0};
    app_disc=xbox_disc_open();xbox_disc_status(app_disc,&d.disc);
    app_library=xbox_library_open();
    LibraryService *library=xbox_library_service(app_library);uint64_t library_log_time=0;bool library_was_busy=false;
    dashboard_set_library(&d,library_poll(library));
    app_profile=xbox_profile_open();ProfileService *profiles=xbox_profile_service(app_profile);
    dashboard_set_profile(&d,profile_poll(profiles));
    app_achievements=xbox_achievements_open();AchievementService *achievements=xbox_achievements_service(app_achievements);
    bool ui_before=false,guide_pending=false,guide_seen=false;uint64_t guide_started=0;
    unsigned profile_error_seen=0;
    XAM_OVERLAPPED ftp_keyboard={0};uint16_t ftp_text[FTP_LOGIN_BYTES]={0},ftp_default[FTP_LOGIN_BYTES]={0},ftp_title[40]={0},ftp_description[96]={0};
    unsigned ftp_field=0,ftp_key_result=0;bool ftp_key_pending=false;
    XAM_OVERLAPPED jelly_keyboard={0};uint16_t jelly_text[JELLY_TEXT]={0},jelly_default[JELLY_TEXT]={0},jelly_title[40]={0},jelly_description[96]={0};
    unsigned jelly_field=0,jelly_key_result=0;bool jelly_key_pending=false;
    xsdk_log_write("startup: blade opening to Games root; no setup/language menu");
    struct timespec last;clock_gettime(CLOCK_MONOTONIC,&last);
    bool connected_before=false; int controller=-1, result=0;
    uint64_t window_start=microseconds(),wait_us=0;XboxRenderStats totals={0};
    unsigned measured_frames=0;XsdkPresentStats previous_present={0};
    xsdk_log_write("render: initialised 1280x720 GPU; Back opens confirmed exit; B returns");
    (void)xsdk_system_ui_open();
    for(unsigned frame=0;!d.exit_requested;frame++) {
        INPUT_STATE state={0};bool connected=false;
        if(controller>=0)connected=XamInputGetState((unsigned)controller,0,&state)==ERROR_SUCCESS;
        int previous_controller=controller;
        if(!connected) {
            controller=-1;
            for(unsigned i=0;i<4;i++) if(XamInputGetState(i,0,&state)==ERROR_SUCCESS) {controller=(int)i;connected=true;break;}
        }
        if(previous_controller!=controller)in=(Input){0};
        if(connected!=connected_before)xsdk_log_write("controller: %s slot %d",connected?"connected":"disconnected",controller);
        connected_before=connected;
        struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
        float dt=(float)(now.tv_sec-last.tv_sec)+(float)(now.tv_nsec-last.tv_nsec)/1e9f;last=now;
        bool guide=xsdk_system_ui_open();
        if(guide_pending) {
            guide_seen|=guide;
            if((guide_seen&&!guide)||(!guide_seen&&microseconds()-guide_started>2000000)) {
                guide_pending=false;profile_refresh(profiles);in=(Input){0};
            }
        }
        if(ftp_key_pending){
            unsigned error=ftp_key_result==997?xsdk_xam_wait(&ftp_keyboard,0,NULL):ftp_key_result;
            if(error!=996){
                ftp_key_pending=false;in=(Input){0};
                if(!error&&!ftp_keyboard.extended_error){
                    char value[FTP_LOGIN_BYTES];bool valid=true;unsigned n=0;
                    while(n<FTP_LOGIN_BYTES-1&&ftp_text[n]){if(ftp_text[n]<32||ftp_text[n]>126){valid=false;break;}value[n]=(char)ftp_text[n];n++;}value[n]=0;
                    ftp_poll(app_ftp,&d.ftp);
                    if(!valid||!ftp_credentials(app_ftp,ftp_field==1?value:d.ftp.username,ftp_field==2?value:d.ftp.password)){d.dialog=true;d.message="Use 1 to 63 printable characters for FTP login details.\nPress B to return.";}
                }
                xsdk_log_write("ftp: keyboard field=%u result=%u cancelled_or_error=%u",ftp_field,error,ftp_keyboard.extended_error);
            }
        }
        if(jelly_key_pending){
            unsigned error=jelly_key_result==997?xsdk_xam_wait(&jelly_keyboard,0,NULL):jelly_key_result;
            if(error!=996){
                jelly_key_pending=false;in=(Input){0};
                if(!error&&!jelly_keyboard.extended_error){
                    JellyCommand edit={.type=JELLY_EDIT,.field=jelly_field};bool valid=true;unsigned n=0;
                    while(n<JELLY_TEXT-1&&jelly_text[n]){if(jelly_text[n]<32||jelly_text[n]>126){valid=false;break;}edit.text[n]=(char)jelly_text[n];n++;}edit.text[n]=0;
                    if(!valid||!jellyfin_request(app_jelly,&edit)){d.dialog=true;d.message="Use up to 128 printable ASCII characters.\nPress B to return.";}
                    xf_wipe(&edit,sizeof edit);
                }
                xf_wipe(jelly_text,sizeof jelly_text);xf_wipe(jelly_default,sizeof jelly_default);
            }
        }
        bool ui=guide||guide_pending||ftp_key_pending||jelly_key_pending;
        bool ui_closed=ui_before&&!ui;
        if(ui_closed){in=(Input){0};profile_refresh(profiles);
            if(controller<0||!XamUserGetSigninState((unsigned)controller))d.achievement_signin_pending=false;
        }
        ui_before=ui;
        if(controller>=0)profile_set_user(profiles,(unsigned)controller);
        unsigned old_profile_revision=d.profile_revision;
        dashboard_set_profile(&d,profile_poll(profiles));
        if(d.profile&&d.profile_revision!=old_profile_revision) {
            xsdk_log_write("profile: slot=%u state=%u fields=%u picture=%u loading=%u busy=%u profiles_known=%u profiles=%u",d.profile->identity.user,d.profile->identity.state,d.profile->known,d.profile->has_picture,d.profile->loading,d.profile->busy,d.profile->profiles_known,d.profile->available_profiles);
            if(d.profile->error_serial!=profile_error_seen) {
                profile_error_seen=d.profile->error_serial;
                snprintf(profile_error,sizeof profile_error,"%s\nPress B to return.",d.profile->error);
                d.dialog=true;d.message=profile_error;
            }
        }
        achievements_set_user(achievements,d.profile?d.profile->identity.user:0,profile_signed_in(d.profile)?d.profile->identity.xuid:0);
        dashboard_set_achievements(&d,achievements_poll(achievements));
        if(ui_closed&&d.achievement_page&&d.achievements&&!d.achievements->busy&&!d.achievements->error&&d.achievements->title_count&&profile_signed_in(d.profile)){
            AchievementRequest refresh={.type=ACH_IMAGES,.user=d.profile->identity.user,.xuid=d.profile->identity.xuid,.title_id=d.achievements->title_id,.first=(unsigned)d.achievement_scroll,.game_first=(unsigned)d.achievement_game/3*3};
            (void)achievements_request(achievements,refresh);
        }
        unsigned a=input_update(&in,connected?buttons(&state.gamepad):0,dt,connected&&!ui&&!d.startup_active);
        bool was_starting=d.startup_active;
        xbox_disc_status(app_disc,&d.disc);
        unsigned previous_library_revision=d.library_revision;
        dashboard_set_library(&d,library_poll(library));
        uint64_t library_now=microseconds();
        if(d.library&&d.library_revision!=previous_library_revision&&
           (d.library->busy!=library_was_busy||!d.library->busy||library_now-library_log_time>=1000000)) {
            xsdk_log_write("library: revision=%u busy=%u roots=%u titles=%u errors=%u %s",d.library_revision,d.library->busy,(unsigned)d.library->root_count,(unsigned)d.library->count,d.library->errors,d.library->status);
            if(d.library->progress.scanning)xsdk_log_write("library scan: folders=%u files=%u depth=%u path=%s",d.library->progress.folders,d.library->progress.files,d.library->progress.depth,d.library->progress.path);
            library_log_time=library_now;library_was_busy=d.library->busy;
        }
        bool was_achievements=d.achievement_page;
        bool old_update=dashboard_update_page(&d);updater_poll(app_updater,&d.update);
        if(d.update.state==UPDATE_READY&&!ui){
            xsdk_log_write("updater: verified update staged; launching local installer");
            (void)xsdk_launch("game:/" UPDATE_INSTALL_HELPER,NULL,0);
            updater_install_failed(app_updater,"Could not start the installer. Restart Blades to retry recovery.");
        }
        JellyPage old_jelly_page=d.jelly_page;
        bool was_jelly_video=d.video_view&&d.video.network;
        dashboard_set_jellyfin(&d,jellyfin_poll(app_jelly));
        if(!dashboard_video_update(&d,app_video,a,dt,ui))dashboard_update(&d,a,dt);
        if(d.video_view&&d.video.network)jellyfin_progress(app_jelly,&d.video);
        else if(was_jelly_video)jellyfin_finish(app_jelly,d.video.position);
        if(old_jelly_page!=JELLY_UI_NONE&&d.jelly_page==JELLY_UI_NONE)jellyfin_cancel(app_jelly);
        dashboard_jellyfin_dispatch(&d,app_jelly);
        if(d.update_check)(void)updater_check(app_updater);
        if(d.update_download)(void)updater_download(app_updater);
        if(d.update_cancel||(old_update&&!dashboard_update_page(&d)))updater_cancel(app_updater);
        if(d.jelly_edit&&!jelly_key_pending){
            jelly_field=d.jelly_edit;memset(jelly_text,0,sizeof jelly_text);
            const char *value=d.jelly&&jelly_field==1?d.jelly->server:d.jelly&&jelly_field==2?d.jelly->username:"";
            keyboard_text(jelly_default,JELLY_TEXT,value);keyboard_text(jelly_title,40,jelly_field==1?"Jellyfin Server Address":jelly_field==2?"Jellyfin Username":"Jellyfin Password");
            keyboard_text(jelly_description,96,jelly_field==1?"IPv4 address and port, e.g. 10.0.0.20:8096":"Up to 128 ASCII characters. Passwords are not saved.");
            jelly_key_result=xsdk_xam_keyboard(controller>=0?(unsigned)controller:0,jelly_title,jelly_description,jelly_default,jelly_text,JELLY_TEXT,&jelly_keyboard);
            if(!jelly_key_result||jelly_key_result==997){jelly_key_pending=true;guide_pending=true;guide_seen=false;guide_started=microseconds();ui=true;in=(Input){0};}
            else {d.dialog=true;d.message="The Xbox keyboard could not be opened.\nPress B to return.";}
        }
        if(!ui&&d.jelly_page==JELLY_UI_BROWSE&&!d.video_view&&!d.dialog&&!d.exit_menu){
            VideoHttpInput stream;if(jellyfin_take_playback(app_jelly,&stream)){
                if(!dashboard_video_http_open(&d,app_video,&stream))jellyfin_finish(app_jelly,0);
                xf_wipe(&stream,sizeof stream);
            }
        }
        if(was_starting&&!d.startup_active)app_ftp=xbox_ftp_open();
        if(app_ftp){
            if(d.ftp_toggle)ftp_enable(app_ftp,!d.ftp.enabled);
            unsigned previous=d.ftp.revision;ftp_poll(app_ftp,&d.ftp);
            if(d.ftp.revision!=previous)xsdk_log_write("ftp: state=%u enabled=%u devices=%u port=%u error=%d save_error=%u",d.ftp.state,d.ftp.enabled,d.ftp.devices,d.ftp.port,d.ftp.error,d.ftp.save_error);
        }
        else if(!d.startup_active)d.ftp=(FtpSnapshot){.state=FTP_ERROR};
        if(d.ftp_edit&&app_ftp&&!ftp_key_pending){
            ftp_field=d.ftp_edit;memset(ftp_text,0,sizeof ftp_text);
            keyboard_text(ftp_default,FTP_LOGIN_BYTES,ftp_field==1?d.ftp.username:d.ftp.password);
            keyboard_text(ftp_title,40,ftp_field==1?"FTP Username":"FTP Password");keyboard_text(ftp_description,96,"Use 1 to 63 printable characters.");
            ftp_key_result=xsdk_xam_keyboard(controller>=0?(unsigned)controller:0,ftp_title,ftp_description,ftp_default,ftp_text,FTP_LOGIN_BYTES,&ftp_keyboard);
            if(!ftp_key_result||ftp_key_result==997){ftp_key_pending=true;guide_pending=true;guide_seen=false;guide_started=microseconds();ui=true;in=(Input){0};}
            else {d.dialog=true;d.message="The Xbox keyboard could not be opened.\nPress B to return.";}
        }
        if(was_achievements&&!d.achievement_page)achievements_cancel(achievements);
        if(d.achievement_command.type==ACH_GUIDE_GAME||d.achievement_command.type==ACH_GUIDE_DETAIL){
            unsigned error=xbox_achievements_show(app_achievements,&d.achievement_command);in=(Input){0};
            xsdk_log_write("achievements: Guide request type=%u title=%08X achievement=%u result=%08X",d.achievement_command.type,d.achievement_command.title_id,d.achievement_command.achievement_id,error);
            if(!error||error==997){guide_pending=true;guide_seen=false;guide_started=microseconds();ui=true;}
            else {snprintf(profile_error,sizeof profile_error,"The Xbox Guide could not open the achievement view.\nCode %u. Press B to return.",error);d.dialog=true;d.message=profile_error;}
        }else if(d.achievement_command.type){
            bool accepted=achievements_request(achievements,d.achievement_command);
            xsdk_log_write("achievements: command=%u accepted=%u",d.achievement_command.type,accepted);
            dashboard_set_achievements(&d,achievements_poll(achievements));
        }
        if(d.profile_command.type){
            unsigned error=xbox_profile_show(app_profile,d.profile_command);in=(Input){0};
            if(!error||error==997){guide_pending=true;guide_seen=false;guide_started=microseconds();ui=true;}
            else {d.achievement_signin_pending=false;snprintf(profile_error,sizeof profile_error,"The Xbox Guide could not be opened (code %u).\nPress B to return.",error);d.dialog=true;d.message=profile_error;}
            xsdk_log_write("profile: Guide command=%u slot=%u result=%08X",d.profile_command.type,d.profile_command.user,error);
        }
        if(a&(BACK|LEFT|RIGHT|EXIT))library_cancel_launch(library);
        if(d.library_command.type) {
            bool accepted=library_request(library,&d.library_command);
            xsdk_log_write("library: command=%u accepted=%u category=%u path=%s",d.library_command.type,accepted,d.library_command.category,d.library_command.path);
            if(!accepted)d.library_wait=false;
        }
        char library_launch[LIBRARY_PATH];
        LibraryCategory launch_category=LIB_ALL;LibraryKind launch_kind=LIB_XEX;
        if(!ui&&!d.startup_active&&!d.exit_menu&&!d.dialog&&!d.achievement_page&&library_take_launch_ex(library,library_launch,&launch_category,&launch_kind)) {
            bool is_video=launch_kind==LIB_MEDIA&&launch_category==LIB_VIDEO;
            if(is_video) {
                if(!dashboard_video_open(&d,app_video,library_launch)){d.dialog=true;d.message="The embedded video player is unavailable.\nPress B to return.";}
            } else {
                xbox_library_handoff(app_library,true);
                (void)xsdk_launch(library_launch,NULL,0);
                xsdk_log_write("library: launch preflight failed errno=%d",errno);
                xbox_library_handoff(app_library,false);d.dialog=true;
                d.message="The title could not be launched.\nCheck that its storage device is connected, then rescan.";
            }
        }
        if(d.disc_command) {
            bool accepted=xbox_disc_request(app_disc,d.disc_command,d.disc.generation);
            xsdk_log_write("disc: input command=%u generation=%u accepted=%u",d.disc_command,d.disc.generation,accepted);
            xbox_disc_status(app_disc,&d.disc);
        }
        DiscMedia launch_media=DISC_MEDIA_GAME;
        if(!ui&&!d.dialog&&!d.exit_menu&&xbox_disc_take_launch(app_disc,&launch_media)) {
            uint8_t media_data[1020];bool system_media=disc_dashboard_data(launch_media,(unsigned)controller,media_data);
            const char *path=disc_launch_path(launch_media);
            xsdk_log_write("disc: manual launch title=%08X media=%u path=%s",d.disc.title_id,launch_media,path);
            (void)xsdk_launch(path,system_media?media_data:NULL,system_media?sizeof media_data:0);
            int launch_error=errno;
            xsdk_log_write("disc: launch preflight failed path=%s media=%u errno=%d (%s)",path,launch_media,launch_error,strerror(launch_error));
            xbox_disc_launch_failed(app_disc);d.dialog=true;
            snprintf(disc_launch_error,sizeof disc_launch_error,"%s\nError %d: %s\nPress B to return, or Y to eject afterwards.",
                system_media?"The Xbox system media player could not be opened.":"The disc could not be launched.",launch_error,strerror(launch_error));
            d.message=disc_launch_error;
        }
        if(was_starting&&!d.startup_active)xsdk_log_write("startup: complete; main Games menu ready");
        if(audio)ui_audio_play(d.sound);
        if(a)xsdk_log_write("input: %02X blade %d depth %d",a,d.blade,d.depth);
        uint64_t render_start=microseconds();
        uint32_t address;status=xsdk_present_begin(&address,5000);
        if(!NT_SUCCESS(status)) {xsdk_log_write("present_begin: %08X",(unsigned)status);result=1;break;}
        wait_us+=microseconds()-render_start;
        XboxRenderStats stats;
        if(!xbox_renderer_frame(renderer,&d,address,&stats)){result=1;break;}
        totals.cpu_us+=stats.cpu_us;totals.upload_us+=stats.upload_us;totals.gpu_wait_us+=stats.gpu_wait_us;
        totals.gpu_us+=stats.gpu_us;totals.builds+=stats.builds;totals.draws+=stats.draws;totals.bg_updates+=stats.bg_updates;
        xsdk_present_end();
        uint64_t submitted=microseconds();measured_frames++;
        if(submitted-window_start>=5000000) {
            XsdkPresentStats present;xsdk_present_stats(&present);
            unsigned fps10=(unsigned)((uint64_t)measured_frames*10000000/(submitted-window_start));
            xsdk_log_write("perf: fps=%u.%u cpu_us=%u gpu_us=%u texture_upload_us=%u gpu_wait_us=%u present_wait_us=%u missed=%u late=%u list_builds=%u draws=%u bg_updates=%u resident_kb=%u",
                fps10/10,fps10%10,(unsigned)(totals.cpu_us/measured_frames),(unsigned)(totals.gpu_us/measured_frames),
                (unsigned)(totals.upload_us/measured_frames),(unsigned)(totals.gpu_wait_us/measured_frames),(unsigned)(wait_us/measured_frames),
                present.missed-previous_present.missed,present.late-previous_present.late,
                totals.builds,totals.draws/measured_frames,totals.bg_updates,stats.resident_kb);
            window_start=submitted;measured_frames=0;wait_us=0;totals=(XboxRenderStats){0};
            previous_present=present;
        }
        if(!frame)xsdk_log_write("frame: first dashboard submitted (console visual check required)");
    }
    close_app();
    xsdk_present_close();status=xsdk_gpu_close();
    if(!NT_SUCCESS(status)) {xsdk_log_write("gpu_close: %08X",(unsigned)status);result=1;}
    xsdk_log_write("exit: return to configured system dashboard; Aurora path not assumed");
    return result;
}
