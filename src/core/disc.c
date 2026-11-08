#include "disc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
void disc_title_copy(char *out,size_t size,const char *s) {
    if(!size)return;
    size_t n=0;
    /* Preserve whole UTF-8 code points; remove controls from media metadata. */
    while(s&&*s&&n+1<size) {
        unsigned char c=(unsigned char)*s;unsigned bytes=c<0x80?1:c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;
        if(!bytes){s++;continue;}
        bool valid=true;for(unsigned i=1;i<bytes;i++)if(!s[i]||((unsigned char)s[i]&0xc0)!=0x80){valid=false;break;}
        if(!valid){s++;continue;}
        if(bytes==3&&((c==0xe0&&(unsigned char)s[1]<0xa0)||(c==0xed&&(unsigned char)s[1]>=0xa0)))valid=false;
        if(bytes==4&&((c==0xf0&&(unsigned char)s[1]<0x90)||(c==0xf4&&(unsigned char)s[1]>=0x90)))valid=false;
        if(!valid){s+=bytes;continue;}
        if(n+bytes>=size)break;
        if(c<32||c==127)out[n++]=' ';
        else {memcpy(out+n,s,bytes);n+=bytes;}
        s+=bytes;
    }
    while(n&&out[n-1]==' ')n--;
    out[n]=0;
}
static void invalidate(DiscDrive *d,DiscState state) {
    d->status.state=state;d->status.title_id=0;d->status.title[0]=0;d->status.media=DISC_MEDIA_PENDING;d->status.generation++;
    d->reading_seconds=0;
    d->last_media=UINT32_MAX;d->media_samples=0;
}
void disc_init(DiscDrive *d) { *d=(DiscDrive){.status={.state=DISC_UNAVAILABLE}}; }
void disc_observe(DiscDrive *d,DiscTray tray,float dt) {
    if(!isfinite(dt)||dt<0)dt=0;
    if(tray>TRAY_ERROR)tray=TRAY_ERROR;
    if(d->pending) {
        d->pending_seconds+=dt;
        bool old=d->pending==DISC_EJECT?(tray==TRAY_CLOSED||tray==TRAY_EMPTY||tray==TRAY_CLOSING):tray==TRAY_OPEN;
        if(old&&d->pending_seconds<3)return; /* Allow the hardware to acknowledge a command. */
        d->pending=DISC_NONE;
        if(old){invalidate(d,DISC_ERROR);d->observed=false;return;}
    }
    if(!d->observed||tray!=d->tray) {
        bool was_closed=d->observed&&(d->tray==TRAY_EMPTY||d->tray==TRAY_CLOSED);
        bool now_closed=tray==TRAY_EMPTY||tray==TRAY_CLOSED;
        d->observed=true;d->tray=tray;
        static const DiscState states[]={DISC_EMPTY,DISC_CLOSING,DISC_OPEN,DISC_OPENING,DISC_READING,DISC_ERROR};
        /* CLOSED/EMPTY cached values can change during authentication. Keep
           an in-flight identification alive until the media query settles. */
        if(!was_closed||!now_closed)invalidate(d,states[tray]);
        else if(d->status.state==DISC_READY||(d->status.state==DISC_EMPTY&&tray==TRAY_CLOSED))invalidate(d,DISC_READING);
    }
    if((tray==TRAY_CLOSED||tray==TRAY_EMPTY)&&d->status.state==DISC_READING) {
        d->reading_seconds+=dt;
        if(d->reading_seconds>=30)d->status.state=DISC_ERROR;
    }
}
bool disc_can_eject(const DiscStatus *s) {
    return s->state==DISC_READY||s->state==DISC_READING||s->state==DISC_UNSUPPORTED||s->state==DISC_ERROR;
}
DiscCommand disc_select(const DiscStatus *s) {
    if(s->state==DISC_OPEN)return DISC_CLOSE;
    if(s->state==DISC_READY)return DISC_LAUNCH;
    if(s->state==DISC_EMPTY||s->state==DISC_UNSUPPORTED||s->state==DISC_ERROR)return DISC_EJECT;
    return DISC_NONE;
}
bool disc_request(DiscDrive *d,DiscCommand cmd,uint32_t generation) {
    if(d->pending||generation!=d->status.generation)return false;
    if(cmd==DISC_LAUNCH)return d->status.state==DISC_READY;
    if(cmd==DISC_EJECT&&(disc_can_eject(&d->status)||d->status.state==DISC_EMPTY))invalidate(d,DISC_OPENING);
    else if(cmd==DISC_CLOSE&&d->status.state==DISC_OPEN)invalidate(d,DISC_CLOSING);
    else return false;
    d->pending=cmd;d->pending_seconds=0;return true;
}
bool disc_identified(DiscDrive *d,uint32_t generation,uint32_t title_id,const char *title,bool playable) {
    if(d->pending||generation!=d->status.generation||(d->tray!=TRAY_CLOSED&&d->tray!=TRAY_EMPTY)||
       (d->status.state!=DISC_READING&&d->status.state!=DISC_ERROR))return false;
    d->status.title_id=title_id;
    disc_title_copy(d->status.title,sizeof d->status.title,title);
    d->status.state=playable?DISC_READY:DISC_UNSUPPORTED;
    d->status.media=playable?DISC_MEDIA_GAME:DISC_MEDIA_OTHER;return true;
}
bool disc_begin_reading(DiscDrive *d,uint32_t generation) {
    if(d->pending||generation!=d->status.generation||(d->tray!=TRAY_CLOSED&&d->tray!=TRAY_EMPTY))return false;
    if(d->status.state==DISC_EMPTY||d->status.state==DISC_UNSUPPORTED){d->status.state=DISC_READING;d->reading_seconds=0;}
    return d->status.state==DISC_READING||d->status.state==DISC_ERROR;
}
DiscMedia disc_media_kind(uint32_t returned,uint32_t raw) {
    (void)returned; /* Observed title ID 5553083B with media word 1 on hardware. */
    switch(raw) {
    case 0:return DISC_MEDIA_NONE;
    case 1:case 9:return DISC_MEDIA_GAME;
    case 5:return DISC_MEDIA_DVD_VIDEO;
    case 7:return DISC_MEDIA_AUDIO_CD;
    case 8:return DISC_MEDIA_DATA;
    case 2:case 4:case 6:case 10:return DISC_MEDIA_OTHER;
    case 3:return DISC_MEDIA_UNKNOWN;
    default:return DISC_MEDIA_PENDING; /* Includes FFFFFFFE and failed/unset queries. */
    }
}
DiscMedia disc_media_update(DiscDrive *d,uint32_t generation,uint32_t returned,uint32_t raw) {
    if(d->pending||generation!=d->status.generation||(d->tray!=TRAY_EMPTY&&d->tray!=TRAY_CLOSED))return DISC_MEDIA_PENDING;
    bool same_media=d->media_samples&&raw==d->last_media;
    if(same_media) {if(d->media_samples<3)d->media_samples++;}
    else {d->last_media=raw;d->media_samples=1;}
    DiscMedia media=disc_media_kind(returned,raw);
    if(media==DISC_MEDIA_NONE) {
        /* Two independent one-second polls, after the close has settled.
           Never turn an empty drive into Unrecognized Disc. */
        if(d->status.state!=DISC_EMPTY&&d->reading_seconds>=3&&d->media_samples>=2)invalidate(d,DISC_EMPTY);
    } else if(media==DISC_MEDIA_AUDIO_CD||media==DISC_MEDIA_DVD_VIDEO||media==DISC_MEDIA_DATA) {
        if(d->status.state!=DISC_READY||d->status.media!=media) {
            disc_begin_reading(d,generation);
            if(d->reading_seconds>=3&&d->media_samples>=3&&disc_identified(d,generation,0,NULL,true))d->status.media=media;
        }
    } else if(media==DISC_MEDIA_OTHER) {
        if(d->status.state!=DISC_UNSUPPORTED) {
            disc_begin_reading(d,generation);
            if(d->reading_seconds>=3&&d->media_samples>=3)disc_identified(d,generation,0,NULL,false);
        }
    } else {
        if(media==DISC_MEDIA_UNKNOWN&&d->status.state==DISC_UNSUPPORTED&&same_media)return media;
        disc_begin_reading(d,generation);
        if(media==DISC_MEDIA_UNKNOWN&&d->reading_seconds>=30&&d->media_samples>=3)
            disc_identified(d,generation,0,NULL,false);
    }
    return media;
}
void disc_label(const DiscStatus *s,char *out,size_t size) {
    static const char *const names[]={"Open Tray","Opening","Close Tray","Closing","Reading","Play Xbox 360 Game","Unrecognized Disc","Can't Read Disc","Drive Unavailable"};
    if(s->state==DISC_READY&&(s->media==DISC_MEDIA_AUDIO_CD||s->media==DISC_MEDIA_DVD_VIDEO||s->media==DISC_MEDIA_DATA))
        snprintf(out,size,"%s",s->media==DISC_MEDIA_AUDIO_CD?"Play CD":s->media==DISC_MEDIA_DVD_VIDEO?"Play DVD":"Open Data Disc");
    else if(s->state==DISC_READY&&s->title[0])snprintf(out,size,"Play %s",s->title);
    else snprintf(out,size,"%s",names[s->state<=DISC_UNAVAILABLE?s->state:DISC_UNAVAILABLE]);
}
bool disc_dashboard_data(DiscMedia media,unsigned user,uint8_t data[1020]) {
    unsigned command;
    if(media==DISC_MEDIA_AUDIO_CD)command=6;
    else if(media==DISC_MEDIA_DVD_VIDEO)command=7;
    else if(media==DISC_MEDIA_DATA)command=0;
    else return false;
    memset(data,0,1020);data[7]=(uint8_t)command;
    data[11]=(uint8_t)(media==DISC_MEDIA_DVD_VIDEO?0xff:user<4?user:0xff);return true;
}
const char *disc_launch_path(DiscMedia media) {
    return media==DISC_MEDIA_AUDIO_CD||media==DISC_MEDIA_DVD_VIDEO||media==DISC_MEDIA_DATA?
        "/Device/Flash/dash.xex":"/Device/CdRom0/default.xex";
}
unsigned disc_reading_frame(float seconds) {
    if(!isfinite(seconds)||seconds<=0)return 0;
    return (unsigned)(fmodf(seconds,1.f/1.7f)*27.2f)%16;
}
