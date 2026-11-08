#ifndef BLADES_JELLYFIN_H
#define BLADES_JELLYFIN_H
#include "browse.h"
#include "video.h"
#include <stdbool.h>
#include <stdint.h>
enum { JELLY_TEXT=129,JELLY_LABEL_BYTES=352 };
enum { JELLY_ART_WIDTH=288,JELLY_ART_HEIGHT=176 };
typedef struct {
    char item[65];unsigned revision,width,height;
    unsigned char pixels[JELLY_ART_WIDTH*JELLY_ART_HEIGHT*4];
} JellyArtwork;
typedef enum { JELLY_OFF,JELLY_CONNECTING,JELLY_CONNECTED,JELLY_ERROR } JellyState;
typedef struct {
    JellyState state;unsigned revision;bool busy,save_error,password_set,loaded;
    char server[JELLY_TEXT],username[JELLY_TEXT],server_name[JELLY_TEXT],status[192];
    xf_page page;char labels[XF_PAGE_SIZE][JELLY_LABEL_BYTES],title[97];unsigned depth,start;
    JellyArtwork artwork;
} JellySnapshot;
typedef enum { JELLY_NONE,JELLY_CONNECT,JELLY_DISCONNECT,JELLY_REMOVE,JELLY_ROOT,
    JELLY_REFRESH,JELLY_OPEN,JELLY_BACK,JELLY_NEXT,JELLY_PREVIOUS,JELLY_PLAY,JELLY_EDIT,JELLY_STOP,JELLY_CANCEL,JELLY_PREVIEW } JellyCommandType;
typedef struct {JellyCommandType type;unsigned row,field;char text[JELLY_TEXT];double position;} JellyCommand;
typedef struct {void *context;void (*worker_start)(void *);} JellyHooks;
typedef struct JellyService JellyService;
/* Snapshots never contain a password, bearer token or authenticated URL.
 * All HTTP and settings I/O runs on the worker. Front is valid until poll. */
JellyService *jellyfin_open(const char *config,const JellyHooks *);
const JellySnapshot *jellyfin_poll(JellyService *);
bool jellyfin_request(JellyService *,const JellyCommand *);
/* Low-priority, debounced selection artwork; NULL cancels it. Never marks UI busy. */
void jellyfin_select(JellyService *,const char *item_id);
void jellyfin_cancel(JellyService *);
bool jellyfin_take_playback(JellyService *,VideoHttpInput *);
void jellyfin_progress(JellyService *,const VideoSnapshot *);
void jellyfin_finish(JellyService *,double position);
void jellyfin_close(JellyService *);
bool jellyfin_playable(const xf_item *);
const char *jellyfin_status(const JellySnapshot *);
void jellyfin_item_label(const xf_item *,enum xf_view,char *,size_t);
void jellyfin_runtime(uint64_t ticks,char *,size_t);
#endif
