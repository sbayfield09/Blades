#include "xbox_profile.h"
#include "xsdk/kernel.h"
#include "xsdk/thread.h"
#include "xsdk/xam.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static atomic_uint mode,reads;static unsigned signins,signouts,views;static bool ui;
static void put(unsigned char *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
uint32_t XamUserGetSigninState(uint32_t user){return user==2?1:0;}
uint32_t XamUserGetSigninInfo(uint32_t user,uint32_t flags,USER_SIGNIN_INFO *out){assert(user==2&&!flags);memset(out,0,sizeof *out);out->signin_state=1;strcpy(out->name,"Real Fixture");return 0;}
uint32_t XamUserGetXUID(uint32_t user,uint32_t mask,uint64_t *uid){assert(user==2&&mask==7);*uid=20;return 0;}
uint32_t XamUserReadProfileSettings(uint32_t title,uint32_t user,uint32_t xuids,const uint64_t *ids,uint32_t count,const uint32_t *settings,uint32_t *bytes,void *buffer,XAM_OVERLAPPED *overlap){
    assert(!title&&user==2&&!xuids&&!ids&&count==4&&!overlap);if(!buffer){*bytes=168;return 122;}assert(*bytes>=168);
    unsigned char *p=buffer;memset(p,0,168);put(p,4);put(p+4,(uint32_t)(uintptr_t)p+8);
    for(unsigned i=0;i<3;i++){unsigned char *r=p+8+i*40;put(r,2);put(r+8,2);put(r+16,settings[i]);r[24]=1;put(r+32,i==0?5:i==1?50:5);}return 0;
}
static uint32_t tile(uint32_t type,uint32_t title,uint64_t image,uint32_t user,uint8_t *data,uint32_t pitch,uint32_t height,XAM_OVERLAPPED *overlap){
    assert((type==4||type==2)&&!title&&!image&&user==2&&pitch==256&&height==64&&!overlap);atomic_fetch_add(&reads,1);
    if(atomic_load(&mode)==2||(atomic_load(&mode)==1&&type==4))return 2;
    for(unsigned i=0;i<PROFILE_PICTURE_BYTES;i+=4){data[i]=255;data[i+1]=12;data[i+2]=34;data[i+3]=56;}return 0;
}
static uint32_t signin_ui(uint32_t user,uint32_t panes,uint32_t flags){assert(user==2&&panes==1&&flags==4);signouts++;return 0;}
static uint32_t gamer_ui(uint32_t user){assert(user==2);views++;return 997;}
uint32_t XamShowSigninUI(uint32_t panes,uint32_t flags){assert(panes==1&&!flags);signins++;return 0;}
bool xsdk_system_ui_open(void){return ui;}
uint32_t XexGetModuleHandle(const char *name,HANDLE *out){assert(!strcmp(name,"xam.xex"));*out=1;return 0;}
NTSTATUS XexGetProcedureAddress(HANDLE h,uint32_t ordinal,void **out){assert(h==1);*out=ordinal==757?(void *)tile:ordinal==702?(void *)signin_ui:ordinal==708?(void *)gamer_ui:NULL;return *out?0:(NTSTATUS)0xc0000001;}
NTSTATUS xsdk_thread_move(uint32_t core){assert(core==5);return 0;}
NTSTATUS xsdk_thread_set_priority(HANDLE h,int32_t priority,int32_t *previous){assert(h==XSDK_CURRENT_THREAD&&priority==-1&&!previous);return 0;}
void xsdk_log_write(const char *format,...){(void)format;}
static void pause_poll(void){struct timespec t={0,10000000};nanosleep(&t,NULL);}
int main(void){
    XboxProfile *app=xbox_profile_open();assert(app);ProfileService *s=xbox_profile_service(app);profile_set_user(s,2);
    const ProfileSnapshot *p=NULL;for(unsigned i=0;i<300;i++){p=profile_poll(s);if(p->known==7&&p->has_picture)break;pause_poll();}
    assert(p&&p->known==7&&p->has_picture&&p->picture[0]==12&&p->picture[1]==34&&p->picture[2]==56&&p->picture[3]==255);
    atomic_store(&mode,2);unsigned before=atomic_load(&reads);profile_refresh(s);for(unsigned i=0;i<300&&atomic_load(&reads)<before+2;i++)pause_poll();
    p=profile_poll(s);assert(p->has_picture&&p->picture[0]==12);
    assert(!xbox_profile_show(app,(ProfileCommand){.type=PROFILE_SIGN_IN})&&signins==1);
    ProfileCommand c={.type=PROFILE_SIGN_OUT,.user=2,.xuid=20};assert(!xbox_profile_show(app,c)&&signouts==1);
    c.type=PROFILE_VIEW;assert(xbox_profile_show(app,c)==997&&views==1);c.xuid=21;assert(xbox_profile_show(app,c)==1317&&views==1);
    c.xuid=20;ui=true;assert(xbox_profile_show(app,c)==170&&views==1);ui=false;
    xbox_profile_close(app);puts("PASS: actual native profile tile/current user/stat parameters, transient-picture retention, Guide sign-out flags/profile target and stale/busy protection");
}
