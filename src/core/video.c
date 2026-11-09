/* Original Blades playback frontend. FFmpeg is separately licensed LGPL2.1+.
 * No XEX handoff, Peer Pressure DLL, code patch or stock-player dependency. */
#include "video.h"
#include "http_client.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/cpu.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef BLADES_XBOX
#include "xsdk/thread.h"
#endif
#define AUDIO_FRAMES 48000u
#define PACKETS 128u
#define VIDEO_PACKET_BYTES (16u*1024u*1024u)
#define AUDIO_PACKET_BYTES (4u*1024u*1024u)
enum { FRAME_FREE,FRAME_WRITING,FRAME_READY,FRAME_HELD };
typedef struct {AVPacket *items[PACKETS];unsigned head,count;size_t bytes;bool eof;} PacketQueue;
typedef struct {uint8_t *pixels;unsigned state,width,height,revision,serial;double pts,aspect;VideoPixelFormat format;} PictureSlot;
typedef struct Session Session;
struct VideoPlayer {
    pthread_mutex_t lock;pthread_cond_t wake;pthread_t worker;
    VideoConfig config;VideoSnapshot state;
    bool quit,stop,pending,running,seek_pending,demux_eof,video_done,audio_done;
    bool demux_waiting_audio,video_needs_packet;
    bool pending_http;VideoHttpInput http_input;
    char path[1024];unsigned serial,next_picture;
    double seek_target,requested_seek,last_video_end,last_audio_end,wall_start,wall_base;
    bool clock_started,audio_started;double audio_base,audio_empty_wall;
    uint64_t audio_consumed,http_generation;
    PacketQueue video_q,audio_q;PictureSlot pictures[VIDEO_FRAME_SLOTS];
    float *audio;unsigned audio_head,audio_count;
    float audio_gain,audio_last[VIDEO_AUDIO_CHANNELS];unsigned audio_tail;
    double telemetry_time,last_picture_wall;unsigned telemetry_decoded,telemetry_presented;
};
struct Session {
    VideoPlayer *p;FILE *file;int64_t file_size;AVIOContext *io;AVFormatContext *format;
    AVCodecContext *video,*audio;int video_index,audio_index;double start,frame_step;
    pthread_t video_thread,audio_thread;bool video_thread_started,audio_thread_started;
    double open_deadline;bool first_decode,first_output;
    unsigned catchup_level;
    bool remote;HttpStream *http;VideoHttpInput http_input;
};
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void place_thread(unsigned cpu){
#ifdef BLADES_XBOX
    (void)xsdk_thread_move(cpu);
#else
    (void)cpu;
#endif
}
static void log_line(VideoPlayer *p,const char *fmt,...) {
    if(!p->config.log)return;
    char line[512];va_list args;va_start(args,fmt);vsnprintf(line,sizeof line,fmt,args);va_end(args);p->config.log(line);
}
static pthread_once_t ffmpeg_once=PTHREAD_ONCE_INIT;
static void init_ffmpeg(void){av_register_all();av_log_set_level(AV_LOG_ERROR);av_max_alloc(64u*1024u*1024u);}
bool video_state_active(VideoState state){return state==VIDEO_OPENING||state==VIDEO_PLAYING||state==VIDEO_PAUSED;}
static bool stopped(VideoPlayer *p){return __atomic_load_n(&p->stop,__ATOMIC_ACQUIRE)||__atomic_load_n(&p->quit,__ATOMIC_ACQUIRE);}
static bool pending_seek(VideoPlayer *p){return __atomic_load_n(&p->seek_pending,__ATOMIC_ACQUIRE);}
static void set_stop(VideoPlayer *p,bool stop){__atomic_store_n(&p->stop,stop,__ATOMIC_RELEASE);}
static void clear_packets(PacketQueue *q) {
    while(q->count){AVPacket *pkt=q->items[q->head];av_packet_unref(pkt);av_free(pkt);q->head=(q->head+1)%PACKETS;q->count--;}
    q->head=0;q->bytes=0;q->eof=false;
}
static void reset_output(VideoPlayer *p,double position) {
    p->audio_head=p->audio_count=0;p->audio_consumed=0;p->audio_started=false;p->audio_empty_wall=0;p->audio_gain=0;
    p->audio_base=position;p->wall_base=position;p->wall_start=seconds();p->clock_started=false;
    p->last_video_end=p->last_audio_end=position;
    p->demux_waiting_audio=p->video_needs_packet=false;p->state.audio_preroll=false;
    p->last_picture_wall=0;p->state.catchup_level=0;
    for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)if(p->pictures[i].state==FRAME_READY)p->pictures[i].state=FRAME_FREE;
}
static double clock_locked(VideoPlayer *p) {
    if(p->state.seeking)return p->state.position;
    if(p->state.state==VIDEO_OPENING)return p->state.network?p->state.position:p->wall_base;
    if(p->state.state==VIDEO_PAUSED)return p->state.position;
    if(p->state.has_audio&&p->config.audio_available) {
        if(!p->audio_started){
            if(p->clock_started&&(p->audio_done||seconds()-p->wall_start>1))return p->wall_base+seconds()-p->wall_start;
            return p->wall_base;
        }
        uint64_t delay=p->config.audio_latency_frames;
        double position=p->audio_base+(p->audio_consumed>delay?(p->audio_consumed-delay)/(double)VIDEO_AUDIO_RATE:0);
        if(p->audio_done&&!p->audio_count) {
            if(!p->audio_empty_wall)p->audio_empty_wall=seconds();
            position+=seconds()-p->audio_empty_wall;
        }
        return position;
    }
    return p->wall_base+(p->clock_started?seconds()-p->wall_start:0);
}
static void fail(VideoPlayer *p,const char *message,int result) {
    char detail[96]={0};if(result<0)av_strerror(result,detail,sizeof detail);
    pthread_mutex_lock(&p->lock);
    if(!stopped(p)) {
        snprintf(p->state.error,sizeof p->state.error,"%s%s%s",message,detail[0]?": ":"",detail);
        p->state.state=VIDEO_FAILED;p->state.revision++;set_stop(p,true);
    }
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    log_line(p,"video: %s%s%s",message,detail[0]?": ":"",detail);
}
static int read_input(void *opaque,uint8_t *out,int bytes) {
    Session *s=opaque;if(stopped(s->p))return AVERROR_EXIT;
    if(s->remote){int n=http_stream_read(s->http,out,bytes);return n>0?n:n==0?AVERROR_EOF:AVERROR(EIO);}
    size_t n=fread(out,1,(size_t)bytes,s->file);
    return n?(int)n:feof(s->file)?AVERROR_EOF:AVERROR(errno?errno:EIO);
}
static int64_t seek_input(void *opaque,int64_t offset,int whence) {
    Session *s=opaque;if(whence==AVSEEK_SIZE)return s->file_size;
    if(s->remote)return AVERROR(ENOSYS);
    if(stopped(s->p))return AVERROR_EXIT;
    whence&=~AVSEEK_FORCE;
    if(whence!=SEEK_SET&&whence!=SEEK_CUR&&whence!=SEEK_END)return AVERROR(EINVAL);
    if(fseeko(s->file,(off_t)offset,whence))return AVERROR(errno?errno:EIO);
    return (int64_t)ftello(s->file);
}
static int interrupt_input(void *opaque) {
    Session *s=opaque;return stopped(s->p)||(s->open_deadline&&seconds()>s->open_deadline);
}
static bool interrupt_http(void *opaque){return interrupt_input(opaque)!=0;}
static AVCodecContext *open_decoder(Session *s,int index,bool video) {
    if(index<0)return NULL;
    AVCodecContext *c=s->format->streams[index]->codec;AVCodec *codec=avcodec_find_decoder(c->codec_id);
    if(!codec)return NULL;
    if(video){c->thread_count=4;c->thread_type=(codec->capabilities&CODEC_CAP_FRAME_THREADS)?FF_THREAD_FRAME:FF_THREAD_SLICE;}
    if(avcodec_open2(c,codec,NULL)<0)return NULL;
    return c;
}
static bool push_packet(VideoPlayer *p,PacketQueue *q,const AVPacket *packet,size_t max_bytes) {
    if(packet->size<0||(unsigned)packet->size>64u*1024u*1024u)return false;
    AVPacket *copy=av_mallocz(sizeof *copy);if(!copy)return false;
    av_init_packet(copy);if(av_packet_ref(copy,packet)<0){av_free(copy);return false;}
    pthread_mutex_lock(&p->lock);
    while(!stopped(p)&&!pending_seek(p)&&(q->count==PACKETS||(q->count&&q->bytes+(size_t)packet->size>max_bytes))){
        if(q==&p->audio_q)p->demux_waiting_audio=true;
        pthread_cond_wait(&p->wake,&p->lock);
    }
    if(q==&p->audio_q)p->demux_waiting_audio=false;
    bool ok=!stopped(p)&&!pending_seek(p);
    if(ok){q->items[(q->head+q->count)%PACKETS]=copy;q->count++;q->bytes+=(size_t)copy->size;}
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    if(!ok){av_packet_unref(copy);av_free(copy);}return ok;
}
static AVPacket *pop_packet(VideoPlayer *p,PacketQueue *q,unsigned *serial,bool *eof) {
    pthread_mutex_lock(&p->lock);
    while(!stopped(p)&&!q->count&&!q->eof){
        if(q==&p->video_q)p->video_needs_packet=true;
        pthread_cond_wait(&p->wake,&p->lock);
    }
    if(q==&p->video_q)p->video_needs_packet=false;
    *serial=p->serial;*eof=q->eof&&!q->count;AVPacket *packet=NULL;
    if(!stopped(p)&&q->count){packet=q->items[q->head];q->head=(q->head+1)%PACKETS;q->count--;q->bytes-=(size_t)packet->size;}
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return packet;
}
static double frame_time(Session *s,AVFrame *frame,int index,double fallback) {
    int64_t pts=av_frame_get_best_effort_timestamp(frame);
    return pts==AV_NOPTS_VALUE?fallback:pts*av_q2d(s->format->streams[index]->time_base)-s->start;
}
static bool output_picture(Session *s,AVFrame *frame,unsigned serial,struct SwsContext **scale,double pts) {
    VideoPlayer *p=s->p;
    if(frame->width<1||frame->height<1||frame->width>1920||frame->height>1080){fail(p,"Video resolution exceeds 1920 x 1080",0);return false;}
    bool planar=p->config.gpu_yuv&&!(frame->width&1)&&!(frame->height&1)&&
        (frame->format==AV_PIX_FMT_YUV420P||frame->format==AV_PIX_FMT_YUVJ420P);
    double factor=planar?1.:fmin(1.,fmin(VIDEO_MAX_WIDTH/(double)frame->width,VIDEO_MAX_HEIGHT/(double)frame->height));
    unsigned w=(unsigned)(frame->width*factor)&~1u,h=(unsigned)(frame->height*factor)&~1u;if(w<2)w=2;if(h<2)h=2;
    pthread_mutex_lock(&p->lock);
    if(serial!=p->serial||pts+.05<p->seek_target){pthread_mutex_unlock(&p->lock);return true;}
    /* Simple360Player VideoLoop: 40ms late skips non-reference deblocking;
       250ms late skips non-reference frames. Hold the level until on time.
       This changes the decoder's work, rather than only discarding its output. */
    double late=p->clock_started?clock_locked(p)-pts:0;
    unsigned level=late>.25?2:late>.04?1:0;
    if(level<s->catchup_level&&late>0)level=s->catchup_level;
    if(level!=s->catchup_level){
        if(level==2&&s->catchup_level<2)p->state.catchup_episodes++;
        s->catchup_level=level;p->state.catchup_level=level;
        s->video->skip_loop_filter=level>=1?AVDISCARD_NONREF:AVDISCARD_DEFAULT;
        s->video->skip_frame=level>=2?AVDISCARD_NONREF:AVDISCARD_DEFAULT;
    }
    /* Match its 500ms anti-freeze safeguard. The timestamp is updated only
       when the renderer takes a picture, never merely when one is decoded. */
    if(late>.5&&p->last_picture_wall&&seconds()-p->last_picture_wall<.5){
        p->state.late_frames++;p->state.dropped++;
        pthread_mutex_unlock(&p->lock);return true;
    }
    PictureSlot *slot=NULL;
    while(!stopped(p)&&serial==p->serial&&!slot){
        for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)if(p->pictures[i].state==FRAME_FREE){slot=&p->pictures[i];slot->state=FRAME_WRITING;break;}
        if(!slot)pthread_cond_wait(&p->wake,&p->lock);
    }
    pthread_mutex_unlock(&p->lock);
    if(!slot||stopped(p))return false;
    double prepare_start=seconds();int rows;VideoPixelFormat format=VIDEO_RGBA;
    if(planar) {
        for(unsigned y=0;y<h;y++)memcpy(slot->pixels+(size_t)y*w,frame->data[0]+(ptrdiff_t)y*frame->linesize[0],w);
        for(unsigned y=0;y<h/2;y++) {
            uint8_t *dest=slot->pixels+(size_t)(h+y)*w;
            memcpy(dest,frame->data[1]+(ptrdiff_t)y*frame->linesize[1],w/2);
            memcpy(dest+w/2,frame->data[2]+(ptrdiff_t)y*frame->linesize[2],w/2);
        }
        bool full=frame->color_range==AVCOL_RANGE_JPEG||frame->format==AV_PIX_FMT_YUVJ420P;
        bool hd=frame->colorspace==AVCOL_SPC_BT709;
        format=full?(hd?VIDEO_YUV709_FULL:VIDEO_YUV601_FULL):(hd?VIDEO_YUV709:VIDEO_YUV601);rows=(int)h;
    } else {
        *scale=sws_getCachedContext(*scale,frame->width,frame->height,(enum AVPixelFormat)frame->format,w,h,AV_PIX_FMT_RGBA,SWS_BILINEAR,NULL,NULL,NULL);
        if(!*scale){fail(p,"Video colour converter could not be created",0);return false;}
        const int *matrix=sws_getCoefficients(frame->colorspace==AVCOL_SPC_BT709?SWS_CS_ITU709:SWS_CS_ITU601);
        (void)sws_setColorspaceDetails(*scale,matrix,frame->color_range==AVCOL_RANGE_JPEG,matrix,1,0,1<<16,1<<16);
        uint8_t *dest[]={slot->pixels,NULL,NULL,NULL};int pitch[]={(int)w*4,0,0,0};
        rows=sws_scale(*scale,(const uint8_t *const*)frame->data,frame->linesize,0,frame->height,dest,pitch);
    }
    double prepare_ms=(seconds()-prepare_start)*1000;
    pthread_mutex_lock(&p->lock);
    if(rows==(int)h&&serial==p->serial&&!stopped(p)) {
        double sar=av_q2d(frame->sample_aspect_ratio);if(!isfinite(sar)||sar<=0)sar=1;
        slot->width=w;slot->height=h;slot->pts=pts;slot->aspect=frame->width*sar/frame->height;
        slot->format=format;slot->serial=serial;slot->revision=++p->next_picture;slot->state=FRAME_READY;
        p->state.width=w;p->state.height=h;p->state.aspect=slot->aspect;p->state.prepare_ms=prepare_ms;
        p->last_video_end=fmax(p->last_video_end,pts+s->frame_step);
    } else slot->state=FRAME_FREE;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    if(!s->first_output&&rows==(int)h){s->first_output=true;log_line(p,"video: first prepared frame format=%u size=%ux%u preparation_ms=%.2f",format,w,h,prepare_ms);}
    return rows==(int)h;
}
static void wait_after_eof(VideoPlayer *,unsigned,bool);
static void *decode_video(void *opaque) {
    Session *s=opaque;VideoPlayer *p=s->p;place_thread(2);
    AVFrame *frame=av_frame_alloc();struct SwsContext *scale=NULL;unsigned seen=0;double next=0;
    if(!frame){fail(p,"Not enough memory for a video frame",0);goto done;}
    while(!stopped(p)) {
        unsigned serial;bool eof;AVPacket *packet=pop_packet(p,&p->video_q,&serial,&eof);
        if(stopped(p)){if(packet){av_packet_unref(packet);av_free(packet);}break;}
        if(serial!=seen){log_line(p,"video: worker ready; flushing codec for generation %u",serial);avcodec_flush_buffers(s->video);s->video->skip_loop_filter=s->video->skip_frame=AVDISCARD_DEFAULT;s->catchup_level=0;log_line(p,"video: codec flush complete");seen=serial;pthread_mutex_lock(&p->lock);next=p->seek_target;pthread_mutex_unlock(&p->lock);}
        AVPacket empty;av_init_packet(&empty);empty.data=NULL;empty.size=0;
        if(!packet&&!eof)continue;
        do {
            if(!s->first_decode)log_line(p,"video: entering first video decode call");
#ifdef BLADES_VIDEO_TEST_DECODE_NS
            /* Test-only decoder-budget model; absent from shipping builds. */
            struct timespec budget={0,BLADES_VIDEO_TEST_DECODE_NS};nanosleep(&budget,NULL);
#endif
            double decode_start=seconds();
            int got=0;int result=avcodec_decode_video2(s->video,frame,&got,packet?packet:&empty);
            double decode_ms=(seconds()-decode_start)*1000;
            pthread_mutex_lock(&p->lock);
            if(serial==p->serial){p->state.decode_ms=p->state.decode_ms*.9+decode_ms*.1;if(got)p->state.decoded++;}
            pthread_mutex_unlock(&p->lock);
            if(result<0){log_line(p,"video: skipped a damaged video packet (%d)",result);break;}
            if(!got)break;
            if(!s->first_decode){
                s->first_decode=true;log_line(p,"video: first decoded frame format=%d size=%dx%d",frame->format,frame->width,frame->height);
#ifdef BLADES_XBOX
                /* Annex-B sources may initialize DSP only after their first
                   in-band SPS; report again once a frame has actually decoded. */
                extern int ff_xb_chroma_verdict,ff_xb_qpel_verdict;
                log_line(p,"video: first-frame Simple360 H264 DSP chroma=%d qpel=%d",ff_xb_chroma_verdict,ff_xb_qpel_verdict);
#endif
            }
            double pts=frame_time(s,frame,s->video_index,next);next=pts+s->frame_step;
            if(!output_picture(s,frame,serial,&scale,pts))break;
        } while(eof&&!stopped(p));
        if(packet){av_packet_unref(packet);av_free(packet);}
        if(eof)wait_after_eof(p,serial,true);
    }
done:
    sws_freeContext(scale);av_frame_free(&frame);
    pthread_mutex_lock(&p->lock);p->video_done=true;pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return NULL;
}
/* Caller holds the output mutex. Padding is streamed through the same bounded
   FIFO as PCM, so long timestamp gaps allocate no extra buffers and cancel. */
