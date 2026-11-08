#ifndef BLADES_LIBRARY_H
#define BLADES_LIBRARY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "disc.h"
enum { LIBRARY_PATH=1024, LIBRARY_ROOTS=32, LIBRARY_VISIBLE=9 };
enum { LIBRARY_DEPTH_DEFAULT=3,LIBRARY_DEPTH_MAX=32 };
#define LIBRARY_DEPTH_ALL UINT32_MAX
typedef enum { LIB_GAMES,LIB_APPS,LIB_HOMEBREW,LIB_ARCADE,LIB_DEMOS,LIB_ALL,LIB_MUSIC,LIB_PICTURES,LIB_VIDEO,LIB_MEDIA_FOLDER } LibraryCategory;
typedef enum { LIB_XEX,LIB_PACKAGE,LIB_MEDIA } LibraryKind;
typedef struct {
    char name[DISC_TITLE_BYTES],path[LIBRARY_PATH];
    uint32_t title_id,content_type;
    LibraryCategory category;LibraryKind kind;
} LibraryEntry;
typedef struct { char path[LIBRARY_PATH];LibraryCategory category;unsigned max_depth; } LibraryRoot;
typedef enum { LIB_SCAN_FOLDER,LIB_SCAN_ENTRY,LIB_SCAN_FILE } LibraryScanKind;
typedef struct {
    char path[LIBRARY_PATH];LibraryScanKind kind;
    unsigned depth,folders,files;size_t found;
    bool scanning,stopping;
} LibraryProgress;
typedef struct { char name[256],path[LIBRARY_PATH];bool executable,is_file; } LibraryFolder;
typedef struct {
    LibraryRoot roots[LIBRARY_ROOTS];size_t root_count;
    LibraryEntry *entries;size_t count;
    LibraryFolder *folders;size_t folder_count;
    char browse_path[LIBRARY_PATH],status[192],launch_path[LIBRARY_PATH];
    unsigned revision,launch_serial,launch_epoch;bool busy,launch_ready;
    LibraryCategory launch_category;LibraryKind launch_kind;
    unsigned errors;bool status_error;
    LibraryProgress progress;
} LibrarySnapshot;
typedef enum { LIB_CMD_NONE,LIB_CMD_SCAN,LIB_CMD_BROWSE,LIB_CMD_ADD,LIB_CMD_REMOVE,LIB_CMD_LAUNCH,LIB_CMD_DEPTH,LIB_CMD_STOP,LIB_CMD_FILES } LibraryCommandType;
typedef struct {
    LibraryCommandType type;char path[LIBRARY_PATH];LibraryCategory category;
    size_t root;LibraryEntry entry;unsigned launch_epoch,max_depth;
    bool direct_launch;
} LibraryCommand;
typedef struct {
    void *context;
    bool (*xex_title)(void *,FILE *,uint32_t *,char *);
    bool (*prepare)(void *,const LibraryEntry *,char *,char *,size_t);
    void (*worker_start)(void *);
    void *progress_context;
    void (*progress)(void *,const LibraryProgress *);
} LibraryHooks;
typedef struct LibraryService LibraryService;
/* All I/O, including folder browsing, settings and launch preparation, runs on
   the worker. The render thread only exchanges owned immutable snapshots. */
LibraryService *library_open(const char *config,const char *self,const LibraryHooks *hooks);
const LibrarySnapshot *library_poll(LibraryService *service);
bool library_request(LibraryService *service,const LibraryCommand *command);
void library_cancel_launch(LibraryService *service);
bool library_take_launch(LibraryService *service,char path[LIBRARY_PATH]);
bool library_take_launch_ex(LibraryService *service,char path[LIBRARY_PATH],LibraryCategory *category,LibraryKind *kind);
void library_close(LibraryService *service);
const char *library_category_name(LibraryCategory category);
size_t library_count(const LibrarySnapshot *s,LibraryCategory filter);
const LibraryEntry *library_entry(const LibrarySnapshot *s,LibraryCategory filter,size_t row);
/* Depth 0 checks files in the selected folder, 1 includes its children, etc.
   The heap worklist also supports an explicit unlimited setting. */
bool library_scan(LibrarySnapshot *s,const LibraryHooks *hooks,const char *self,const unsigned *stop);
bool library_read_config(const char *file,LibrarySnapshot *s);
bool library_write_config(const char *file,const LibrarySnapshot *s);
bool library_read_entry(FILE *file,const char *path,LibraryCategory category,const LibraryHooks *hooks,LibraryEntry *entry);
bool library_xex_header(FILE *file,uint32_t *title_id);
/* One extension only: name.xex, ignoring case. Directory dots are permitted. */
bool library_plain_xex(const char *path);
bool library_media_category(LibraryCategory category);
bool library_media_supported(const char *path,LibraryCategory category);
void library_snapshot_free(LibrarySnapshot *s);
bool library_join(char out[LIBRARY_PATH],const char *parent,const char *name);
void library_parent(char path[LIBRARY_PATH]);
void library_depth_label(unsigned depth,char *label,size_t size);
#endif
