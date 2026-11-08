#include "xbox_profile.h"
#include "xsdk/thread.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct XboxProfile {
    ProfileService *service;
    uint32_t (*signin_ui)(uint32_t,uint32_t,uint32_t);
    uint32_t (*gamer_ui)(uint32_t);
    uint32_t (*tile)(uint32_t,uint32_t,uint64_t,uint32_t,uint8_t *,uint32_t,uint32_t,XAM_OVERLAPPED *);
};
static void *resolve(unsigned ordinal) {HANDLE module;void *p=NULL;if(XexGetModuleHandle("xam.xex",&module)==0&&NT_SUCCESS(XexGetProcedureAddress(module,ordinal,&p)))return p;return NULL;}
static bool identity(void *context,unsigned user,ProfileIdentity *out) {
    (void)context;*out=(ProfileIdentity){.user=user,.state=PROFILE_SIGNED_OUT};
    unsigned state=XamUserGetSigninState(user);if(!state)return true;if(state>2)return false;
    USER_SIGNIN_INFO info={0};if(XamUserGetSigninInfo(user,0,&info))return false;
    if(!info.signin_state)return true;
    if(info.signin_state>2)return false;
    uint64_t xuid=0;if(XamUserGetXUID(user,7,&xuid)||!xuid)return false;
    out->state=info.signin_state==2?PROFILE_LIVE:PROFILE_LOCAL;out->xuid=xuid;out->guest=(info.unknown_08&2)!=0;
    info.name[15]=0;snprintf(out->name,sizeof out->name,"%s",info.name[0]?info.name:"Signed In");return true;
}
static bool settings(unsigned user,unsigned count,const uint32_t *ids,ProfileSnapshot *out,char key[64]) {
    uint32_t bytes=0;unsigned result=XamUserReadProfileSettings(0,user,0,NULL,count,ids,&bytes,NULL,NULL);
    if((result&&result!=122)||bytes<8||bytes>8192)return false;
    uint32_t capacity=bytes;uint8_t *data=calloc(1,bytes);if(!data)return false;
    result=XamUserReadProfileSettings(0,user,0,NULL,count,ids,&bytes,data,NULL);
    bool ok=!result&&bytes<=capacity&&profile_settings(data,bytes,(uint32_t)(uintptr_t)data,out,key);free(data);return ok;
}
static void details(void *context,ProfileSnapshot *out) {
    XboxProfile *p=context;const uint32_t ids[]={0x10040012,0x10040006,0x10040013,0x4064000f};char key[64]={0};
    if(!settings(out->identity.user,4,ids,out,key)) {
        out->known=0;key[0]=0;
        for(unsigned i=0;i<4;i++) {
            ProfileSnapshot value={.identity=out->identity};char one_key[64]={0};
            if(!settings(out->identity.user,1,ids+i,&value,one_key))continue;
            if(value.known&PROFILE_GAMES)out->games=value.games;
            if(value.known&PROFILE_SCORE)out->score=value.score;
            if(value.known&PROFILE_ACHIEVEMENTS)out->achievements=value.achievements;
            out->known|=value.known;if(one_key[0])snprintf(key,sizeof key,"%s",one_key);
        }
    }
    /* Local/current profile tiles do not depend on a parseable picture key. */
    uint8_t *pixels=calloc(1,PROFILE_PICTURE_BYTES*2);if(!pixels)return;
    const unsigned types[]={4,2,9};
    for(unsigned i=0;p->tile&&i<3;i++){
        uint32_t title=0;uint64_t image=0;
        if(types[i]==9&&!profile_picture_key(key,&title,&image))continue;
        memset(pixels,0,PROFILE_PICTURE_BYTES);
        unsigned result=p->tile(types[i],title,image,out->identity.user,pixels,256,64,NULL);
        if(!result&&profile_picture_argb(pixels,PROFILE_PICTURE_BYTES,256,pixels+PROFILE_PICTURE_BYTES)){memcpy(out->picture,pixels+PROFILE_PICTURE_BYTES,PROFILE_PICTURE_BYTES);out->has_picture=true;break;}
        xsdk_log_write("profile: picture type=%u result=%u unavailable",types[i],result);
    }
    free(pixels);
}
static bool available_profiles(void *context,unsigned *total){
    (void)context;
    static const char *const devices[]={"hdd:/","usb0:/","usb1:/","usb2:/","mu0:/","mu1:/","bladesmu:/",
        "/Device/Mass0PartitionFile/Storage/","/Device/Mass1PartitionFile/Storage/","/Device/Mass2PartitionFile/Storage/"};
    bool ok=profile_count_packages(devices,sizeof devices/sizeof devices[0],total);
    xsdk_log_write("profile: package metadata count_known=%u count=%u",ok,*total);return ok;
}
static void worker_start(void *context) {(void)context;(void)xsdk_thread_move(5);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-1,NULL);}
XboxProfile *xbox_profile_open(void) {
    XboxProfile *p=calloc(1,sizeof *p);if(!p)return NULL;p->signin_ui=resolve(702);p->gamer_ui=resolve(708);p->tile=resolve(757);
    ProfileHooks hooks={.context=p,.identity=identity,.details=details,.profiles=available_profiles,.worker_start=worker_start};
    p->service=profile_open(&hooks);if(!p->service){free(p);return NULL;}
    xsdk_log_write("profile: Guide sign-in; read-only card worker; signout_api=%u picture_api=%u",p->signin_ui!=NULL,p->tile!=NULL);return p;
}
ProfileService *xbox_profile_service(XboxProfile *p){return p?p->service:NULL;}
unsigned xbox_profile_signin(void){(void)xsdk_system_ui_open();return XamShowSigninUI(1,0);}
unsigned xbox_profile_show(XboxProfile *p,ProfileCommand command){
    if(!p)return 50;
    if(xsdk_system_ui_open())return 170;
    if(command.type==PROFILE_SIGN_IN)return XamShowSigninUI(1,0);
    ProfileIdentity current;
    if(command.user>=4||!command.xuid||!identity(p,command.user,&current)||current.xuid!=command.xuid)return 1317;
    if(command.type==PROFILE_SIGN_OUT)return p->signin_ui?p->signin_ui(command.user,1,4):50;
    if(command.type==PROFILE_VIEW)return p->gamer_ui?p->gamer_ui(command.user):50;
    return 87;
}
void xbox_profile_close(XboxProfile *p){if(p){profile_close(p->service);free(p);}}