static bool append_audio(VideoPlayer *p,const float *samples,uint64_t frames,unsigned serial){
    unsigned channels=p->config.channels;uint64_t copied=0;
    while(copied<frames&&!stopped(p)&&serial==p->serial){
        while(p->audio_count==AUDIO_FRAMES&&!stopped(p)&&serial==p->serial)pthread_cond_wait(&p->wake,&p->lock);
        if(stopped(p)||serial!=p->serial)return false;
        unsigned at=(p->audio_head+p->audio_count)%AUDIO_FRAMES,n=AUDIO_FRAMES-p->audio_count;
        if((uint64_t)n>frames-copied)n=(unsigned)(frames-copied);
        if(n>AUDIO_FRAMES-at)n=AUDIO_FRAMES-at;
        if(n){
            float *dest=p->audio+(size_t)at*channels;
            if(samples)memcpy(dest,samples+(size_t)copied*channels,(size_t)n*channels*sizeof(float));
            else {memset(dest,0,(size_t)n*channels*sizeof(float));p->state.audio_gap_frames+=n;}
            p->audio_count+=n;copied+=n;
        }
        pthread_cond_broadcast(&p->wake);
    }
    return copied==frames;
}
static bool output_audio(Session *s,const float *samples,unsigned frames,double pts,unsigned serial) {
    VideoPlayer *p=s->p;unsigned channels=p->config.channels;
    if(!isfinite(pts)||fabs(pts)>1e12){fail(p,"Invalid audio timestamps",0);return false;}
    pthread_mutex_lock(&p->lock);
    if(serial!=p->serial||stopped(p)){pthread_mutex_unlock(&p->lock);return false;}
    /* The FIFO clock starts at the seek origin, not at the first audio packet.
       Missing source time must remain silence; concatenating packets made
       subsequent sound play early relative to the video timestamps. */
    double expected=p->audio_base+(p->audio_consumed+p->audio_count)/(double)VIDEO_AUDIO_RATE;
    double gap=pts-expected;
    if(gap<-.002){
        unsigned skip=(unsigned)fmin(frames,ceil(-gap*VIDEO_AUDIO_RATE));
        samples+=(size_t)skip*channels;frames-=skip;p->state.audio_overlap_frames+=skip;
    }else if(gap>.002&&frames){
        if(!append_audio(p,NULL,(uint64_t)llround(gap*VIDEO_AUDIO_RATE),serial)){pthread_mutex_unlock(&p->lock);return false;}
    }
    bool ok=append_audio(p,samples,frames,serial);
    if(ok&&serial==p->serial)p->last_audio_end=fmax(p->last_audio_end,p->audio_base+(p->audio_consumed+p->audio_count)/(double)VIDEO_AUDIO_RATE);
    pthread_mutex_unlock(&p->lock);return ok;
}
static bool resample_audio(Session *s,AVFrame *frame,SwrContext **resample,int *rate,int *format,int64_t *layout,
                           float **samples,unsigned *capacity,double *next,unsigned serial) {
    int in_rate=frame->sample_rate?frame->sample_rate:s->audio->sample_rate;
    int64_t in_layout=frame->channel_layout?(int64_t)frame->channel_layout:av_get_default_channel_layout(frame->channels);
    if(in_rate<=0||!in_layout||frame->nb_samples<0){fail(s->p,"Unsupported audio layout",0);return false;}
    if(!*resample||*rate!=in_rate||*format!=frame->format||*layout!=in_layout) {
        swr_free(resample);int64_t out=s->p->config.channels==6?AV_CH_LAYOUT_5POINT1_BACK:AV_CH_LAYOUT_STEREO;
        *resample=swr_alloc_set_opts(NULL,out,AV_SAMPLE_FMT_FLT,VIDEO_AUDIO_RATE,in_layout,(enum AVSampleFormat)frame->format,in_rate,0,NULL);
        if(!*resample||swr_init(*resample)<0){fail(s->p,"Audio resampler could not be created",0);return false;}
        *rate=in_rate;*format=frame->format;*layout=in_layout;
    }
    int limit=swr_get_out_samples(*resample,frame->nb_samples);
    if(limit<0||limit>262144){fail(s->p,"Invalid decoded audio frame size",0);return false;}
    if((unsigned)limit>*capacity) {
        float *larger=av_realloc(*samples,(size_t)limit*s->p->config.channels*sizeof(float));
        if(!larger){fail(s->p,"Not enough memory for decoded audio",0);return false;}*samples=larger;*capacity=(unsigned)limit;
    }
    uint8_t *out[]={(uint8_t*)*samples};
    int64_t delay=swr_get_delay(*resample,in_rate);
    int count=swr_convert(*resample,out,limit,(const uint8_t**)frame->extended_data,frame->nb_samples);
    if(count<0){fail(s->p,"Audio conversion failed",count);return false;}
    double pts=frame_time(s,frame,s->audio_index,*next);*next=pts+frame->nb_samples/(double)in_rate;
    return output_audio(s,*samples,(unsigned)count,pts-delay/(double)in_rate,serial);
}
static void wait_after_eof(VideoPlayer *p,unsigned serial,bool video) {
    pthread_mutex_lock(&p->lock);
    if(serial==p->serial){if(video)p->video_done=true;else p->audio_done=true;}
    pthread_cond_broadcast(&p->wake);
    while(!stopped(p)&&serial==p->serial)pthread_cond_wait(&p->wake,&p->lock);
    pthread_mutex_unlock(&p->lock);
}
static void *decode_audio(void *opaque) {
    Session *s=opaque;VideoPlayer *p=s->p;place_thread(1);
    AVFrame *frame=av_frame_alloc();SwrContext *resample=NULL;float *samples=NULL;unsigned capacity=0,seen=0;
    int rate=0,format=-1;int64_t layout=0;double next=0;
    if(!frame){fail(p,"Not enough memory for an audio frame",0);goto done;}
    while(!stopped(p)) {
        unsigned serial;bool eof;AVPacket *packet=pop_packet(p,&p->audio_q,&serial,&eof);
        if(stopped(p)){if(packet){av_packet_unref(packet);av_free(packet);}break;}
        if(serial!=seen){avcodec_flush_buffers(s->audio);swr_free(&resample);seen=serial;pthread_mutex_lock(&p->lock);next=p->seek_target;pthread_mutex_unlock(&p->lock);}
        if(!packet&&!eof)continue;
        AVPacket view;av_init_packet(&view);view.data=NULL;view.size=0;if(packet)view=*packet;
        do {
            int got=0;int consumed=avcodec_decode_audio4(s->audio,frame,&got,&view);
            if(consumed<0)break;
            if(got&&!resample_audio(s,frame,&resample,&rate,&format,&layout,&samples,&capacity,&next,serial))break;
            if(packet){if(consumed<=0)break;view.data+=consumed;view.size-=consumed;}
            else if(!got)break;
        } while(!stopped(p)&&(eof||view.size>0));
        if(packet){av_packet_unref(packet);av_free(packet);}
        if(eof)wait_after_eof(p,serial,false);
    }
done:
    av_free(samples);swr_free(&resample);av_frame_free(&frame);
    pthread_mutex_lock(&p->lock);p->audio_done=true;pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return NULL;
}
static bool open_session(Session *s,const char *path) {
    VideoPlayer *p=s->p;s->video_index=s->audio_index=-1;
    if(s->remote){
        char error[192];int status=0;s->open_deadline=seconds()+60;
        if(s->http_input.restart){
            xf_response *reply=malloc(sizeof *reply);
            bool ok=reply&&http_request(&s->http_input.endpoint,s->http_input.stop_request,reply,interrupt_http,s,error,sizeof error);
            if(ok){status=reply->status;ok=(status>=200&&status<300)||status==404;}
            if(reply){xf_wipe(reply,sizeof *reply);free(reply);}
            if(!ok){if(!stopped(p))fail(p,"The Jellyfin stream could not be restarted",0);return false;}
        }
        s->http=http_stream_open(&s->http_input.endpoint,s->http_input.request,true,interrupt_http,s,error,sizeof error,&status);
        pthread_mutex_lock(&p->lock);p->state.network_status=status;pthread_mutex_unlock(&p->lock);
        if(!s->http){if(!stopped(p))fail(p,error,0);return false;}
        s->file_size=-1;
    }else{
    s->file=fopen(path,"rb");
    if(!s->file){fail(p,"The selected video could not be opened",AVERROR(errno));return false;}
    if(fseeko(s->file,0,SEEK_END)||(s->file_size=(int64_t)ftello(s->file))<=0||fseeko(s->file,0,SEEK_SET)) {
        fail(p,"The video is empty or cannot be read",0);return false;
    }
    }
    /* FFmpeg2.8's nonseekable probe/rewind path assumes its standard32KiB
       refill size; a64KiB original buffer can assert after short TCP reads. */
    int buffer_bytes=s->remote?32*1024:64*1024;
    uint8_t *buffer=av_malloc((size_t)buffer_bytes);if(!buffer){fail(p,"Not enough memory for video input",0);return false;}
    s->io=avio_alloc_context(buffer,buffer_bytes,0,s,read_input,NULL,s->remote?NULL:seek_input);
    if(!s->io){av_free(buffer);fail(p,"Video input could not be created",0);return false;}
    if(s->remote)s->io->seekable=0;
    s->format=avformat_alloc_context();if(!s->format){fail(p,"Not enough memory for video metadata",0);return false;}
    s->format->pb=s->io;s->format->flags|=AVFMT_FLAG_CUSTOM_IO;
    s->format->interrupt_callback=(AVIOInterruptCB){interrupt_input,s};s->format->probesize=512*1024;s->format->max_analyze_duration=500000;
    s->open_deadline=seconds()+(s->remote?60:15);
    int result=avformat_open_input(&s->format,path,NULL,NULL);
    if(result<0){if(!stopped(p))fail(p,"Video container could not be opened",result);return false;}
    result=avformat_find_stream_info(s->format,NULL);
    if(result<0){if(!stopped(p))fail(p,"Video information could not be read",result);return false;}
    s->open_deadline=0;s->start=s->format->start_time==AV_NOPTS_VALUE?0:s->format->start_time/(double)AV_TIME_BASE;
    if(s->remote)s->start-=s->http_input.start_position;
    s->video_index=av_find_best_stream(s->format,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(s->video_index<0){fail(p,"This file has no supported video stream",0);return false;}
    AVCodecContext *header=s->format->streams[s->video_index]->codec;
    if(header->width>1920||header->height>1080){fail(p,"Video resolution exceeds 1920 x 1080",0);return false;}
    log_line(p,"video: input headers ready; opening decoder");
    s->video=open_decoder(s,s->video_index,true);if(!s->video){fail(p,"The video codec is not supported",0);return false;}
    AVRational rate=av_guess_frame_rate(s->format,s->format->streams[s->video_index],NULL);
    double fps=av_q2d(rate);s->frame_step=isfinite(fps)&&fps>=1&&fps<=240?1/fps:.04;
    s->audio_index=av_find_best_stream(s->format,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    if(p->config.audio_available&&s->audio_index>=0)s->audio=open_decoder(s,s->audio_index,false);
    if(p->config.audio_available&&s->audio_index>=0&&!s->audio){fail(p,"The audio codec is not supported",0);return false;}
    pthread_mutex_lock(&p->lock);
    if(stopped(p)){pthread_mutex_unlock(&p->lock);return false;}
    p->state.has_audio=s->audio!=NULL;p->state.channels=p->config.channels;p->state.duration=s->remote?s->http_input.duration:s->format->duration==AV_NOPTS_VALUE?0:s->format->duration/(double)AV_TIME_BASE;
    p->state.width=s->video->width;p->state.height=s->video->height;
    p->state.seeking=false;p->state.state=p->state.state==VIDEO_PAUSED?VIDEO_PAUSED:VIDEO_PLAYING;p->state.revision++;
    p->video_done=false;p->audio_done=s->audio==NULL;
    double duration=p->state.duration;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    log_line(p,"video: embedded decode %s %dx%d audio=%s duration=%.2f workers=%d cpu_flags=%X gpu_yuv=%u fps=%.3f",s->video->codec->name,s->video->width,s->video->height,s->audio?s->audio->codec->name:"none",duration,s->video->thread_count,av_get_cpu_flags(),p->config.gpu_yuv,1/s->frame_step);
    log_line(p,"video: Simple360 scheduling frame_threads=%u slots=%u workers=3,4,5,2 demux=1 video=2 audio=1 catchup=40/250ms",s->video->active_thread_type==FF_THREAD_FRAME,VIDEO_FRAME_SLOTS);
#ifdef BLADES_XBOX
    { extern int ff_xb_chroma_verdict,ff_xb_qpel_verdict;
      log_line(p,"video: Simple360 H264 DSP chroma=%d qpel=%d (0=vector self-test passed, -2=unused)",ff_xb_chroma_verdict,ff_xb_qpel_verdict); }
#endif
    return true;
}
static void seek_session(Session *s) {
    VideoPlayer *p=s->p;
    pthread_mutex_lock(&p->lock);double target=p->requested_seek;__atomic_store_n(&p->seek_pending,false,__ATOMIC_RELEASE);pthread_mutex_unlock(&p->lock);
    int64_t time=(int64_t)((target+s->start)*AV_TIME_BASE);
    int result=av_seek_frame(s->format,-1,time,AVSEEK_FLAG_BACKWARD);
    if(result<0){log_line(p,"video: seek failed (%d)",result);return;}
    pthread_mutex_lock(&p->lock);
    clear_packets(&p->video_q);clear_packets(&p->audio_q);p->serial++;p->seek_target=target;p->demux_eof=false;
    p->video_done=false;p->audio_done=s->audio==NULL;reset_output(p,target);p->state.position=target;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    log_line(p,"video: seek to %.2f",target);
}
static void close_session(Session *s) {
    VideoPlayer *p=s->p;
    pthread_mutex_lock(&p->lock);set_stop(p,true);pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
    if(s->video_thread_started)pthread_join(s->video_thread,NULL);
    if(s->audio_thread_started)pthread_join(s->audio_thread,NULL);
    if(s->video)avcodec_close(s->video);
    if(s->audio)avcodec_close(s->audio);
    if(s->format)avformat_close_input(&s->format);
    if(s->io){av_freep(&s->io->buffer);av_freep(&s->io);}
    if(s->file)fclose(s->file);
    http_stream_close(s->http);xf_wipe(&s->http_input,sizeof s->http_input);
    pthread_mutex_lock(&p->lock);clear_packets(&p->video_q);clear_packets(&p->audio_q);p->audio_count=0;
    for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)if(p->pictures[i].state!=FRAME_HELD)p->pictures[i].state=FRAME_FREE;
    p->running=false;
    if(p->state.state==VIDEO_STOPPING)p->state.state=VIDEO_IDLE;
    p->state.revision++;pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
}
static void *coordinator(void *opaque) {
    VideoPlayer *p=opaque;place_thread(1);
    while(true) {
        pthread_mutex_lock(&p->lock);
        while(!p->pending&&!p->quit)pthread_cond_wait(&p->wake,&p->lock);
        if(p->quit){pthread_mutex_unlock(&p->lock);break;}
        char path[1024];memcpy(path,p->path,sizeof path);p->pending=false;p->running=true;p->telemetry_time=seconds();p->telemetry_decoded=p->telemetry_presented=0;set_stop(p,false);
        __atomic_store_n(&p->seek_pending,false,__ATOMIC_RELEASE);p->serial++;
        Session session={.p=p,.remote=p->pending_http};if(session.remote)session.http_input=p->http_input;
        p->seek_target=session.remote?session.http_input.start_position:0;p->demux_eof=false;reset_output(p,p->seek_target);
        pthread_mutex_unlock(&p->lock);
        if(open_session(&session,path)&&!stopped(p)) {
            pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,512*1024);
            if(!pthread_create(&session.video_thread,&attr,decode_video,&session))session.video_thread_started=true;else fail(p,"Video decoder thread could not start",0);
            if(!stopped(p)&&session.audio){if(!pthread_create(&session.audio_thread,&attr,decode_audio,&session))session.audio_thread_started=true;else fail(p,"Audio decoder thread could not start",0);}
            pthread_attr_destroy(&attr);
            while(!stopped(p)) {
                if(pending_seek(p)){seek_session(&session);continue;}
                pthread_mutex_lock(&p->lock);
                while(p->demux_eof&&!stopped(p)&&!pending_seek(p))pthread_cond_wait(&p->wake,&p->lock);
                pthread_mutex_unlock(&p->lock);
                if(stopped(p)||pending_seek(p))continue;
                AVPacket packet;av_init_packet(&packet);
                int result=av_read_frame(session.format,&packet);
                if(result<0) {
                    if(session.remote&&!stopped(p)&&(result!=AVERROR_EOF||http_stream_error(session.http)[0])){fail(p,http_stream_error(session.http),result);continue;}
                    pthread_mutex_lock(&p->lock);p->demux_eof=true;p->video_q.eof=true;p->audio_q.eof=true;pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);continue;
                }
                bool queued=true;
                if(packet.stream_index==session.video_index)queued=push_packet(p,&p->video_q,&packet,VIDEO_PACKET_BYTES);
                else if(session.audio&&packet.stream_index==session.audio_index)queued=push_packet(p,&p->audio_q,&packet,AUDIO_PACKET_BYTES);
                av_packet_unref(&packet);
                if(!queued&&!stopped(p)&&!pending_seek(p))fail(p,"Not enough memory for video packets",0);
            }
        }
        close_session(&session);
    }
    return NULL;
}
VideoPlayer *video_create(const VideoConfig *config) {
    VideoPlayer *p=calloc(1,sizeof *p);if(!p)return NULL;
    p->config=config?*config:(VideoConfig){.channels=2};
if(p->config.channels!=6)p->config.channels=2;
    p->audio=malloc((size_t)AUDIO_FRAMES*p->config.channels*sizeof(float));
    for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)p->pictures[i].pixels=av_malloc((size_t)VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT*4);
    bool memory=p->audio!=NULL;for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)memory=memory&&p->pictures[i].pixels;
    if(!memory){for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)av_free(p->pictures[i].pixels);free(p->audio);free(p);return NULL;}
    pthread_mutex_init(&p->lock,NULL);pthread_cond_init(&p->wake,NULL);pthread_once(&ffmpeg_once,init_ffmpeg);
    pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,1024*1024);
    int result=pthread_create(&p->worker,&attr,coordinator,p);pthread_attr_destroy(&attr);
    if(result){pthread_cond_destroy(&p->wake);pthread_mutex_destroy(&p->lock);for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)av_free(p->pictures[i].pixels);free(p->audio);free(p);return NULL;}
    return p;
}
bool video_play(VideoPlayer *p,const char *path) {
    if(!p||!path||!path[0]||strnlen(path,sizeof p->path)==sizeof p->path)return false;
    pthread_mutex_lock(&p->lock);p->pending_http=false;xf_wipe(&p->http_input,sizeof p->http_input);snprintf(p->path,sizeof p->path,"%s",path);p->pending=true;set_stop(p,true);
    unsigned revision=p->state.revision+1;p->state=(VideoSnapshot){.state=VIDEO_OPENING,.revision=revision,.channels=p->config.channels};
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return true;
}
bool video_play_http(VideoPlayer *p,const VideoHttpInput *input){
    if(!p||!input||!input->request[0]||strnlen(input->request,sizeof input->request)==sizeof input->request||!isfinite(input->start_position)||input->start_position<0||input->start_position>1e10||!isfinite(input->duration)||input->duration<0)return false;
    pthread_mutex_lock(&p->lock);p->pending_http=true;p->http_input=*input;
    snprintf(p->path,sizeof p->path,"Jellyfin stream");p->pending=true;set_stop(p,true);
    unsigned revision=p->state.revision+1;p->state=(VideoSnapshot){.state=VIDEO_OPENING,.revision=revision,.channels=p->config.channels,.network=true,.position=input->start_position,.duration=input->duration};
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return true;
}
void video_stop(VideoPlayer *p) {
    if(!p)return;
    pthread_mutex_lock(&p->lock);p->pending=false;set_stop(p,true);p->state.state=p->running?VIDEO_STOPPING:VIDEO_IDLE;p->state.revision++;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
}
void video_pause(VideoPlayer *p,bool paused) {
    if(!p)return;
    pthread_mutex_lock(&p->lock);
    if(video_state_active(p->state.state)) {
        double position=clock_locked(p);
        if(paused&&p->state.state!=VIDEO_PAUSED){p->state.position=position;p->state.state=VIDEO_PAUSED;p->state.revision++;}
        else if(!paused&&p->state.state==VIDEO_PAUSED){p->wall_base=position;p->wall_start=seconds();p->state.state=VIDEO_PLAYING;p->state.revision++;}
    }
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
}
/* Jellyfin hashes media, User-Agent, device and PlaySessionId into its
   progressive cache filename, but not StartTimeTicks. Its DELETE returns before
   delayed file removal. Give each restart a unique request identity while
   preserving the authenticated device and logical playback/reporting session. */
