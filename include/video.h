#ifndef BLADES_VIDEO_H
#define BLADES_VIDEO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"
enum { VIDEO_FRAME_SLOTS=4,VIDEO_MAX_WIDTH=1280,VIDEO_MAX_HEIGHT=720,VIDEO_AUDIO_RATE=48000,VIDEO_AUDIO_CHANNELS=6 };
typedef enum { VIDEO_RGBA,VIDEO_YUV601,VIDEO_YUV709,VIDEO_YUV601_FULL,VIDEO_YUV709_FULL } VideoPixelFormat;
typedef enum { VIDEO_IDLE,VIDEO_OPENING,VIDEO_PLAYING,VIDEO_PAUSED,VIDEO_STOPPING,VIDEO_ENDED,VIDEO_FAILED } VideoState;
typedef struct {
    VideoState state;unsigned revision,width,height,channels,decoded,dropped;
    double position,duration,aspect,decode_ms,prepare_ms;
    unsigned presented,late_frames,late_presented;uint64_t audio_underrun_frames,audio_gap_frames,audio_overlap_frames;
    double video_lag_ms;bool audio_preroll;
    unsigned catchup_level,catchup_episodes;
    bool has_audio,network,seeking;int network_status;char error[192];
} VideoSnapshot;
/* Private authenticated request: never put this in Dashboard or render logs.
 * Progressive seeking reopens a server stream at start_position, then maps
 * its timestamps onto the full item timeline. Local keeps byte seeking. */
typedef struct {xf_endpoint endpoint;char request[XF_REQUEST_LIMIT],stop_request[XF_REQUEST_LIMIT];double duration,start_position;bool restart;} VideoHttpInput;
/* YUV variants: width x height Y, followed by height/2 rows containing U
   (width/2 bytes) and V (width/2 bytes) side by side. rgba points at the atlas. */
typedef struct {const unsigned char *rgba;unsigned width,height,slot,revision;double pts,aspect;VideoPixelFormat format;} VideoPicture;
typedef struct {unsigned channels,audio_latency_frames;bool audio_available;void (*log)(const char *);bool gpu_yuv;} VideoConfig;
typedef struct VideoPlayer VideoPlayer;
VideoPlayer *video_create(const VideoConfig *);
bool video_play(VideoPlayer *,const char *path);
bool video_play_http(VideoPlayer *,const VideoHttpInput *);
void video_stop(VideoPlayer *);
void video_pause(VideoPlayer *,bool paused);
void video_seek(VideoPlayer *,double relative_seconds);
void video_poll(VideoPlayer *,VideoSnapshot *);
bool video_take_picture(VideoPlayer *,VideoPicture *);
void video_release_picture(VideoPlayer *,unsigned slot,unsigned revision);
/* Writes n interleaved frames, config.channels channels. Nonblocking w.r.t. I/O
   and decode; silence on pause/underflow. The service remains alive until close. */
bool video_audio_render(VideoPlayer *,float *out,unsigned frames);
unsigned video_audio_channels(const VideoPlayer *);
void video_close(VideoPlayer *);
bool video_state_active(VideoState);
#endif
