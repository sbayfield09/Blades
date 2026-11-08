#include "audio_boundary.h"
void audio_boundary_apply(AudioBoundary *s,float *stereo,size_t frames,bool stop){
    enum { RAMP=2048,SILENCE=4096 };
    if(stop&&!s->stopping){s->stopping=true;s->stop_gain=s->opening<RAMP?(float)s->opening/RAMP:1.f;}
    for(size_t i=0;i<frames;i++){
        float gain=s->opening<RAMP?(float)s->opening/RAMP:1.f;if(s->opening<RAMP)s->opening++;
        if(s->stopping){gain=s->closing<RAMP?s->stop_gain*(1.f-(float)s->closing/RAMP):0.f;if(s->closing<RAMP+SILENCE)s->closing++;else s->finished=true;}
        stereo[i*2]*=gain;stereo[i*2+1]*=gain;
    }
}
