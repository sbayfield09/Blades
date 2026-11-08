#include "blades.h"
#include "draw_list.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void put(uint8_t *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static void record(uint8_t *p,unsigned user,uint32_t id,uint32_t value){put(p,1);put(p+8,user);put(p+16,id);p[24]=1;put(p+32,value);}
static void parser(void) {
    uint8_t data[256]={0};const uint32_t base=0x10000000;put(data,4);put(data+4,base+8);
    record(data+8,2,0x10040012,19);record(data+48,2,0x10040006,123456);record(data+88,2,0x10040013,0);
    record(data+128,2,0x4064000f,50);data[152]=4;put(data+164,base+168);
    const char *key="FFFE07D10002000100010001";for(unsigned i=0;i<24;i++)data[169+i*2]=key[i];
    ProfileSnapshot p={.identity={.user=2}};char pic[64];
    assert(profile_settings(data,sizeof data,base,&p,pic));assert(p.known==7&&p.games==19&&p.score==123456&&p.achievements==0);assert(!strcmp(pic,key));
    p.identity.user=1;assert(profile_settings(data,sizeof data,base,&p,pic)&&!p.known&&!pic[0]);p.identity.user=2;
    assert(!profile_settings(data,7,base,&p,pic));assert(!profile_settings(data,160,base,&p,pic));
    put(data+4,base-1);assert(!profile_settings(data,sizeof data,base,&p,pic));put(data+4,base+200);assert(!profile_settings(data,sizeof data,base,&p,pic));put(data+4,base+8);
    put(data,0xffffffff);assert(!profile_settings(data,sizeof data,base,&p,pic));put(data,4);
    put(data+40,0xffffffff);assert(profile_settings(data,sizeof data,base,&p,pic)&&!(p.known&PROFILE_GAMES));put(data+40,19);
    data[72]=4;assert(profile_settings(data,sizeof data,base,&p,pic)&&!(p.known&PROFILE_SCORE));data[72]=1;
    put(data+88,0);assert(profile_settings(data,sizeof data,base,&p,pic)&&!(p.known&PROFILE_ACHIEVEMENTS));put(data+88,1);
    put(data+164,base+250);assert(profile_settings(data,sizeof data,base,&p,pic)&&!pic[0]);put(data+164,base+168);
    data[168]=1;assert(profile_settings(data,sizeof data,base,&p,pic)&&!pic[0]);data[168]=0;
    put(data+104,0x10040006);assert(!profile_settings(data,sizeof data,base,&p,pic));
    uint32_t title;uint64_t image;assert(profile_picture_key(key,&title,&image)&&title==0xfffe07d1&&image==0x0002000100010001ULL);
    assert(!profile_picture_key("FFFE07D1000200010001000Z",&title,&image));assert(!profile_picture_key("FFFE07D10000000000000000",&title,&image));assert(!profile_picture_key("short",&title,&image));
    uint8_t src[64*260]={0},dst[PROFILE_PICTURE_BYTES];src[0]=128;src[1]=12;src[2]=34;src[3]=56;
    assert(profile_picture_argb(src,sizeof src,260,dst));assert(dst[0]==12&&dst[1]==34&&dst[2]==56&&dst[3]==128);
    src[0]=0;assert(profile_picture_argb(src,sizeof src,260,dst)&&dst[3]==255);src[1]=src[2]=src[3]=0;assert(!profile_picture_argb(src,sizeof src,260,dst));
    assert(!profile_picture_argb(src,63*260,260,dst));assert(!profile_picture_argb(src,sizeof src,255,dst));
}
typedef struct {pthread_mutex_t lock;ProfileIdentity users[4];unsigned score,calls,signouts,error,available;bool replace,count_fail;} Fake;
static bool identity(void *c,unsigned user,ProfileIdentity *p){Fake *f=c;pthread_mutex_lock(&f->lock);*p=f->users[user];pthread_mutex_unlock(&f->lock);return true;}
static void details(void *c,ProfileSnapshot *p){Fake *f=c;pthread_mutex_lock(&f->lock);p->known=7;p->score=f->score;p->games=23;p->achievements=100;f->calls++;if(f->replace){f->users[0]=(ProfileIdentity){.user=0,.state=PROFILE_LOCAL,.xuid=99,.name="Replacement"};f->replace=false;}pthread_mutex_unlock(&f->lock);}
static bool profiles(void *c,unsigned *count){Fake *f=c;pthread_mutex_lock(&f->lock);*count=f->available;bool ok=!f->count_fail;pthread_mutex_unlock(&f->lock);return ok;}
static unsigned signout(void *c,unsigned user,uint64_t expected){Fake *f=c;pthread_mutex_lock(&f->lock);assert(f->users[user].xuid==expected);f->signouts++;unsigned error=f->error;if(!error)f->users[user]=(ProfileIdentity){.user=user,.state=PROFILE_SIGNED_OUT};pthread_mutex_unlock(&f->lock);return error;}
static void pause_poll(void){struct timespec t={0,10000000};nanosleep(&t,NULL);}
static const ProfileSnapshot *wait_profile(ProfileService *s,unsigned user,uint64_t xuid,unsigned score,bool error) {
    for(unsigned i=0;i<300;i++){const ProfileSnapshot *p=profile_poll(s);if(p->identity.user==user&&p->identity.xuid==xuid&&!p->loading&&(!xuid||(p->known==7&&p->score==score))&&(!error||p->error_serial))return p;pause_poll();}
    assert(!"profile worker timed out");return NULL;
}
static void service(void) {
    Fake f={.lock=PTHREAD_MUTEX_INITIALIZER,.score=12345,.available=2};
    for(unsigned i=0;i<4;i++)f.users[i]=(ProfileIdentity){.user=i,.state=PROFILE_SIGNED_OUT};
    f.users[0]=(ProfileIdentity){.user=0,.state=PROFILE_LIVE,.xuid=10,.name="Example"};f.users[2]=(ProfileIdentity){.user=2,.state=PROFILE_LOCAL,.xuid=20,.name="Local Player"};
    uint64_t plan[4];assert(profile_signout_plan(f.users,0,10,plan)&&plan[0]==0&&plan[2]==20);assert(!profile_signout_plan(f.users,0,11,plan));assert(!profile_signout_plan(f.users,1,10,plan));f.users[3].state=PROFILE_UNAVAILABLE;assert(!profile_signout_plan(f.users,0,10,plan));f.users[3].state=PROFILE_SIGNED_OUT;
    ProfileHooks hooks={.context=&f,.identity=identity,.details=details,.profiles=profiles,.sign_out=signout};ProfileService *s=profile_open(&hooks);assert(s);
    const ProfileSnapshot *p=wait_profile(s,0,10,12345,false);assert(!strcmp(p->identity.name,"Example"));unsigned rev=p->revision;
    pthread_mutex_lock(&f.lock);assert(!f.signouts);f.score=23456;pthread_mutex_unlock(&f.lock);profile_refresh(s);
    p=wait_profile(s,0,10,23456,false);assert(p->revision>rev);
    profile_set_user(s,2);p=profile_poll(s);assert(p->loading&&!p->known&&!p->has_picture&&p->identity.user==2);wait_profile(s,2,20,23456,false);
    assert(!profile_request(s,(ProfileCommand){.type=PROFILE_SIGN_OUT,.user=0,.xuid=10}));
    assert(profile_request(s,(ProfileCommand){.type=PROFILE_SIGN_OUT,.user=2,.xuid=20}));wait_profile(s,2,0,0,false);
    pthread_mutex_lock(&f.lock);assert(f.signouts==1&&f.users[0].xuid==10);f.error=5;pthread_mutex_unlock(&f.lock);
    profile_set_user(s,0);wait_profile(s,0,10,23456,false);assert(profile_request(s,(ProfileCommand){.type=PROFILE_SIGN_OUT,.user=0,.xuid=10}));p=wait_profile(s,0,10,23456,true);assert(p->error_code==5);
    pthread_mutex_lock(&f.lock);f.replace=true;f.score=34567;pthread_mutex_unlock(&f.lock);profile_refresh(s);p=wait_profile(s,0,99,34567,false);assert(!strcmp(p->identity.name,"Replacement"));
    assert(profile_request(s,(ProfileCommand){.type=PROFILE_SIGN_OUT,.user=0,.xuid=10}));
    for(unsigned i=0;i<100;i++){p=profile_poll(s);if(p->error_code==1317)break;pause_poll();}assert(p->error_code==1317);
    pthread_mutex_lock(&f.lock);assert(f.signouts==2&&f.users[0].xuid==99);pthread_mutex_unlock(&f.lock);profile_set_user(s,1);
    for(unsigned i=0;i<100;i++){p=profile_poll(s);if(p->identity.user==1&&p->profiles_known)break;pause_poll();}
    assert(p->profiles_known&&p->available_profiles==2);
    pthread_mutex_lock(&f.lock);f.available=0;pthread_mutex_unlock(&f.lock);profile_refresh(s);
    for(unsigned i=0;i<100;i++){p=profile_poll(s);if(p->profiles_known&&!p->available_profiles)break;pause_poll();}assert(p->profiles_known&&!p->available_profiles);
    pthread_mutex_lock(&f.lock);f.count_fail=true;pthread_mutex_unlock(&f.lock);profile_refresh(s);
    for(unsigned i=0;i<100;i++){p=profile_poll(s);if(!p->profiles_known)break;pause_poll();}assert(!p->profiles_known);
profile_close(s);pthread_mutex_destroy(&f.lock);
}
static void press(Dashboard *d,unsigned action){dashboard_update(d,action,1.f/60);}
static void navigation(void) {
    Dashboard d;dashboard_init(&d);press(&d,UP);assert(d.profile_card_selected);press(&d,SELECT);assert(d.profile_command.type==PROFILE_SIGN_IN&&!d.depth);press(&d,DOWN);assert(!d.profile_card_selected&&d.selected[d.blade]==0);
    press(&d,PROFILE);assert(d.profile_command.type==PROFILE_SIGN_IN&&!d.dialog);
    ProfileSnapshot p={.identity={.state=PROFILE_LOCAL,.user=2,.xuid=20,.name="Example"},.revision=3};dashboard_set_profile(&d,&p);
    press(&d,PROFILE);assert(d.profile_command.type==PROFILE_SIGN_OUT&&d.profile_command.user==2&&d.profile_command.xuid==20&&!d.dialog);
    press(&d,0);assert(!d.profile_command.type);
    press(&d,UP);assert(d.profile_card_selected);press(&d,SELECT);assert(d.profile_command.type==PROFILE_VIEW&&d.profile_command.user==2&&d.profile_command.xuid==20);
    p.revision++;dashboard_set_profile(&d,&p);assert(d.profile_card_selected);
    press(&d,DOWN);assert(!d.profile_card_selected&&d.selected[d.blade]==0);
    p.busy=true;press(&d,PROFILE);assert(!d.profile_command.type);
    p.busy=false;p.identity.state=PROFILE_SIGNED_OUT;p.identity.xuid=0;p.revision++;dashboard_set_profile(&d,&p);press(&d,UP);assert(d.profile_card_selected);
    p.identity.state=PROFILE_LIVE;p.identity.xuid=22;p.revision++;dashboard_set_profile(&d,&p);assert(d.profile_card_selected);press(&d,SELECT);assert(d.profile_command.type==PROFILE_VIEW&&d.profile_command.xuid==22);

}
int main(void){parser();service();navigation();puts("PASS: bounded XAM stats/picture parsing, profile polling/replacement, controller slot changes, stale sign-out protection, other-user preservation and Guide card navigation");return 0;}