static bool stream_position(VideoHttpInput *input,double target,uint64_t generation){
    VideoHttpInput updated=*input;
    char *value=strstr(updated.request,"&StartTimeTicks=");if(!value)return false;
    value+=strlen("&StartTimeTicks=");char *end=value;
    while(*end>='0'&&*end<='9')end++;
    if(end==value||(*end!='&'&&*end!=' '))return false;
    char ticks[32];int n=snprintf(ticks,sizeof ticks,"%llu",(unsigned long long)(target*10000000));
    size_t tail=strlen(end)+1,head=(size_t)(value-updated.request);
    if(n<0||head+(size_t)n+tail>sizeof updated.request)return false;
    memmove(value+n,end,tail);memcpy(value,ticks,(size_t)n);
    char agent[96];n=snprintf(agent,sizeof agent,"User-Agent: Blades/0.53 stream/%llu\r\n",(unsigned long long)generation);
    char *line=strstr(updated.request,"\r\nUser-Agent:");
    if(line){line+=2;end=strstr(line,"\r\n");if(!end)return false;end+=2;}
    else {line=strstr(updated.request,"\r\n\r\n");if(!line)return false;line+=2;end=line;}
    head=(size_t)(line-updated.request);tail=strlen(end)+1;
    if(n<0||head+(size_t)n+tail>sizeof updated.request)return false;
    memmove(line+n,end,tail);memcpy(line,agent,(size_t)n);
    updated.start_position=target;updated.restart=true;*input=updated;return true;
}
void video_seek(VideoPlayer *p,double relative) {
    if(!p||!isfinite(relative))return;
    bool requested=false;double from=0,to=0;uint64_t generation=0;
    pthread_mutex_lock(&p->lock);
    if(p->state.state==VIDEO_PLAYING||p->state.state==VIDEO_PAUSED||(p->state.network&&p->state.state==VIDEO_OPENING)) {
        double base=p->state.seeking?p->state.position:pending_seek(p)?p->requested_seek:clock_locked(p);
        double target=fmax(0,base+relative);
        if(p->state.duration>0)target=fmin(target,fmax(0,p->state.duration-(p->state.network?.5:.1)));
        if(p->state.network){
            uint64_t next=p->http_generation+1;
            if(next&&p->state.duration>0&&p->http_input.stop_request[0]&&stream_position(&p->http_input,target,next)){
                bool paused=p->state.state==VIDEO_PAUSED;
                p->pending=true;set_stop(p,true);unsigned revision=p->state.revision+1;p->http_generation=next;
                p->state=(VideoSnapshot){.state=paused?VIDEO_PAUSED:VIDEO_OPENING,.revision=revision,.network=true,.seeking=true,
                    .position=target,.duration=p->http_input.duration,.channels=p->config.channels};
                requested=true;from=base;to=target;generation=next;
            }
        }else{p->requested_seek=target;__atomic_store_n(&p->seek_pending,true,__ATOMIC_RELEASE);}
        pthread_cond_broadcast(&p->wake);
    }
    pthread_mutex_unlock(&p->lock);
    if(requested)log_line(p,"video: network seek %.3f -> %.3f generation=%llu",from,to,(unsigned long long)generation);
}
void video_poll(VideoPlayer *p,VideoSnapshot *out) {
    if(!out)return;
    if(!p){*out=(VideoSnapshot){.state=VIDEO_FAILED,.error="Video playback is unavailable."};return;}
    pthread_mutex_lock(&p->lock);
    if(video_state_active(p->state.state))p->state.position=fmax(0,clock_locked(p));
    if(p->state.state==VIDEO_PLAYING&&p->demux_eof&&p->video_done&&p->audio_done&&!p->audio_count&&!pending_seek(p)) {
        bool queued=false;for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)queued|=p->pictures[i].state==FRAME_READY||p->pictures[i].state==FRAME_WRITING;
        if(!queued&&p->state.position+.005>=fmax(p->last_video_end,p->last_audio_end)){p->state.state=VIDEO_ENDED;p->state.revision++;set_stop(p,true);pthread_cond_broadcast(&p->wake);}
    }
    double elapsed=seconds()-p->telemetry_time;bool report=p->telemetry_time>0&&
        (p->state.state==VIDEO_PLAYING||p->state.state==VIDEO_PAUSED)&&elapsed>=5;
    double decoded_fps=0,presented_fps=0;
    if(report){decoded_fps=(p->state.decoded-p->telemetry_decoded)/elapsed;presented_fps=(p->state.presented-p->telemetry_presented)/elapsed;p->telemetry_time=seconds();p->telemetry_decoded=p->state.decoded;p->telemetry_presented=p->state.presented;}
    *out=p->state;pthread_mutex_unlock(&p->lock);
    if(report)log_line(p,"video-perf: state=%u decoded_fps=%.2f presented_fps=%.2f decode_call_ms=%.2f prepare_ms=%.2f dropped=%u late=%u lag_ms=%.1f late_presented=%u audio_preroll=%u catchup=%u catchup_episodes=%u audio_underrun_frames=%llu audio_gap_frames=%llu audio_overlap_frames=%llu",out->state,decoded_fps,presented_fps,out->decode_ms,out->prepare_ms,out->dropped,out->late_frames,out->video_lag_ms,out->late_presented,out->audio_preroll,out->catchup_level,out->catchup_episodes,(unsigned long long)out->audio_underrun_frames,(unsigned long long)out->audio_gap_frames,(unsigned long long)out->audio_overlap_frames);
}
bool video_take_picture(VideoPlayer *p,VideoPicture *out) {
    if(!p||!out)return false;
    pthread_mutex_lock(&p->lock);
    if(stopped(p)||p->state.seeking){pthread_mutex_unlock(&p->lock);return false;}
    double clock=clock_locked(p);int chosen=-1;
    bool bootstrap=!p->clock_started&&!p->audio_started;
    for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++) {
        PictureSlot *f=&p->pictures[i];
        if(f->state==FRAME_READY&&f->serial==p->serial&&
            (bootstrap||f->pts<=clock+.005)&&(chosen<0||
                (bootstrap?f->pts<p->pictures[chosen].pts:f->pts>p->pictures[chosen].pts)))chosen=(int)i;
    }
    if(chosen>=0) {
        PictureSlot *f=&p->pictures[chosen];
        for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)if(i!=(unsigned)chosen&&p->pictures[i].state==FRAME_READY&&p->pictures[i].serial==p->serial&&p->pictures[i].pts<=f->pts){p->pictures[i].state=FRAME_FREE;p->state.dropped++;}
        p->state.video_lag_ms=fmax(0,clock-f->pts)*1000;
        if(p->clock_started&&p->state.video_lag_ms>80)p->state.late_presented++;
        f->state=FRAME_HELD;*out=(VideoPicture){f->pixels,f->width,f->height,(unsigned)chosen,f->revision,f->pts,f->aspect,f->format};p->state.presented++;
        p->last_picture_wall=seconds();
        if(!p->clock_started){p->clock_started=true;p->wall_base=f->pts;p->wall_start=seconds();}
        pthread_cond_broadcast(&p->wake);
    }
    bool first=chosen>=0&&p->state.presented==1;
    pthread_mutex_unlock(&p->lock);
    if(first)log_line(p,"video: first frame delivered to renderer format=%u size=%ux%u",out->format,out->width,out->height);
    return chosen>=0;
}
void video_release_picture(VideoPlayer *p,unsigned slot,unsigned revision) {
    if(!p||slot>=VIDEO_FRAME_SLOTS)return;
    pthread_mutex_lock(&p->lock);PictureSlot *f=p->pictures+slot;
    if(f->state==FRAME_HELD&&f->revision==revision)f->state=FRAME_FREE;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);
}
bool video_audio_render(VideoPlayer *p,float *out,unsigned frames) {
    if(!p||!out)return false;
    unsigned channels=p->config.channels;memset(out,0,(size_t)frames*channels*sizeof(float));
    pthread_mutex_lock(&p->lock);bool active=video_state_active(p->state.state)||p->audio_tail;unsigned n=0;
    unsigned ready=0;for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)ready+=p->pictures[i].state==FRAME_READY&&p->pictures[i].serial==p->serial;
    /* Normal startup waits for the first displayed picture. If demux cannot
       reach the first video packet until an audio-only lead is consumed,
       break that specific bounded-queue dependency rather than deadlocking. */
    if(!p->clock_started&&!ready&&p->demux_waiting_audio&&p->video_needs_packet)p->state.audio_preroll=true;
    bool primed=p->audio_started||p->state.audio_preroll||
        (p->clock_started&&(ready>=VIDEO_FRAME_SLOTS-1||p->video_done||seconds()-p->wall_start>2));
    if(p->state.state==VIDEO_PLAYING&&!stopped(p)&&primed) {
        n=p->audio_count;if(n>frames)n=frames;
        if(p->audio_started&&!p->audio_done)p->state.audio_underrun_frames+=frames-n;
        for(unsigned i=0;i<n;i++) {
            p->audio_gain=fminf(1,p->audio_gain+1.f/256);
            for(unsigned c=0;c<channels;c++) {
                float sample=p->audio[(size_t)((p->audio_head+i)%AUDIO_FRAMES)*channels+c];
                out[(size_t)i*channels+c]=p->audio_last[c]=isfinite(sample)?sample*p->audio_gain:0;
            }
        }
        if(n)p->audio_tail=256;
        p->audio_head=(p->audio_head+n)%AUDIO_FRAMES;p->audio_count-=n;p->audio_consumed+=n;
        if(n)p->audio_started=true;
    }
    /* A short sample tail avoids a hard waveform edge on pause/stop/underrun.
       The next PCM data ramps in again, without involving the decoder here. */
    for(unsigned i=n;i<frames;i++) {
        p->audio_gain=0;
        if(p->audio_tail){p->audio_tail--;for(unsigned c=0;c<channels;c++)out[(size_t)i*channels+c]=p->audio_last[c]*(p->audio_tail/256.f);}
    }
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);return active;
}
unsigned video_audio_channels(const VideoPlayer *p){return p?p->config.channels:2;}
void video_close(VideoPlayer *p) {
    if(!p)return;
    pthread_mutex_lock(&p->lock);__atomic_store_n(&p->quit,true,__ATOMIC_RELEASE);set_stop(p,true);p->pending=false;
    pthread_cond_broadcast(&p->wake);pthread_mutex_unlock(&p->lock);pthread_join(p->worker,NULL);
    pthread_cond_destroy(&p->wake);pthread_mutex_destroy(&p->lock);
    for(unsigned i=0;i<VIDEO_FRAME_SLOTS;i++)av_free(p->pictures[i].pixels);
    free(p->audio);free(p);
}
