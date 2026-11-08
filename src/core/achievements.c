#include "achievements.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
/* FILETIME is low DWORD then high DWORD; each DWORD is BE on Xenon. */
static uint64_t filetime(const uint8_t *p){return (uint64_t)be32(p+4)<<32|be32(p);}
static bool unicode(const uint8_t *p,size_t bytes,size_t at,char *out,size_t capacity){
    size_t n=0;bool ended=false;if(at&1||at>=bytes)return false;
    for(;at+1<bytes;at+=2){unsigned c=(unsigned)p[at]<<8|p[at+1];if(!c){ended=true;break;}if(n+1<capacity)out[n++]=c>=32&&c<127?(char)c:c=='\n'||c=='\r'?' ': '?';}
    out[n]=0;return ended;
}
bool achievement_title_record(const uint8_t *p,size_t bytes,AchievementTitle *out){
    if(bytes<168||!be32(p))return false;
    *out=(AchievementTitle){.id=be32(p),.possible=be32(p+4),.earned=be32(p+8),.score_possible=be32(p+12),.score_earned=be32(p+16),.played=filetime(p+32)};
    if(!unicode(p,168,40,out->name,sizeof out->name))return false;
    size_t clean=0;for(size_t i=0;out->name[i];i++)if(out->name[i]!='?')out->name[clean++]=out->name[i];out->name[clean]=0;
    if(!out->name[0])snprintf(out->name,sizeof out->name,"Xbox 360 Game");
    return true;
}
bool achievement_item_record(const uint8_t *p,size_t bytes,uint32_t base,size_t offset,AchievementItem *out){
    if(offset>bytes||bytes-offset<36)return false;
    const uint8_t *r=p+offset;
    *out=(AchievementItem){.id=be32(r),.image=be32(r+16),.score=be32(r+20),.unlocked=filetime(r+24),.flags=be32(r+32)};
    out->earned=(out->flags&0x00030000)!=0;out->secret=!out->earned&&!(out->flags&8);
    if(out->secret){snprintf(out->name,sizeof out->name,"Secret Achievement");snprintf(out->description,sizeof out->description,"Continue playing to unlock this secret achievement.");return true;}
    uint32_t name=be32(r+4),description=be32(r+(out->earned?8:12));
    if(name&&(name<base||!unicode(p,bytes,name-base,out->name,sizeof out->name)))return false;
    if(description&&(description<base||!unicode(p,bytes,description-base,out->description,sizeof out->description)))return false;
    if(!out->name[0])snprintf(out->name,sizeof out->name,"Achievement %u",out->id);
    if(!out->description[0])snprintf(out->description,sizeof out->description,"A description is unavailable for this achievement.");
    return true;
}
static bool before(const AchievementItem *a,const AchievementItem *b,bool score){
    if(score&&a->score!=b->score)return a->score>b->score;
    if(a->earned!=b->earned)return a->earned;
    if(a->unlocked!=b->unlocked)return a->unlocked>b->unlocked;
    return a->id<b->id;
}
void achievement_prepare(AchievementSnapshot *s){
    unsigned count=s->item_count<ACH_ITEM_LIMIT?s->item_count:ACH_ITEM_LIMIT;
    for(unsigned mode=0;mode<2;mode++){
        uint16_t *order=mode?s->score_order:s->order;
        for(unsigned i=0;i<count;i++){unsigned j=i;while(j&&before(s->items+i,s->items+order[j-1],mode!=0)){order[j]=order[j-1];j--;}order[j]=(uint16_t)i;}
    }s->ordered=true;
}
unsigned achievement_view_index(const AchievementSnapshot *s,unsigned row,bool score){if(!s||row>=s->item_count||row>=ACH_ITEM_LIMIT)return ACH_ITEM_LIMIT;return s->ordered?(score?s->score_order[row]:s->order[row]):row;}
unsigned achievement_grid_count(const AchievementSnapshot *s){unsigned count=0;if(s)for(unsigned i=0;i<s->item_count&&i<ACH_ITEM_LIMIT;i++)count+=s->items[i].earned;return count;}
const AchievementItem *achievement_grid_item(const AchievementSnapshot *s,unsigned row,unsigned *index){
    if(!s)return NULL;
    for(unsigned i=0;i<s->item_count&&i<ACH_ITEM_LIMIT;i++){unsigned at=achievement_view_index(s,i,false);if(at>=s->item_count)continue;if(s->items[at].earned){if(!row){if(index)*index=at;return s->items+at;}row--;}}
    return NULL;
}
void achievement_date(uint64_t ticks,char date[16]){
    date[0]=0;if(ticks<116444736000000000ULL)return;uint64_t sec=(ticks-116444736000000000ULL)/10000000;
    if(sec>253402300799ULL)return;
    time_t t=(time_t)sec;struct tm value;if(gmtime_r(&t,&value))strftime(date,16,"%d/%m/%Y",&value);
}
struct AchievementService {pthread_t thread;pthread_mutex_t lock;pthread_cond_t wake;AchievementHooks hooks;unsigned user,epoch,job_epoch,revision;uint64_t xuid;bool stop,pending;AchievementRequest job;AchievementSnapshot front,work,ready;};
static bool cancelled(void *ctx){AchievementService *s=ctx;pthread_mutex_lock(&s->lock);bool value=s->stop||s->epoch!=s->job_epoch;pthread_mutex_unlock(&s->lock);return value;}
static void *worker(void *ctx){
    AchievementService *s=ctx;if(s->hooks.worker_start)s->hooks.worker_start(s->hooks.context);
    for(;;){
        pthread_mutex_lock(&s->lock);while(!s->stop&&!s->job.type)pthread_cond_wait(&s->wake,&s->lock);
        if(s->stop){pthread_mutex_unlock(&s->lock);break;}
        AchievementRequest request=s->job;s->job.type=ACH_NONE;s->job_epoch=s->epoch;s->work=s->front;pthread_mutex_unlock(&s->lock);
        unsigned error=!s->hooks.identity||!s->hooks.identity(s->hooks.context,request.user,request.xuid)?1317:s->hooks.load?s->hooks.load(s->hooks.context,&request,&s->work,cancelled,s):50;
        if(!cancelled(s)&&!error)achievement_prepare(&s->work);
        if(!cancelled(s)&&s->hooks.identity&&!s->hooks.identity(s->hooks.context,request.user,request.xuid))error=1317;
        if(error==1317){s->work.title_count=s->work.item_count=0;s->work.title_id=0;memset(s->work.tiles,0,sizeof s->work.tiles);}
        pthread_mutex_lock(&s->lock);
        if(!s->stop&&s->epoch==s->job_epoch){s->work.busy=false;s->work.error=error!=0;s->work.error_code=error;s->work.revision=++s->revision;s->ready=s->work;s->pending=true;}
        pthread_mutex_unlock(&s->lock);
    }return NULL;
}
AchievementService *achievements_open(const AchievementHooks *hooks){
    AchievementService *s=calloc(1,sizeof *s);if(!s)return NULL;if(hooks)s->hooks=*hooks;
    pthread_mutex_init(&s->lock,NULL);pthread_cond_init(&s->wake,NULL);pthread_attr_t a;pthread_attr_init(&a);pthread_attr_setstacksize(&a,256*1024);int error=pthread_create(&s->thread,&a,worker,s);pthread_attr_destroy(&a);
    if(error){pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->lock);free(s);return NULL;}return s;
}
void achievements_set_user(AchievementService *s,unsigned user,uint64_t xuid){if(!s)return;pthread_mutex_lock(&s->lock);if(s->user!=user||s->xuid!=xuid){s->epoch++;s->user=user;s->xuid=xuid;s->pending=false;s->job.type=ACH_NONE;memset(&s->front,0,sizeof s->front);s->front.user=user;s->front.xuid=xuid;s->front.revision=++s->revision;}pthread_mutex_unlock(&s->lock);}
bool achievements_request(AchievementService *s,AchievementRequest r){
    if(!s||r.type<ACH_LOAD||r.type>ACH_IMAGES||r.user>=4||!r.xuid)return false;
    pthread_mutex_lock(&s->lock);bool ok=s->user==r.user&&s->xuid==r.xuid;
    if(ok){s->epoch++;s->pending=false;s->job=r;s->front.busy=r.type!=ACH_IMAGES;s->front.error=false;s->front.error_code=0;
        if(r.type==ACH_LOAD){s->front.title_count=s->front.item_count=0;s->front.title_id=0;s->front.truncated=false;memset(s->front.tiles,0,sizeof s->front.tiles);}
        if(r.type==ACH_GAME){s->front.item_count=0;s->front.title_id=r.title_id;s->front.truncated=false;memset(s->front.tiles,0,sizeof s->front.tiles);}
        s->front.revision=++s->revision;pthread_cond_signal(&s->wake);}
    pthread_mutex_unlock(&s->lock);return ok;
}
void achievements_cancel(AchievementService *s){if(s){pthread_mutex_lock(&s->lock);s->epoch++;s->job.type=ACH_NONE;s->pending=false;s->front.busy=false;s->front.revision=++s->revision;pthread_mutex_unlock(&s->lock);}}
const AchievementSnapshot *achievements_poll(AchievementService *s){if(!s)return NULL;pthread_mutex_lock(&s->lock);if(s->pending){s->front=s->ready;s->pending=false;}pthread_mutex_unlock(&s->lock);return &s->front;}
void achievements_close(AchievementService *s){if(s){pthread_mutex_lock(&s->lock);s->stop=true;pthread_cond_signal(&s->wake);pthread_mutex_unlock(&s->lock);pthread_join(s->thread,NULL);pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->lock);free(s);}}
