#define _DEFAULT_SOURCE
#include "video.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void check(VideoPlayer *player,bool after_seek){
    double end=now()+8,last_pts=-1;unsigned light=0;bool heard=false;
    while(now()<end){
        float pcm[512];video_audio_render(player,pcm,256);
        VideoPicture image;if(video_take_picture(player,&image)){
            last_pts=image.pts;light=image.rgba[(image.height/2)*image.width+image.width/2];
            video_release_picture(player,image.slot,image.revision);
        }
        VideoSnapshot state;video_poll(player,&state);assert(state.state!=VIDEO_FAILED);
        for(unsigned i=0;i<512;i++)if(fabsf(pcm[i])>.12f)heard=true;
        if(heard){
            printf("flash/tone %s: video_pts=%.3f source_tone=3.000 light=%u\n",after_seek?"after seek":"initial",last_pts,light);fflush(stdout);
            assert(fabs(last_pts-3)<.10&&light>180&&"audio tone plays ahead of its matching video flash");return;
        }
        usleep(1000);
    }
    assert(!"synchronized tone was not heard");
}
int main(int argc,char **argv){
    assert(argc==2);VideoPlayer *player=video_create(&(VideoConfig){.channels=2,.audio_available=true,.gpu_yuv=true});assert(player&&video_play(player,argv[1]));
    check(player,false);
    VideoSnapshot state;video_poll(player,&state);video_pause(player,true);
    /* Exclude the intentional5ms click-suppression tail of the old tone. */
    for(unsigned i=0;i<4;i++){float silence[512];video_audio_render(player,silence,256);}
    video_seek(player,1-state.position);
    /* Let the local seek acknowledge its generation before unpausing. */
    for(unsigned i=0;i<1000;i++){video_poll(player,&state);if(fabs(state.position-1)<.01)break;usleep(1000);}
    assert(fabs(state.position-1)<.01);video_pause(player,false);check(player,true);video_close(player);
    puts("PASS: timestamp-gap flash/tone synchronization within100ms at start and after seek");
}
