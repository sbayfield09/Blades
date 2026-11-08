#include "xbox_achievements.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include "xsdk/thread.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
struct XboxAchievements {
    AchievementService *service;
    uint32_t (*show_game)(uint32_t,uint32_t);
    uint32_t (*show_detail)(uint32_t,uint32_t,uint32_t);
    uint32_t (*titles)(uint32_t,uint32_t,uint64_t,uint32_t,uint32_t,uint32_t *,HANDLE *);
    uint32_t (*items)(uint32_t,uint32_t,uint64_t,uint32_t,uint32_t,uint32_t,uint32_t *,HANDLE *);
    uint32_t (*tile)(uint32_t,uint32_t,uint64_t,uint32_t,uint8_t *,uint32_t,uint32_t,XAM_OVERLAPPED *);
};
static void *resolve(unsigned ordinal){HANDLE h;void *p=NULL;return XexGetModuleHandle("xam.xex",&h)==0&&NT_SUCCESS(XexGetProcedureAddress(h,ordinal,&p))?p:NULL;}
static bool identity(void *ctx,unsigned user,uint64_t expected){(void)ctx;uint64_t uid=0;return XamUserGetSigninState(user)>0&&!XamUserGetXUID(user,7,&uid)&&uid==expected;}
static int compare_titles(const void *a,const void *b){const AchievementTitle *x=a,*y=b;if(x->played!=y->played)return x->played>y->played?-1:1;return strcasecmp(x->name,y->name);}
static unsigned titles(XboxAchievements *a,const AchievementRequest *r,AchievementSnapshot *s,AchievementCancelled cancel,void *c){
    /* Requestee zero reads the local user slot, including offline profiles.
       The captured XUID is reserved for before/after identity validation. */
    if(!a->titles)return 50;
    uint32_t bytes=0;HANDLE h=0;unsigned error=a->titles(0,r->user,0,0,16,&bytes,&h);
    if(error||!h){if(h)NtClose(h);return error?error:13;}
    if(bytes<168||bytes>262144){NtClose(h);return 13;}uint8_t *data=malloc(bytes);if(!data){NtClose(h);return 8;}
    unsigned result=0;bool finished=false;s->title_count=0;
    for(unsigned page=0;page<ACH_TITLE_LIMIT/16+1;page++){
        if(cancel(c)){result=995;break;}uint32_t count=0;error=XamEnumerate(h,0,data,bytes,&count,NULL);
        if(error==18||(!error&&!count)){finished=true;break;}
        if(error){result=error;break;}if(count>bytes/168){result=13;break;}
        for(unsigned i=0;i<count;i++){
            if(s->title_count==ACH_TITLE_LIMIT){s->truncated=true;break;}
            AchievementTitle value;if(!achievement_title_record(data+i*168,168,&value)){result=13;break;}
            if(value.id==0x424c4453||value.id==0xfffe07d1)continue;
            bool duplicate=false;for(unsigned n=0;n<s->title_count;n++)if(s->titles[n].id==value.id){duplicate=true;break;}
            if(!duplicate)s->titles[s->title_count++]=value;
        }
        if(result||s->truncated)break;
    }
    if(!finished&&!s->truncated&&!result)result=13;
    free(data);NtClose(h);if(!result)qsort(s->titles,s->title_count,sizeof s->titles[0],compare_titles);return result;
}
static unsigned items(XboxAchievements *a,const AchievementRequest *r,AchievementSnapshot *s,AchievementCancelled cancel,void *c){
    if(!a->items)return 50;
    uint32_t bytes=0;HANDLE h=0;unsigned error=a->items(s->title_id,r->user,0,7,0,16,&bytes,&h);
    if(error||!h){if(h)NtClose(h);return error?error:13;}
    if(bytes<36||bytes>262144){NtClose(h);return 13;}uint8_t *data=malloc(bytes);if(!data){NtClose(h);return 8;}
    unsigned result=0;bool finished=false;s->item_count=0;
    for(unsigned page=0;page<ACH_ITEM_LIMIT/16+1;page++){
        if(cancel(c)){result=995;break;}uint32_t count=0;error=XamEnumerate(h,0,data,bytes,&count,NULL);
        if(error==18||(!error&&!count)){finished=true;break;}
        if(error){result=error;break;}if(count>bytes/36){result=13;break;}
        for(unsigned i=0;i<count;i++){
            if(s->item_count==ACH_ITEM_LIMIT){s->truncated=true;break;}
            AchievementItem value;if(!achievement_item_record(data,bytes,(uint32_t)(uintptr_t)data,i*36,&value)){result=13;break;}
            s->items[s->item_count++]=value;
        }
        if(result||s->truncated)break;
    }
    if(!finished&&!s->truncated&&!result)result=13;
    free(data);NtClose(h);return result;
}
static void images(XboxAchievements *a,const AchievementRequest *r,AchievementSnapshot *s,AchievementCancelled cancel,void *c){
    memset(s->tiles,0,sizeof s->tiles);if(!a->tile)return;
    uint8_t *pixels=malloc(PROFILE_PICTURE_BYTES);if(!pixels)return;
    for(unsigned n=0;n<ACH_TILES&&!cancel(c);n++){
        AchievementTile *tile=s->tiles+n;bool game=n>=ACH_GRID;unsigned index=game?r->game_first+n-ACH_GRID:r->first+n;
        const AchievementItem *item=game?NULL:achievement_grid_item(s,index,NULL);
        if(game?index>=s->title_count:!item)continue;
        uint32_t title=game?s->titles[index].id:s->title_id;uint64_t image=game?0:item->image;
        memset(pixels,0,PROFILE_PICTURE_BYTES);unsigned error=a->tile(game?1:0,title,image,r->user,pixels,256,64,NULL);
        tile->index=index;tile->title=title;
        if(!error)tile->valid=profile_picture_argb(pixels,PROFILE_PICTURE_BYTES,256,tile->pixels);
    }
    free(pixels);
}
static unsigned load(void *ctx,const AchievementRequest *request,AchievementSnapshot *s,AchievementCancelled cancel,void *c){
    XboxAchievements *a=ctx;AchievementRequest r=*request;unsigned error=0;
    if(r.type==ACH_LOAD){error=titles(a,&r,s,cancel,c);if(error||!s->title_count){xsdk_log_write("achievements: history result=%u titles=%u",error,s->title_count);return error;}s->title_id=s->titles[0].id;r.first=r.game_first=0;}
    if(r.type==ACH_LOAD||r.type==ACH_GAME){
        if(!s->title_id)return 13;
        const AchievementTitle *title=NULL;for(unsigned n=0;n<s->title_count;n++)if(s->titles[n].id==s->title_id){title=s->titles+n;break;}
        if(!title)return 87;
        if(title->possible||title->earned){error=items(a,&r,s,cancel,c);if(error){xsdk_log_write("achievements: item metadata result=%u",error);return error;}}else s->item_count=0;
    }
    achievement_prepare(s);
    if(!cancel(c))images(a,&r,s,cancel,c);
    xsdk_log_write("achievements: operation=%u titles=%u items=%u unlocked=%u limited=%u",r.type,s->title_count,s->item_count,achievement_grid_count(s),s->truncated);return 0;
}
static void worker_start(void *ctx){(void)ctx;(void)xsdk_thread_move(5);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-1,NULL);}
XboxAchievements *xbox_achievements_open(void){
    XboxAchievements *a=calloc(1,sizeof *a);if(!a)return NULL;a->titles=resolve(755);a->items=resolve(750);a->tile=resolve(757);a->show_game=resolve(709);a->show_detail=resolve(736);
    AchievementHooks hooks={.context=a,.identity=identity,.load=load,.worker_start=worker_start};a->service=achievements_open(&hooks);
    if(!a->service){free(a);return NULL;}xsdk_log_write("achievements: read-only APIs titles=%u items=%u images=%u",a->titles!=NULL,a->items!=NULL,a->tile!=NULL);return a;
}
AchievementService *xbox_achievements_service(XboxAchievements *a){return a?a->service:NULL;}
void xbox_achievements_close(XboxAchievements *a){if(a){achievements_close(a->service);free(a);}}

unsigned xbox_achievements_show(XboxAchievements *a,const AchievementRequest *r){
    if(!a||!r||r->user>=4||!r->title_id||(r->type!=ACH_GUIDE_GAME&&r->type!=ACH_GUIDE_DETAIL))return 87;
    if(!identity(a,r->user,r->xuid))return 1317;
    if(xsdk_system_ui_open())return 170;
    /* Verified in the original retail 5766 dashboard: ordinal 736 receives
       r3=user, r4=game Title ID, r5=achievement ID (call at 0x92213754).
       Ordinal 709 receives user and game Title ID. Never substitute the
       Blades app's Title ID or a grid position for the actual achievement ID. */
    if(r->type==ACH_GUIDE_DETAIL)return a->show_detail?a->show_detail(r->user,r->title_id,r->achievement_id):50;
    return a->show_game?a->show_game(r->user,r->title_id):50;
}
