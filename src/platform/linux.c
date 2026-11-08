#include "blades.h"
#include "resources.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SDL_AudioStream *audio;
static LibraryService *library;
static AchievementService *achievements;
static VideoPlayer *video;
static JellyService *jelly;
static Updater *updates;
static bool prepare_media(void *context,const LibraryEntry *entry,char *path,char *error,size_t bytes) {
    (void)context;
    if(entry->kind==LIB_MEDIA&&entry->category==LIB_VIDEO){snprintf(path,LIBRARY_PATH,"%s",entry->path);return true;}
    snprintf(error,bytes,"This item requires the Xbox 360.");return false;
}
static void audio_callback(void *context,SDL_AudioStream *stream,int additional,int total) {
    (void)context;(void)total;
    float samples[512];
    while(additional>0) {
        int frames=(additional+7)/8;if(frames>256)frames=256;
        VideoPlayer *player=__atomic_load_n(&video,__ATOMIC_ACQUIRE);
        if(!player||!video_audio_render(player,samples,(unsigned)frames))ui_audio_render(samples,(size_t)frames);
        if(!SDL_PutAudioStreamData(stream,samples,frames*8))return;
        additional-=frames*8;
    }
}
static unsigned poll_pad(SDL_Gamepad *p) {
    unsigned a=0;
    if(!p)return 0;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_DPAD_UP)||SDL_GetGamepadAxis(p,SDL_GAMEPAD_AXIS_LEFTY)<-16000)a|=UP;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_DPAD_DOWN)||SDL_GetGamepadAxis(p,SDL_GAMEPAD_AXIS_LEFTY)>16000)a|=DOWN;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_DPAD_LEFT)||SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)||SDL_GetGamepadAxis(p,SDL_GAMEPAD_AXIS_LEFTX)<-16000)a|=LEFT;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_DPAD_RIGHT)||SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)||SDL_GetGamepadAxis(p,SDL_GAMEPAD_AXIS_LEFTX)>16000)a|=RIGHT;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_SOUTH))a|=SELECT;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_EAST))a|=BACK;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_WEST))a|=PROFILE;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_NORTH))a|=EJECT;
    if(SDL_GetGamepadButton(p,SDL_GAMEPAD_BUTTON_BACK))a|=EXIT;
    return a;
}
static int snapshot(Canvas *c,const char *path) {
    FILE *f=fopen(path,"wb");if(!f)return 1;
    fprintf(f,"P6\n%d %d\n255\n",WIDTH,HEIGHT);
    for(int i=0;i<WIDTH*HEIGHT;i++) {
        unsigned char rgb[3]={(unsigned char)(c->pixels[i]>>16),(unsigned char)(c->pixels[i]>>8),(unsigned char)c->pixels[i]};
        if(fwrite(rgb,1,3,f)!=3){fclose(f);return 1;}
    }
    return fclose(f)!=0;
}
int main(int argc,char **argv) {
    Dashboard d;dashboard_init(&d); Input input={0};
    Canvas c={.pixels=calloc(WIDTH*HEIGHT,4)}; if(!c.pixels)return 1;
    const char *resource_path=getenv("BLADES_RESOURCE_PACK");
    if(resource_path)resources_open(resource_path);
    else if(!resources_open("Assets/blades.xzp"))resources_open("Assets/shrdres.xzp");
    if(argc>=3 && !strcmp(argv[1],"--snapshot")) {
        if(argc>3) {
            char *end=NULL;long blade=strtol(argv[3],&end,10);
            if(!*argv[3]||*end||blade<0||blade>=BLADE_COUNT) {
                fprintf(stderr,"blade index must be 0 through %d\n",BLADE_COUNT-1);free(c.pixels);return 1;
            }
            d.blade=(int)blade;d.position=(float)blade;d.target=(float)blade;
        }
        dashboard_render(&c,&d);int result=snapshot(&c,argv[2]);free(c.pixels);return result;
    }
    dashboard_begin_startup(&d);
    const char *config=getenv("BLADES_LIBRARY_CONFIG");
    LibraryHooks hooks={.prepare=prepare_media};
    library=library_open(config?config:"Config/library-paths.txt",NULL,&hooks);
    const char *jelly_config=getenv("BLADES_JELLYFIN_CONFIG");jelly=jellyfin_open(jelly_config?jelly_config:"Config/jellyfin-settings.json",NULL);
    updates=updater_open("Updates",NULL,NULL);
    achievements=achievements_open(NULL);
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {fprintf(stderr,"SDL: %s\n",SDL_GetError());free(c.pixels);return 1;}
    SDL_Window *window=NULL;SDL_Renderer *renderer=NULL;
    if(!SDL_CreateWindowAndRenderer("Blades | Desktop preview",WIDTH,HEIGHT,SDL_WINDOW_RESIZABLE,&window,&renderer))goto fail;
    SDL_SetRenderLogicalPresentation(renderer,WIDTH,HEIGHT,SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_Texture *texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,WIDTH,HEIGHT);
    if(!texture)goto fail;
    ui_audio_init();
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_AudioSpec spec={.format=SDL_AUDIO_F32,.channels=2,.freq=48000};
        audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,audio_callback,NULL);
        if(audio)SDL_ResumeAudioStreamDevice(audio);
    }
    if(!audio)fprintf(stderr,"audio unavailable; continuing silently: %s\n",SDL_GetError());
    __atomic_store_n(&video,video_create(&(VideoConfig){.channels=2,.audio_available=audio!=NULL}),__ATOMIC_RELEASE);
    SDL_Gamepad *pad=NULL; bool done=false; int frames=0;
    int limit=argc==3&&!strcmp(argv[1],"--frames")?atoi(argv[2]):0;
    Uint64 last=SDL_GetTicksNS();
    while(!done&&!d.exit_requested) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_EVENT_QUIT)done=true;
            if(e.type==SDL_EVENT_GAMEPAD_REMOVED && pad && e.gdevice.which==SDL_GetGamepadID(pad)) {
                SDL_CloseGamepad(pad);pad=NULL;input=(Input){0};
            }
        }
        if(!pad) {int n=0;SDL_JoystickID *ids=SDL_GetGamepads(&n);if(n)pad=SDL_OpenGamepad(ids[0]);SDL_free(ids);}
        const bool *k=SDL_GetKeyboardState(NULL);unsigned a=poll_pad(pad);
        if(k[SDL_SCANCODE_UP]||k[SDL_SCANCODE_W])a|=UP;
        if(k[SDL_SCANCODE_DOWN]||k[SDL_SCANCODE_S])a|=DOWN;
        if(k[SDL_SCANCODE_LEFT]||k[SDL_SCANCODE_Q])a|=LEFT;
        if(k[SDL_SCANCODE_RIGHT]||k[SDL_SCANCODE_E])a|=RIGHT;
        if(k[SDL_SCANCODE_RETURN]||k[SDL_SCANCODE_SPACE])a|=SELECT;
        if(k[SDL_SCANCODE_ESCAPE]||k[SDL_SCANCODE_BACKSPACE])a|=BACK;
        if(k[SDL_SCANCODE_X])a|=PROFILE;
        if(k[SDL_SCANCODE_Y])a|=EJECT;
        if(k[SDL_SCANCODE_F10])a|=EXIT;
        Uint64 now=SDL_GetTicksNS();float dt=(float)(now-last)/1e9f;last=now;
        unsigned events=input_update(&input,a,dt,!d.startup_active);
        dashboard_set_library(&d,library_poll(library));
        dashboard_set_achievements(&d,achievements_poll(achievements));
        bool old_update=dashboard_update_page(&d);updater_poll(updates,&d.update);
        JellyPage old_jelly_page=d.jelly_page;bool was_jelly_video=d.video_view&&d.video.network;
        dashboard_set_jellyfin(&d,jellyfin_poll(jelly));
        if(!dashboard_video_update(&d,video,events,dt,false))dashboard_update(&d,events,dt);
        if(d.video_view&&d.video.network)jellyfin_progress(jelly,&d.video);
        else if(was_jelly_video)jellyfin_finish(jelly,d.video.position);
        if(old_jelly_page!=JELLY_UI_NONE&&d.jelly_page==JELLY_UI_NONE)jellyfin_cancel(jelly);
        dashboard_jellyfin_dispatch(&d,jelly);
        if(d.update_check)(void)updater_check(updates);
        if(d.update_download)(void)updater_download(updates);
        if(d.update_cancel||(old_update&&!dashboard_update_page(&d)))updater_cancel(updates);
        if(d.jelly_edit){d.dialog=true;d.message="Desktop preview: Jellyfin connection editing uses the Xbox keyboard.\nPress B to return.";}
        if(d.jelly_page==JELLY_UI_BROWSE&&!d.video_view&&!d.dialog&&!d.exit_menu){VideoHttpInput stream;if(jellyfin_take_playback(jelly,&stream)){if(!dashboard_video_http_open(&d,video,&stream))jellyfin_finish(jelly,0);xf_wipe(&stream,sizeof stream);}}
        if(d.achievement_command.type==ACH_GUIDE_GAME||d.achievement_command.type==ACH_GUIDE_DETAIL){d.dialog=true;d.message="Desktop preview: achievement selection requests the real Xbox Guide.\nPress B to return.";}
        else if(d.achievement_command.type)achievements_request(achievements,d.achievement_command);
        if(d.ftp_edit){d.dialog=true;d.message="Desktop preview: editing FTP login details uses the Xbox keyboard on console.\nPress B to return.";}
        if(d.profile_command.type){d.achievement_signin_pending=false;d.dialog=true;d.message="Desktop preview: Xbox profile sign-in requires the console Guide.\nPress B to return.";}
        if(d.library_command.type&&!library_request(library,&d.library_command))d.library_wait=false;
        char launch[LIBRARY_PATH];LibraryCategory category;LibraryKind kind;
        if(library_take_launch_ex(library,launch,&category,&kind)&&kind==LIB_MEDIA&&category==LIB_VIDEO) {
            d.library_wait=false;
            if(!dashboard_video_open(&d,video,launch)){d.dialog=true;d.message="The embedded video player is unavailable.";}
        }
        if(audio)ui_audio_play(d.sound);
        dashboard_render(&c,&d);
        if(!SDL_UpdateTexture(texture,NULL,c.pixels,WIDTH*4))goto render_fail;
        SDL_RenderClear(renderer);SDL_RenderTexture(renderer,texture,NULL,NULL);SDL_RenderPresent(renderer);
        if(limit&&++frames>=limit)done=true;
        SDL_Delay(8);
    }
    SDL_CloseGamepad(pad);SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    updater_close(updates);jellyfin_close(jelly);achievements_close(achievements);library_close(library);SDL_DestroyAudioStream(audio);video_close(__atomic_exchange_n(&video,NULL,__ATOMIC_ACQ_REL));ui_audio_close();resources_close();SDL_Quit();free(c.pixels);return 0;
render_fail:
    SDL_CloseGamepad(pad);SDL_DestroyTexture(texture);
fail:
    updater_close(updates);jellyfin_close(jelly);
    achievements_close(achievements);library_close(library);
    SDL_DestroyAudioStream(audio);
    video_close(__atomic_exchange_n(&video,NULL,__ATOMIC_ACQ_REL));
    ui_audio_close();resources_close();
    fprintf(stderr,"SDL: %s\n",SDL_GetError());SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();free(c.pixels);return 1;
}
