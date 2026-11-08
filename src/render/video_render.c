#include "blades.h"
#include "draw_list.h"
#include <math.h>
#include <stdio.h>
static void timestamp(char *out,size_t bytes,double seconds) {
    unsigned value=(unsigned)fmax(0,fmin(seconds,359999));
    if(value>=3600)snprintf(out,bytes,"%u:%02u:%02u",value/3600,value/60%60,value%60);
    else snprintf(out,bytes,"%u:%02u",value/60,value%60);
}
void dashboard_video_render(Canvas *c,const Dashboard *d) {
    fill_rect(c,0,0,WIDTH,HEIGHT,0xff000000);
    if(d->video_has_picture&&d->video_picture.rgba) {
        static unsigned ids[VIDEO_FRAME_SLOTS];unsigned slot=d->video_picture.slot;
        if(slot>=VIDEO_FRAME_SLOTS)return;
        if(!ids[slot])ids[slot]=canvas_image_id();
        DrawImage image={.pixels=d->video_picture.rgba,.id=ids[slot],.revision=d->video_picture.revision,
            .width=(int)d->video_picture.width,.height=(int)d->video_picture.height,.format=d->video_picture.format==VIDEO_RGBA?IMAGE_RGBA:(ImageFormat)(IMAGE_YUV601+d->video_picture.format-VIDEO_YUV601)};
        double aspect=d->video_picture.aspect;if(!isfinite(aspect)||aspect<=0)aspect=16./9;
        int width=WIDTH,height=HEIGHT;
        if(!d->video_stretch){if(aspect>WIDTH/(double)HEIGHT)height=(int)lround(WIDTH/aspect);else width=(int)lround(HEIGHT*aspect);}
        if(width<1)width=1;
        if(height<1)height=1;
        canvas_image_tagged(c,image,(WIDTH-width)/2,(HEIGHT-height)/2,width,height);
    }
    if(!d->video_controls)return;
    for(int y=585;y<HEIGHT;y++)fill_rect(c,0,y,WIDTH,1,((unsigned)(190*(y-585)/(HEIGHT-585))<<24));
    char position[24],duration[24],time[60];timestamp(position,sizeof position,d->video.position);timestamp(duration,sizeof duration,d->video.duration);
    snprintf(time,sizeof time,"%s / %s",position,duration);text(c,68,608,25,0xffeff3ec,time);
    const char *state=d->video.seeking?"Seeking...":!d->video_has_picture?"Opening video...":d->video.state==VIDEO_PAUSED?"Paused":"";
    text(c,WIDTH-68-text_width(25,state),608,25,0xffeff3ec,state);
    fill_rect(c,68,646,1144,3,0x75ffffff);
    if(d->video.duration>0)fill_rect(c,68,646,(int)(1144*fmin(1,fmax(0,d->video.position/d->video.duration))),3,0xffa3c97a);
    text(c,68,668,24,0xffeff3ec,d->video.state==VIDEO_PAUSED?"A  Resume    B  Back":"A  Pause    B  Back");
    const char *help=d->video.network&&d->video.duration<=0?"X  Fit/Stretch":"Left/Right  Seek    X  Fit/Stretch";
    text(c,WIDTH-68-text_width(24,help),668,24,0xffeff3ec,help);
}
