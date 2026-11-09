#ifndef BLADES_UPDATER_H
#define BLADES_UPDATER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifndef BLADES_VERSION
#define BLADES_VERSION "0.52.0"
#endif
#define BLADES_REPOSITORY "sbayfield09/Blades-reimagined"
#define BLADES_UPDATE_PATH "/" BLADES_REPOSITORY "/main/dist/"
typedef enum { UPDATE_IDLE,UPDATE_CHECKING,UPDATE_AVAILABLE,UPDATE_CURRENT,UPDATE_DOWNLOADING,UPDATE_READY,UPDATE_ERROR,UPDATE_CANCELLED,UPDATE_PREPARING } UpdateState;
typedef struct {
    UpdateState state;unsigned revision;uint64_t received,total;
    char version[24],filename[96],message[192],saved[256];
} UpdateSnapshot;
typedef struct Updater Updater;
typedef struct {char version[24],filename[96],sha256[65];uint64_t bytes;} UpdateManifest;
bool update_manifest_parse(const char *,size_t,UpdateManifest *);
int update_version_compare(const char *,const char *);
Updater *updater_open(const char *directory,void (*worker_start)(void *),void *);
bool updater_install_to(Updater *,const char *root);
void updater_install_failed(Updater *,const char *message);
void updater_poll(Updater *,UpdateSnapshot *);
bool updater_check(Updater *);
bool updater_download(Updater *);
void updater_cancel(Updater *);
void updater_close(Updater *);
#endif
