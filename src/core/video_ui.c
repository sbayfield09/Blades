#include "blades.h"
#include <math.h>
#include <stdio.h>
bool dashboard_video_open(Dashboard *d,VideoPlayer *player,const char *path) {
    if(!player||!video_play(player,path))return false;
    if(d->video_has_picture)video_release_picture(player,d->video_picture.slot,d->video_picture.revision);
    d->video_picture=(VideoPicture){0};d->video_has_picture=false;
    d->video_view=true;d->video_controls=true;d->video_controls_time=3;
    d->video_guide=false;d->video_resume_guide=false;video_poll(player,&d->video);return true;
}
bool dashboard_video_http_open(Dashboard *d,VideoPlayer *player,const VideoHttpInput *input){
    if(!player||!video_play_http(player,input))return false;
    if(d->video_has_picture)video_release_picture(player,d->video_picture.slot,d->video_picture.revision);
    d->video_picture=(VideoPicture){0};d->video_has_picture=false;d->video_view=true;d->video_controls=true;d->video_controls_time=3;
    d->video_guide=false;d->video_resume_guide=false;video_poll(player,&d->video);return true;
}
static void close_view(Dashboard *d,VideoPlayer *player) {
    video_stop(player);
    if(d->video_has_picture)video_release_picture(player,d->video_picture.slot,d->video_picture.revision);
    d->video_has_picture=false;d->video_picture=(VideoPicture){0};d->video_view=false;
    d->library_wait=false;d->video_guide=false;d->video_resume_guide=false;
}
bool dashboard_video_update(Dashboard *d,VideoPlayer *player,unsigned actions,float dt,bool guide) {
    if(!d->video_view)return false;
    d->actions=actions;d->sound=UI_SOUND_NONE;d->disc_command=DISC_NONE;
    d->library_command=(LibraryCommand){0};d->profile_command=(ProfileCommand){0};d->achievement_command=(AchievementRequest){0};
    d->ftp_toggle=false;d->ftp_edit=0;d->elapsed+=fmaxf(0,dt);
    d->jelly_command=(JellyCommand){0};d->jelly_edit=0;
    video_poll(player,&d->video);
    if(guide&&!d->video_guide){d->video_resume_guide=d->video.state!=VIDEO_PAUSED;video_pause(player,true);}
    if(!guide&&d->video_guide&&d->video_resume_guide){video_pause(player,false);d->video_resume_guide=false;}
    d->video_guide=guide;
    if(!guide) {
        if(actions&(BACK|EXIT)){close_view(d,player);return true;}
        if(actions&SELECT)video_pause(player,d->video.state!=VIDEO_PAUSED);
        if(actions&LEFT)video_seek(player,-10);
        if(actions&RIGHT)video_seek(player,10);
        if(actions&PROFILE)d->video_stretch=!d->video_stretch;
        if(actions&(UP|DOWN|EJECT))d->video_controls_time=d->video_controls?0:3;
        else if(actions)d->video_controls_time=3;
    }
    d->video_controls_time=fmaxf(0,d->video_controls_time-fmaxf(0,dt));
    video_poll(player,&d->video);
    if(d->video.state==VIDEO_FAILED) {
        snprintf(d->video_error,sizeof d->video_error,"Could not play this video.\n%s\nPress B to return.",d->video.error);
        close_view(d,player);d->dialog=true;d->message=d->video_error;return true;
    }
    if(d->video.state==VIDEO_ENDED||d->video.state==VIDEO_IDLE||d->video.state==VIDEO_STOPPING){close_view(d,player);return true;}
    VideoPicture picture;
    if(video_take_picture(player,&picture)) {
        if(d->video_has_picture)video_release_picture(player,d->video_picture.slot,d->video_picture.revision);
        d->video_picture=picture;d->video_has_picture=true;
    }
    d->video_controls=d->video.seeking||d->video_controls_time>0||d->video.state==VIDEO_PAUSED||!d->video_has_picture;
    return true;
}
