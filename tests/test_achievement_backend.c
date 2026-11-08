/* Exercise the console backend's actual enumeration/cleanup code using XAM
   fixtures, not a replacement parser or fake UI data loader. */
#include "xbox_achievements.h"
#include "xsdk/kernel.h"
#include "xsdk/thread.h"
#include "xsdk/xam.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static unsigned mode,open_handles,tiles,title_page,item_page,game_views,detail_views,guide_result;
static bool guide_visible;
static void put(uint8_t *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static void wide(uint8_t *p,const char *text){for(;*text;text++,p+=2){p[0]=0;p[1]=*text;}p[0]=p[1]=0;}
static uint32_t title_enum(uint32_t title,uint32_t user,uint64_t uid,uint32_t first,uint32_t count,uint32_t *bytes,HANDLE *h){assert(!title&&!user&&!uid&&!first&&count==16);*bytes=mode==1?8:16*168;*h=100;open_handles++;title_page=0;return 0;}
static uint32_t item_enum(uint32_t title,uint32_t user,uint64_t uid,uint32_t flags,uint32_t first,uint32_t count,uint32_t *bytes,HANDLE *h){assert(title==123&&!user&&!uid&&flags==7&&!first&&count==16);*bytes=8192;*h=101;open_handles++;item_page=0;return 0;}
static uint32_t tile(uint32_t type,uint32_t title,uint64_t image,uint32_t user,uint8_t *data,uint32_t pitch,uint32_t height,XAM_OVERLAPPED *overlap){assert((type==0||type==1)&&title>=123&&!user&&pitch==256&&height==64&&!overlap);assert(type==1||image==11||image==12);memset(data,255,PROFILE_PICTURE_BYTES);tiles++;return 0;}
static uint32_t show_game(uint32_t user,uint32_t title){assert(!user&&title==123);game_views++;return guide_result;}
static uint32_t show_detail(uint32_t user,uint32_t title,uint32_t achievement){assert(!user&&title==123&&achievement==7);detail_views++;return guide_result;}
bool xsdk_system_ui_open(void){return guide_visible;}
uint32_t XexGetModuleHandle(const char *name,HANDLE *h){assert(!strcmp(name,"xam.xex"));*h=1;return 0;}
NTSTATUS XexGetProcedureAddress(HANDLE h,uint32_t ordinal,void **out){assert(h==1);*out=ordinal==755?(void *)title_enum:ordinal==750?(void *)item_enum:ordinal==757?(void *)tile:ordinal==709?(void *)show_game:ordinal==736&&mode!=4?(void *)show_detail:NULL;return *out?0:(NTSTATUS)0xc0000001;}
uint32_t XamUserGetSigninState(uint32_t user){return user==0?1:0;}
uint32_t XamUserGetXUID(uint32_t user,uint32_t mask,uint64_t *uid){assert(!user&&mask==7);*uid=10;return 0;}
uint32_t XamEnumerate(HANDLE h,uint32_t flags,void *buffer,uint32_t bytes,uint32_t *count,XAM_OVERLAPPED *overlap){
    assert(!flags&&!overlap);uint8_t *p=buffer;memset(p,0,bytes);
    if(h==100){if(title_page++||mode==3){*count=0;return 18;}assert(bytes>=336);*count=2;put(p,124);put(p+4,10);put(p+8,2);wide(p+40,"Game B");p+=168;put(p,123);put(p+4,3);put(p+8,2);put(p+12,1000);put(p+16,20);wide(p+40,"Game A");return 0;}
    assert(h==101);if(item_page++){*count=0;return 18;}*count=3;
    for(unsigned n=0;n<3;n++){uint8_t *r=p+n*36;unsigned at=512+n*512;put(r,n+1);put(r+4,(uint32_t)(uintptr_t)p+at);put(r+8,(uint32_t)(uintptr_t)p+at+128);put(r+12,(uint32_t)(uintptr_t)p+at+256);put(r+16,11+n);put(r+20,10);put(r+32,n<2?0x00020008:0);wide(p+at,"Example");wide(p+at+128,"Unlocked description");wide(p+at+256,"Locked description");}
    if(mode==2)put(p+4,(uint32_t)(uintptr_t)p-1);return 0;
}
NTSTATUS NtClose(HANDLE h){assert((h==100||h==101)&&open_handles);open_handles--;return 0;}
NTSTATUS xsdk_thread_move(uint32_t thread){assert(thread==5);return 0;}
NTSTATUS xsdk_thread_set_priority(HANDLE h,int32_t priority,int32_t *previous){assert(h==XSDK_CURRENT_THREAD&&priority==-1&&!previous);return 0;}
void xsdk_log_write(const char *format,...){(void)format;}
static const AchievementSnapshot *wait_ready(AchievementService *s){for(unsigned n=0;n<300;n++){const AchievementSnapshot *p=achievements_poll(s);if(!p->busy)return p;struct timespec t={0,10000000};nanosleep(&t,NULL);}assert(!"native backend timed out");return NULL;}
int main(void){
    XboxAchievements *a=xbox_achievements_open();assert(a);AchievementService *s=xbox_achievements_service(a);achievements_set_user(s,0,10);AchievementRequest r={.type=ACH_LOAD,.user=0,.xuid=10};
    assert(achievements_request(s,r));const AchievementSnapshot *p=wait_ready(s);assert(!p->error&&p->title_count==2&&p->titles[0].id==123&&p->item_count==3&&achievement_grid_count(p)==2&&tiles==4&&!open_handles);assert(p->tiles[0].valid&&p->tiles[0].pixels[0]==255&&p->items[2].secret);
    AchievementRequest view={.type=ACH_GUIDE_DETAIL,.user=0,.xuid=10,.title_id=123,.achievement_id=7};
    assert(!achievements_request(s,view));assert(!xbox_achievements_show(a,&view)&&detail_views==1&&!game_views);
    guide_result=997;assert(xbox_achievements_show(a,&view)==997&&detail_views==2);guide_result=0;
    guide_visible=true;assert(xbox_achievements_show(a,&view)==170&&detail_views==2);guide_visible=false;
    view.xuid=11;assert(xbox_achievements_show(a,&view)==1317&&detail_views==2);view.xuid=10;view.title_id=0;assert(xbox_achievements_show(a,&view)==87);view.title_id=123;
    view.type=ACH_GUIDE_GAME;assert(!xbox_achievements_show(a,&view)&&game_views==1);view.type=ACH_GUIDE_DETAIL;
    mode=1;assert(achievements_request(s,r));p=wait_ready(s);assert(p->error&&p->error_code==13&&!open_handles);
    mode=2;assert(achievements_request(s,r));p=wait_ready(s);assert(p->error&&p->error_code==13&&!open_handles);
    mode=3;assert(achievements_request(s,r));p=wait_ready(s);assert(!p->error&&!p->title_count&&!p->item_count&&!open_handles);
    xbox_achievements_close(a);mode=4;a=xbox_achievements_open();assert(a);assert(xbox_achievements_show(a,&view)==50&&detail_views==2);xbox_achievements_close(a);
    puts("PASS: actual native backend parameters, BE history/achievement buffers, local pictures, earned-grid filtering, malformed sizes/pointers, empty completion handle cleanup, verified native Guide targets/returns, busy/stale guards and no custom fallback");return 0;
}
