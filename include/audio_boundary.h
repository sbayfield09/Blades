#ifndef BLADES_AUDIO_BOUNDARY_H
#define BLADES_AUDIO_BOUNDARY_H
#include <stdbool.h>
#include <stddef.h>
typedef struct {unsigned opening,closing;float stop_gain;bool stopping,finished;} AudioBoundary;
/* Device lifecycle envelope only; navigation clips retain their original PCM. */
void audio_boundary_apply(AudioBoundary *state,float *stereo,size_t frames,bool stop);
#endif
