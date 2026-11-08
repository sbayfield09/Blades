#ifndef BLADES_DISC_H
#define BLADES_DISC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { DISC_TITLE_BYTES=192 };
typedef enum { DISC_EMPTY,DISC_OPENING,DISC_OPEN,DISC_CLOSING,DISC_READING,
    DISC_READY,DISC_UNSUPPORTED,DISC_ERROR,DISC_UNAVAILABLE } DiscState;
typedef enum { DISC_NONE,DISC_EJECT,DISC_CLOSE,DISC_LAUNCH } DiscCommand;
/* XAM's public tray enumeration, not the unrelated raw SMC status bits. */
typedef enum { TRAY_EMPTY,TRAY_CLOSING,TRAY_OPEN,TRAY_OPENING,TRAY_CLOSED,TRAY_ERROR } DiscTray;
typedef enum { DISC_MEDIA_PENDING,DISC_MEDIA_NONE,DISC_MEDIA_GAME,DISC_MEDIA_OTHER,DISC_MEDIA_UNKNOWN,
    DISC_MEDIA_AUDIO_CD,DISC_MEDIA_DVD_VIDEO,DISC_MEDIA_DATA } DiscMedia;
typedef struct { DiscState state;uint32_t generation,title_id;char title[DISC_TITLE_BYTES];DiscMedia media; } DiscStatus;
typedef struct {
    DiscStatus status;DiscTray tray;DiscCommand pending;float pending_seconds,reading_seconds;
    bool observed;
    uint32_t last_media;
    unsigned media_samples;
} DiscDrive;
void disc_init(DiscDrive *drive);
void disc_observe(DiscDrive *drive,DiscTray tray,float dt);
bool disc_request(DiscDrive *drive,DiscCommand command,uint32_t generation);
bool disc_identified(DiscDrive *drive,uint32_t generation,uint32_t title_id,const char *title,bool playable);
bool disc_begin_reading(DiscDrive *drive,uint32_t generation);
/* XAM's return word can be a title ID, not HRESULT success. Classify its
   full media word: FFFFFFFE is pending, never an unsupported media type. */
DiscMedia disc_media_kind(uint32_t returned,uint32_t raw_media);
DiscMedia disc_media_update(DiscDrive *drive,uint32_t generation,uint32_t returned,uint32_t raw_media);
bool disc_can_eject(const DiscStatus *status);
DiscCommand disc_select(const DiscStatus *status);
void disc_label(const DiscStatus *status,char *label,size_t size);
void disc_title_copy(char *out,size_t size,const char *title);
unsigned disc_reading_frame(float seconds);
/* Retail dashboard launch-data ABI: version, command, user, reserved. */
bool disc_dashboard_data(DiscMedia media,unsigned user,uint8_t data[1020]);
/* xsdk_launch resolves device paths and creates the DOS alias XAM needs.
   Flash: is not guaranteed to be mounted in a standalone title. */
const char *disc_launch_path(DiscMedia media);
#endif
