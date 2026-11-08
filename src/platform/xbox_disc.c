#include "xbox_disc.h"
#include "disc_metadata.h"
#include "xsdk/thread.h"
#include "xsdk/log.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {
    uint32_t (*tray)(void);
    uint32_t (*media)(void *,void *);
    void (*smc)(const void *,void *);
    void (*aes_key)(void *,const uint8_t *);
    void (*aes_cbc)(void *,const uint8_t *,uint32_t,uint8_t *,uint8_t *,uint32_t);
    uint32_t (*ldi_create)(uint32_t *,void *,uint32_t,uint32_t,void *,uint32_t *,uint32_t *);
    uint32_t (*ldi_expand)(uint32_t,const uint8_t *,uint16_t,uint8_t *,uint32_t *);
    uint32_t (*ldi_destroy)(uint32_t);
} DiscApi;
struct XboxDisc {
    pthread_mutex_t lock;HANDLE poll,metadata;DiscApi api;DiscDrive drive;
    bool stop,launch_pending,launch_ready;DiscCommand command;uint32_t launch_generation;
};
static void *resolve(const char *name,uint32_t ordinal) {
    HANDLE module;void *p=NULL;
    if(XexGetModuleHandle(name,&module)!=0||!NT_SUCCESS(XexGetProcedureAddress(module,ordinal,&p)))return NULL;
    return p;
}
typedef struct {
    DiscApi *api;_Alignas(16) uint8_t aes[352],iv[16];void *work;uint32_t ldi;bool compressed;
} Codec;
static void finish(void *context) {
    Codec *c=context;if(c->compressed)(void)c->api->ldi_destroy(c->ldi);
    free(c->work);c->work=NULL;c->compressed=false;
    memset(c->aes,0,sizeof c->aes);memset(c->iv,0,sizeof c->iv);
}
static bool start(void *context,const uint8_t key[16],bool encrypted,uint32_t window) {
    Codec *c=context;DiscApi *a=c->api;
    if(encrypted) {
        if(!a->aes_key||!a->aes_cbc)return false;
        static const uint8_t retail[16]={0x20,0xb1,0x85,0xa5,0x9d,0x28,0xfd,0xc3,0x40,0x58,0x3f,0xbb,0x08,0x96,0xbf,0x91};
        uint8_t session[16],iv[16]={0};
        a->aes_key(c->aes,retail);a->aes_cbc(c->aes,key,16,session,iv,0);a->aes_key(c->aes,session);
        memset(session,0,sizeof session);memset(c->iv,0,sizeof c->iv);
    }
    if(window) {
        if(!a->ldi_create||!a->ldi_expand||!a->ldi_destroy)return false;
        c->work=malloc(0x23200+window);if(!c->work)return false;
        uint32_t maximum=32768,minimum=0,configuration[2]={window,1};
        if(a->ldi_create(&maximum,configuration,0,0,c->work,&minimum,&c->ldi)!=0){finish(c);return false;}
        c->compressed=true;
    }
    return true;
}
static bool decrypt(void *context,uint8_t *bytes,size_t size) {
    Codec *c=context;if(size&15)return false;
    c->api->aes_cbc(c->aes,bytes,(uint32_t)size,bytes,c->iv,0);return true;
}
static bool expand(void *context,const uint8_t *in,size_t size,uint8_t *out,size_t *count) {
    Codec *c=context;uint32_t n=(uint32_t)*count;
    if(size>65535||c->api->ldi_expand(c->ldi,in,(uint16_t)size,out,&n)!=0)return false;
    *count=n;return true;
}
bool xbox_disc_xex_title(FILE *file,uint32_t *id,char title[DISC_TITLE_BYTES]) {
    /* The codec state is per call: optical metadata and library scans can run
       concurrently without sharing CBC or LZX state. */
    DiscApi api={0};
    api.aes_key=resolve("xboxkrnl.exe",0x159);api.aes_cbc=resolve("xboxkrnl.exe",0x15b);
    api.ldi_create=resolve("xboxkrnl.exe",0xb6);api.ldi_expand=resolve("xboxkrnl.exe",0xb7);api.ldi_destroy=resolve("xboxkrnl.exe",0xb8);
    Codec codec={.api=&api};DiscCodec services={&codec,start,decrypt,expand,finish};
    return disc_xex_title(file,&services,id,title);
}
static double seconds(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec+(double)t.tv_nsec/1e9;
}
static bool stopped(XboxDisc *d) {pthread_mutex_lock(&d->lock);bool stop=d->stop;pthread_mutex_unlock(&d->lock);return stop;}
static uint32_t poll_drive(void *context) {
    XboxDisc *d=context;double last=seconds();DiscStatus previous={.state=DISC_UNAVAILABLE};
    while(!stopped(d)) {
        pthread_mutex_lock(&d->lock);DiscCommand command=d->command;d->command=DISC_NONE;pthread_mutex_unlock(&d->lock);
        if(command==DISC_EJECT||command==DISC_CLOSE) {
            _Alignas(16) uint8_t message[16]={0x8b,command==DISC_EJECT?0x60:0x62};
            d->api.smc(message,NULL);xsdk_log_write("disc: tray command %s",command==DISC_EJECT?"open":"close");
        }
        uint32_t tray=d->api.tray();double now=seconds();
        pthread_mutex_lock(&d->lock);
        disc_observe(&d->drive,(DiscTray)tray,now-last);last=now;
        if(d->launch_generation!=d->drive.status.generation){d->launch_ready=false;d->launch_pending=false;}
        DiscStatus status=d->drive.status;pthread_mutex_unlock(&d->lock);
        if(status.state!=previous.state||status.generation!=previous.generation||status.title_id!=previous.title_id||status.media!=previous.media) {
            char label[DISC_TITLE_BYTES+16];disc_label(&status,label,sizeof label);
            xsdk_log_write("disc: tray=%u state=%u generation=%u title=%08X %s",tray,status.state,status.generation,status.title_id,label);
            previous=status;
        }
        (void)xsdk_thread_sleep(100);
    }
    return 0;
}
static bool metadata(XboxDisc *d,uint32_t *id,char *title,bool title_needed) {
    title[0]=0;
    if(title_needed) {
        FILE *art=fopen("/Device/CdRom0/nxeart","rb");
        if(art){(void)disc_stfs_title(art,title);fclose(art);}
    }
    FILE *xex=fopen(DISC_NATIVE_XEX,"rb");if(!xex)return false;
    Codec codec={.api=&d->api};DiscCodec services={&codec,start,decrypt,expand,finish};
    bool ok=disc_xex_title(xex,&services,id,title_needed&&!title[0]?title:NULL);
    fclose(xex);return ok;
}
static uint32_t read_metadata(void *context) {
    XboxDisc *d=context;uint32_t attempted=UINT32_MAX,last_result=UINT32_MAX,last_type=UINT32_MAX;double retry=0;
    while(!stopped(d)) {
        pthread_mutex_lock(&d->lock);DiscStatus status=d->drive.status;
        bool launch=d->launch_pending&&d->launch_generation==status.generation;
        pthread_mutex_unlock(&d->lock);
        if(launch||((status.state==DISC_EMPTY||status.state==DISC_READING||status.state==DISC_ERROR||status.state==DISC_UNSUPPORTED)&&
           (attempted!=status.generation||seconds()>=retry))) {
            /* These outputs are structures (12/20 bytes in public FreeStyle
               code), not two DWORDs. Reserve aligned space for both. */
            _Alignas(16) uint32_t media[16]={UINT32_MAX},info[16]={0};
            uint32_t result=d->api.media(media,info),id=0,type=media[0];char title[DISC_TITLE_BYTES]={0};
            uint32_t before=d->api.tray();
            pthread_mutex_lock(&d->lock);
            disc_observe(&d->drive,(DiscTray)before,0);
            DiscMedia kind=launch?disc_media_kind(result,type):disc_media_update(&d->drive,status.generation,result,type);
            bool current=status.generation==d->drive.status.generation&&!d->drive.pending;
            pthread_mutex_unlock(&d->lock);
            bool game=current&&kind==DISC_MEDIA_GAME&&metadata(d,&id,title,!launch);
            uint32_t fresh=d->api.tray(); /* Discard a read completed after physical eject. */
            pthread_mutex_lock(&d->lock);
            disc_observe(&d->drive,(DiscTray)fresh,0);
            if(launch) {
                bool media_ready=current&&(kind==DISC_MEDIA_AUDIO_CD||kind==DISC_MEDIA_DVD_VIDEO||kind==DISC_MEDIA_DATA)&&kind==status.media;
                d->launch_ready=((game&&id==status.title_id)||media_ready)&&d->drive.status.state==DISC_READY&&
                    status.generation==d->drive.status.generation;
                d->launch_pending=false;
                if(!d->launch_ready&&d->drive.status.state==DISC_READY) {
                    d->drive.status.state=DISC_READING;d->drive.status.title[0]=0;d->drive.status.title_id=0;d->drive.status.media=DISC_MEDIA_PENDING;d->drive.status.generation++;
                }
            } else if(game)disc_identified(&d->drive,status.generation,id,title,true);
            float reading_seconds=d->drive.reading_seconds;unsigned samples=d->drive.media_samples;bool playable=d->drive.status.state==DISC_READY;
            pthread_mutex_unlock(&d->lock);
            attempted=status.generation;retry=seconds()+1;
            if(result!=last_result||type!=last_type||game||launch)
                xsdk_log_write("disc: media return=%08X raw=%08X kind=%u title=%08X playable=%u launch_check=%u reading_ms=%u samples=%u",result,type,kind,id,playable,launch,(unsigned)(reading_seconds*1000),samples);
            last_result=result;last_type=type;
        }
        (void)xsdk_thread_sleep(100);
    }
    return 0;
}
XboxDisc *xbox_disc_open(void) {
    XboxDisc *d=calloc(1,sizeof *d);if(!d)return NULL;
    if(pthread_mutex_init(&d->lock,NULL)!=0){free(d);return NULL;}
    disc_init(&d->drive);
#define API(field,module,ordinal) d->api.field=resolve(module,ordinal)
    API(tray,"xam.xex",0x1aa);API(media,"xam.xex",0x1a3);API(smc,"xboxkrnl.exe",0x29);
    API(aes_key,"xboxkrnl.exe",0x159);API(aes_cbc,"xboxkrnl.exe",0x15b);
    API(ldi_create,"xboxkrnl.exe",0xb6);API(ldi_expand,"xboxkrnl.exe",0xb7);API(ldi_destroy,"xboxkrnl.exe",0xb8);
#undef API
    if(!d->api.tray||!d->api.media||!d->api.smc) {
        xsdk_log_write("disc: required exports unavailable; drive disabled");xbox_disc_close(d);return NULL;
    }
    if(!NT_SUCCESS(xsdk_thread_create_on(&d->poll,64*1024,poll_drive,d,2))||
       !NT_SUCCESS(xsdk_thread_create_on(&d->metadata,128*1024,read_metadata,d,4))) {xbox_disc_close(d);return NULL;}
    xsdk_log_write("disc: monitoring tray at 10 Hz; metadata worker; autoplay disabled");return d;
}
void xbox_disc_status(XboxDisc *d,DiscStatus *s) {
    if(!d){*s=(DiscStatus){.state=DISC_UNAVAILABLE};return;}
    pthread_mutex_lock(&d->lock);*s=d->drive.status;pthread_mutex_unlock(&d->lock);
}
bool xbox_disc_request(XboxDisc *d,DiscCommand command,uint32_t generation) {
    if(!d)return false;
    pthread_mutex_lock(&d->lock);
    bool accepted=!d->command&&(command==DISC_EJECT||(!d->launch_pending&&!d->launch_ready))&&disc_request(&d->drive,command,generation);
    if(accepted) {
        if(command==DISC_LAUNCH){d->launch_pending=true;d->launch_generation=generation;}
        else {d->command=command;d->launch_ready=false;d->launch_pending=false;}
    }
    pthread_mutex_unlock(&d->lock);return accepted;
}
bool xbox_disc_take_launch(XboxDisc *d,DiscMedia *media) {
    if(!d)return false;
    pthread_mutex_lock(&d->lock);bool waiting=d->launch_ready;pthread_mutex_unlock(&d->lock);
    if(!waiting)return false;
    uint32_t tray=d->api.tray();
    pthread_mutex_lock(&d->lock);
    disc_observe(&d->drive,(DiscTray)tray,0);
    bool ready=d->launch_ready&&(tray==TRAY_CLOSED||tray==TRAY_EMPTY)&&d->drive.status.state==DISC_READY&&
        d->launch_generation==d->drive.status.generation;
    if(ready&&media)*media=d->drive.status.media;
    d->launch_ready=false;pthread_mutex_unlock(&d->lock);return ready;
}
void xbox_disc_launch_failed(XboxDisc *d) {
    if(!d)return;
    pthread_mutex_lock(&d->lock);d->launch_ready=false;d->launch_pending=false;
    DiscMedia media=d->drive.status.media;
    if(media!=DISC_MEDIA_AUDIO_CD&&media!=DISC_MEDIA_DVD_VIDEO&&media!=DISC_MEDIA_DATA)d->drive.status.state=DISC_ERROR;
    pthread_mutex_unlock(&d->lock);
}
void xbox_disc_close(XboxDisc *d) {
    if(!d)return;
    pthread_mutex_lock(&d->lock);d->stop=true;pthread_mutex_unlock(&d->lock);
    if(d->poll){(void)NtWaitForSingleObjectEx(d->poll,USER_MODE,0,NULL);(void)NtClose(d->poll);}
    if(d->metadata){(void)NtWaitForSingleObjectEx(d->metadata,USER_MODE,0,NULL);(void)NtClose(d->metadata);}
    pthread_mutex_destroy(&d->lock);free(d);
}
