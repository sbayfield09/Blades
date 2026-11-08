#include "blades.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void put(uint8_t *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static void wide(uint8_t *p,const char *text){for(;*text;text++,p+=2){p[0]=0;p[1]=*text;}p[0]=p[1]=0;}
static void parser(void){
    uint8_t title[168]={0};put(title,0x12345678);put(title+4,50);put(title+8,47);put(title+12,1000);put(title+16,920);wide(title+40,"Example Game?");put(title+32,(uint32_t)132223104000000000ULL);put(title+36,(uint32_t)(132223104000000000ULL>>32));AchievementTitle t;
    assert(achievement_title_record(title,sizeof title,&t)&&t.id==0x12345678&&t.earned==47&&t.score_earned==920&&t.played==132223104000000000ULL&&!strcmp(t.name,"Example Game"));assert(!achievement_title_record(title,167,&t));
    memset(title+40,65,128);assert(!achievement_title_record(title,sizeof title,&t));
    uint8_t raw[512]={0};const uint32_t base=0x10000000;put(raw,7);put(raw+4,base+64);put(raw+8,base+128);put(raw+12,base+256);put(raw+16,11);put(raw+20,10);put(raw+32,8);
    wide(raw+64,"First Win");wide(raw+128,"You won your first race.");wide(raw+256,"Win your first race.");AchievementItem item;
    assert(achievement_item_record(raw,sizeof raw,base,0,&item)&&!item.earned&&!item.secret&&item.score==10&&!strcmp(item.description,"Win your first race."));
    put(raw+24,(uint32_t)132223104000000000ULL);put(raw+28,(uint32_t)(132223104000000000ULL>>32));put(raw+32,0x00020008);assert(achievement_item_record(raw,sizeof raw,base,0,&item)&&item.earned&&item.unlocked==132223104000000000ULL&&!strcmp(item.description,"You won your first race."));
    put(raw+32,0x00010008);assert(achievement_item_record(raw,sizeof raw,base,0,&item)&&item.earned);
    put(raw+32,0x30000008);assert(achievement_item_record(raw,sizeof raw,base,0,&item)&&!item.earned);
    put(raw+32,0x00020008);
    put(raw+4,base-1);assert(!achievement_item_record(raw,sizeof raw,base,0,&item));put(raw+32,0);assert(achievement_item_record(raw,sizeof raw,base,0,&item)&&item.secret&&!strcmp(item.name,"Secret Achievement"));assert(!strstr(item.description,"first race"));
    put(raw+32,8);put(raw+4,base+65);assert(!achievement_item_record(raw,sizeof raw,base,0,&item));assert(!achievement_item_record(raw,35,base,0,&item));assert(!achievement_item_record(raw,sizeof raw,base,500,&item));
    AchievementSnapshot *snapshot=calloc(1,sizeof *snapshot);assert(snapshot);snapshot->item_count=3;snapshot->items[0]=(AchievementItem){.id=1,.earned=true,.score=10,.unlocked=100};snapshot->items[1]=(AchievementItem){.id=2,.score=40};snapshot->items[2]=(AchievementItem){.id=3,.earned=true,.score=20,.unlocked=200};achievement_prepare(snapshot);unsigned index;assert(achievement_grid_count(snapshot)==2&&achievement_grid_item(snapshot,0,&index)==snapshot->items+2&&index==2);assert(achievement_view_index(snapshot,0,false)==2&&achievement_view_index(snapshot,0,true)==1);assert(!achievement_grid_item(snapshot,2,NULL));free(snapshot);
    char date[16];achievement_date(132223104000000000ULL,date);assert(!strcmp(date,"01/01/2020"));achievement_date(0,date);assert(!date[0]);achievement_date(UINT64_MAX,date);assert(!date[0]);
}
static void press(Dashboard *d,unsigned a){dashboard_update(d,a,1.f/60);}
static void navigation(void){
    Dashboard d;dashboard_init(&d);press(&d,SELECT);assert(d.achievement_signin_pending&&!d.depth&&d.profile_command.type==PROFILE_SIGN_IN&&!d.achievement_page);
    ProfileSnapshot p={.identity={.user=2,.state=PROFILE_LOCAL,.xuid=20,.name="Example"},.revision=1};dashboard_set_profile(&d,&p);press(&d,0);assert(d.achievement_page&&d.depth==1&&d.achievement_command.type==ACH_LOAD&&!d.achievement_signin_pending);
    AchievementSnapshot *s=calloc(1,sizeof *s);assert(s);s->title_count=4;s->item_count=25;s->title_id=123;s->titles[0].id=123;s->titles[1].id=124;s->revision=1;
    for(unsigned i=0;i<25;i++){s->items[i].earned=true;s->items[i].id=100+i;}achievement_prepare(s);dashboard_set_achievements(&d,s);
    press(&d,RIGHT);assert(d.achievement_grid);press(&d,DOWN);press(&d,DOWN);press(&d,DOWN);assert(d.achievement_index==18&&d.achievement_scroll==18&&d.achievement_command.type==ACH_IMAGES);
    press(&d,SELECT);assert(d.achievement_command.type==ACH_GUIDE_DETAIL&&d.achievement_command.user==2&&d.achievement_command.xuid==20&&d.achievement_command.title_id==123&&d.achievement_command.achievement_id==118);
    assert(d.achievement_page&&d.achievement_grid&&d.achievement_index==18);press(&d,0);assert(!d.achievement_command.type&&d.achievement_index==18);
    press(&d,LEFT);assert(!d.achievement_grid);press(&d,DOWN);assert(d.achievement_game==1&&d.achievement_command.type==ACH_GAME&&d.achievement_command.title_id==124);
    press(&d,SELECT);assert(d.achievement_command.type==ACH_GUIDE_GAME&&d.achievement_command.title_id==124&&d.achievement_command.user==2&&d.achievement_command.xuid==20);
    assert(d.achievement_page&&!d.achievement_grid);press(&d,PROFILE);assert(d.achievement_command.type==ACH_LOAD&&d.achievement_game==0);
    p.identity.xuid=21;p.revision++;dashboard_set_profile(&d,&p);assert(!d.achievement_page&&!d.depth);
    press(&d,SELECT);assert(d.achievement_page);press(&d,BACK);assert(!d.achievement_page&&!d.depth&&d.previous_achievement_page);
    p.identity.state=PROFILE_SIGNED_OUT;p.identity.xuid=0;dashboard_set_profile(&d,&p);press(&d,SELECT);assert(d.profile_command.type==PROFILE_SIGN_IN);press(&d,BACK);assert(!d.achievement_signin_pending);free(s);
}
typedef struct {pthread_mutex_t lock;uint64_t xuid;bool hold,empty;unsigned error,calls,began;} Fake;
static bool identity(void *ctx,unsigned user,uint64_t uid){Fake *f=ctx;pthread_mutex_lock(&f->lock);bool match=user<4&&uid==f->xuid;pthread_mutex_unlock(&f->lock);return match;}
static unsigned load(void *ctx,const AchievementRequest *r,AchievementSnapshot *s,AchievementCancelled cancel,void *c){
    Fake *f=ctx;pthread_mutex_lock(&f->lock);f->began++;pthread_mutex_unlock(&f->lock);for(;;){pthread_mutex_lock(&f->lock);bool hold=f->hold;pthread_mutex_unlock(&f->lock);if(!hold||cancel(c))break;struct timespec t={0,1000000};nanosleep(&t,NULL);}
    pthread_mutex_lock(&f->lock);unsigned error=f->error;bool empty=f->empty;f->calls++;pthread_mutex_unlock(&f->lock);
    if(error)return error;s->title_count=empty?0:2;s->item_count=empty?0:25;s->title_id=r->title_id?r->title_id:123;snprintf(s->titles[0].name,sizeof s->titles[0].name,"Profile %llu",(unsigned long long)r->xuid);return 0;
}
static const AchievementSnapshot *wait_ready(AchievementService *s){for(unsigned i=0;i<300;i++){const AchievementSnapshot *p=achievements_poll(s);if(!p->busy)return p;struct timespec t={0,10000000};nanosleep(&t,NULL);}assert(!"achievement worker timeout");return NULL;}
static void service(void){
    Fake f={.lock=PTHREAD_MUTEX_INITIALIZER,.xuid=20};AchievementHooks h={.context=&f,.identity=identity,.load=load};AchievementService *s=achievements_open(&h);assert(s);achievements_set_user(s,2,20);
    AchievementRequest r={.type=ACH_LOAD,.user=2,.xuid=20};assert(achievements_request(s,r));const AchievementSnapshot *p=wait_ready(s);assert(p->title_count==2&&p->item_count==25&&!p->error&&!strcmp(p->titles[0].name,"Profile 20"));
    pthread_mutex_lock(&f.lock);f.empty=true;pthread_mutex_unlock(&f.lock);assert(achievements_request(s,r));p=wait_ready(s);assert(!p->title_count&&!p->item_count&&!p->error);
    pthread_mutex_lock(&f.lock);f.error=5;pthread_mutex_unlock(&f.lock);assert(achievements_request(s,r));p=wait_ready(s);assert(p->error&&p->error_code==5);
    pthread_mutex_lock(&f.lock);f.error=0;f.empty=false;f.hold=true;unsigned expected=f.began+1;pthread_mutex_unlock(&f.lock);assert(achievements_request(s,r));
    for(unsigned i=0;i<100;i++){pthread_mutex_lock(&f.lock);bool began=f.began>=expected;pthread_mutex_unlock(&f.lock);if(began)break;struct timespec t={0,1000000};nanosleep(&t,NULL);}
    pthread_mutex_lock(&f.lock);assert(f.began>=expected);pthread_mutex_unlock(&f.lock);achievements_set_user(s,3,30);p=achievements_poll(s);assert(!p->busy&&!p->title_count&&!p->item_count&&p->xuid==30);assert(!achievements_request(s,r));
    pthread_mutex_lock(&f.lock);f.xuid=30;f.hold=false;pthread_mutex_unlock(&f.lock);r.user=3;r.xuid=30;assert(achievements_request(s,r));p=wait_ready(s);assert(!p->error&&!strcmp(p->titles[0].name,"Profile 30"));
    pthread_mutex_lock(&f.lock);f.xuid=40;pthread_mutex_unlock(&f.lock);assert(achievements_request(s,r));p=wait_ready(s);assert(p->error_code==1317&&!p->title_count&&!p->item_count);
    achievements_set_user(s,3,0);p=achievements_poll(s);assert(!p->title_count&&!p->item_count&&!p->xuid);achievements_close(s);pthread_mutex_destroy(&f.lock);
}
int main(void){parser();navigation();service();puts("PASS: bounded achievement/title strings, secret/unlocked descriptions, dates, sign-in gate/resume, grid paging, real-Guide target commands, selection preservation, back navigation, scope cancellation and empty/error completion");return 0;}
