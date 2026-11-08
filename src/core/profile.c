#include "profile.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
struct ProfileService {
    pthread_t thread;pthread_mutex_t lock;ProfileHooks hooks;
    unsigned stop,user,revision,picture_revision;bool refresh,pending,busy;ProfileCommand command;
    ProfileSnapshot front,published,working;
};
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static bool span(size_t size,size_t at,size_t n){return at<=size&&n<=size-at;}
bool profile_signed_in(const ProfileSnapshot *p){return p&&p->identity.xuid&&(p->identity.state==PROFILE_LOCAL||p->identity.state==PROFILE_LIVE);}
bool profile_settings(const uint8_t *p,size_t size,uint32_t base,ProfileSnapshot *out,char key[64]) {
    out->known=0;key[0]=0;
    if(size<8)return false;
    uint32_t count=be32(p),ptr=be32(p+4);if(count>32||ptr<base||!span(size,ptr-base,(size_t)count*40))return false;
    for(size_t i=0;i<count;i++) {
        const uint8_t *r=p+(ptr-base)+i*40;
        if(!be32(r)||be32(r+8)!=out->identity.user)continue;
        uint32_t id=be32(r+16),value=be32(r+32);unsigned flag=id==0x10040012?PROFILE_GAMES:id==0x10040006?PROFILE_SCORE:id==0x10040013?PROFILE_ACHIEVEMENTS:0;
        if(flag&&r[24]==1&&value<=INT32_MAX) {
            if(out->known&flag)return false;
            if(flag==PROFILE_GAMES)out->games=value;
            if(flag==PROFILE_SCORE)out->score=value;
            if(flag==PROFILE_ACHIEVEMENTS)out->achievements=value;
            out->known|=flag;
        }
        if(id==0x4064000f&&r[24]==4) {
            uint32_t bytes=value,text=be32(r+36);
            if(bytes<2||(bytes&1)||text<base||!span(size,text-base,bytes))continue;
            size_t at=0;const uint8_t *s=p+text-base;
            for(size_t j=0;j+1<bytes&&at<63;j+=2){if(s[j]||s[j+1]>=128){at=0;break;}if(!s[j+1])break;key[at++]=(char)s[j+1];}
            key[at]=0;
        }
    }
    return true;
}
bool profile_picture_key(const char *key,uint32_t *title,uint64_t *image) {
    *title=0;*image=0;if(strlen(key)!=24)return false;
    for(unsigned i=0;i<24;i++) {
        unsigned c=(unsigned char)key[i],n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:16;
        if(n>15)return false;
        if(i<8)*title=(*title<<4)|n;else *image=(*image<<4)|n;
    }
    return *image!=0;
}
bool profile_picture_argb(const uint8_t *p,size_t bytes,size_t pitch,uint8_t out[PROFILE_PICTURE_BYTES]) {
    if(!p||pitch<64*4||pitch>4096||bytes/pitch<64)return false;
    bool alpha=false,color=false;
    for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++) {
        const uint8_t *s=p+y*pitch+x*4;uint8_t *d=out+(y*64+x)*4;
        d[0]=s[1];d[1]=s[2];d[2]=s[3];d[3]=s[0];alpha|=s[0]!=0;color|=(s[1]|s[2]|s[3])!=0;
    }
    if(!alpha&&color)for(unsigned i=0;i<64*64;i++)out[i*4+3]=255;
    return alpha||color;
}
static bool same_identity(const ProfileIdentity *a,const ProfileIdentity *b){return a->user==b->user&&a->state==b->state&&a->xuid==b->xuid&&a->guest==b->guest&&!strcmp(a->name,b->name);}
bool profile_signout_plan(const ProfileIdentity users[4],unsigned user,uint64_t expected,uint64_t xuids[4]) {
    if(user>=4||!expected||users[user].xuid!=expected||(users[user].state!=PROFILE_LOCAL&&users[user].state!=PROFILE_LIVE))return false;
    for(unsigned i=0;i<4;i++) {
        if(users[i].state==PROFILE_UNAVAILABLE)return false;
        bool active=users[i].state==PROFILE_LOCAL||users[i].state==PROFILE_LIVE;
        if(active&&!users[i].xuid)return false;
        xuids[i]=active?users[i].xuid:0;
    }
    xuids[user]=0;return true;
}
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void publish(ProfileService *s) {
    pthread_mutex_lock(&s->lock);
    if(s->working.identity.user==s->user) {
        s->working.revision=s->published.revision;s->working.picture_revision=s->published.picture_revision;
        if(memcmp(&s->working,&s->published,sizeof s->working)) {
            bool changed=s->working.has_picture!=s->published.has_picture||s->working.identity.xuid!=s->published.identity.xuid||memcmp(s->working.picture,s->published.picture,PROFILE_PICTURE_BYTES);
            if(changed)s->working.picture_revision=++s->picture_revision;
            s->working.revision=++s->revision;s->published=s->working;s->pending=true;
        }
    }
    pthread_mutex_unlock(&s->lock);
}
static ProfileIdentity read_identity(ProfileService *s,unsigned user) {
    ProfileIdentity id={.user=user,.state=PROFILE_UNAVAILABLE};
    if(s->hooks.identity&&!s->hooks.identity(s->hooks.context,user,&id))id=(ProfileIdentity){.user=user,.state=PROFILE_UNAVAILABLE};
    id.user=user;id.name[sizeof id.name-1]=0;return id;
}
static void *worker(void *context) {
    ProfileService *s=context;if(s->hooks.worker_start)s->hooks.worker_start(s->hooks.context);
    double details_after=0,profiles_after=0;
    while(!__atomic_load_n(&s->stop,__ATOMIC_ACQUIRE)) {
        pthread_mutex_lock(&s->lock);unsigned user=s->user;bool refresh=s->refresh;s->refresh=false;ProfileCommand command=s->command;s->command.type=PROFILE_NONE;pthread_mutex_unlock(&s->lock);
        if(command.type==PROFILE_SIGN_OUT) {
            s->working.busy=true;publish(s);
            ProfileIdentity current=read_identity(s,command.user);
            unsigned error=command.user!=user||current.xuid!=command.xuid||!command.xuid?1317:s->hooks.sign_out?s->hooks.sign_out(s->hooks.context,command.user,command.xuid):50;
            if(error){s->working.error_serial++;s->working.error_code=error;snprintf(s->working.error,sizeof s->working.error,"Sign out failed (code %u). The profile may have changed.",error);}
            s->working.busy=false;refresh=true;
            pthread_mutex_lock(&s->lock);s->busy=false;pthread_mutex_unlock(&s->lock);
        }
        ProfileIdentity id=read_identity(s,user);bool changed=!same_identity(&s->working.identity,&id)||s->working.loading;
        if(changed) {
            unsigned serial=s->working.error_serial,code=s->working.error_code;char error[160];memcpy(error,s->working.error,sizeof error);
            s->working=(ProfileSnapshot){.identity=id,.error_serial=serial,.error_code=code};memcpy(s->working.error,error,sizeof error);publish(s);
        }
        if(!profile_signed_in(&s->working)&&(changed||refresh||seconds()>=profiles_after)&&s->hooks.profiles) {
            unsigned count=0;s->working.profiles_known=s->hooks.profiles(s->hooks.context,&count);
            s->working.available_profiles=s->working.profiles_known?count:0;profiles_after=seconds()+5;
        }
        if(profile_signed_in(&s->working)&&(changed||refresh||seconds()>=details_after)&&s->hooks.details) {
            ProfileSnapshot next=s->working;next.known=0;
            s->hooks.details(s->hooks.context,&next);
            ProfileIdentity fresh=read_identity(s,user);
            if(same_identity(&fresh,&id))s->working=next;
            else {s->working.identity=fresh;s->working.known=0;s->working.has_picture=false;memset(s->working.picture,0,sizeof s->working.picture);details_after=0;publish(s);continue;}
            details_after=seconds()+5;
        }
        publish(s);
        struct timespec delay={0,50000000};for(unsigned n=0;n<5&&!__atomic_load_n(&s->stop,__ATOMIC_ACQUIRE);n++)nanosleep(&delay,NULL);
    }
    return NULL;
}
ProfileService *profile_open(const ProfileHooks *hooks) {
    if(!hooks)return NULL;
    ProfileService *s=calloc(1,sizeof *s);if(!s)return NULL;s->hooks=*hooks;s->front.loading=s->working.loading=true;s->front.revision=s->revision=1;s->published=s->front;
    pthread_mutex_init(&s->lock,NULL);pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,256*1024);
    int error=pthread_create(&s->thread,&attr,worker,s);pthread_attr_destroy(&attr);
    if(error){pthread_mutex_destroy(&s->lock);free(s);return NULL;}return s;
}
void profile_set_user(ProfileService *s,unsigned user) {
    if(!s||user>=4)return;
    pthread_mutex_lock(&s->lock);
    if(s->user!=user){s->user=user;s->refresh=true;s->pending=false;s->front=(ProfileSnapshot){.identity={.user=user},.loading=true,.revision=++s->revision};}
    pthread_mutex_unlock(&s->lock);
}
void profile_refresh(ProfileService *s){if(s){pthread_mutex_lock(&s->lock);s->refresh=true;pthread_mutex_unlock(&s->lock);}}
const ProfileSnapshot *profile_poll(ProfileService *s) {
    if(!s)return NULL;
    pthread_mutex_lock(&s->lock);if(s->pending){s->front=s->published;s->pending=false;}pthread_mutex_unlock(&s->lock);return &s->front;
}
bool profile_request(ProfileService *s,ProfileCommand command) {
    if(!s||command.type!=PROFILE_SIGN_OUT||command.user>=4||!command.xuid)return false;
    pthread_mutex_lock(&s->lock);bool accepted=!s->busy&&s->user==command.user;
    if(accepted){s->command=command;s->busy=true;}pthread_mutex_unlock(&s->lock);return accepted;
}
void profile_close(ProfileService *s){if(s){__atomic_store_n(&s->stop,1,__ATOMIC_RELEASE);pthread_join(s->thread,NULL);pthread_mutex_destroy(&s->lock);free(s);}}
