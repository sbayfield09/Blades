#ifndef BLADES_UPDATE_INSTALL_H
#define BLADES_UPDATE_INSTALL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define UPDATE_INSTALL_FILES 64
#define UPDATE_INSTALL_NAME 160
#define UPDATE_INSTALL_WORK ".blades-update"
#define UPDATE_INSTALL_HELPER UPDATE_INSTALL_WORK "/installer.xex"
typedef struct {char path[UPDATE_INSTALL_NAME],hash[65],old_hash[65];uint64_t size,old_size;bool existed;} UpdateInstallFile;
typedef struct {unsigned count;UpdateInstallFile files[UPDATE_INSTALL_FILES];} UpdateInstallPlan;
typedef bool (*InstallCancelled)(void *);
typedef void (*InstallProgress)(void *,unsigned,unsigned);
bool update_install_allowed(const char *path);
bool update_install_pending(const char *root);
bool update_install_stage(const char *root,const char *zip,InstallCancelled cancel,InstallProgress progress,void *context,char *error,size_t bytes);
/* 1 installed, 0 recovered previous version, -1 failed (journal retained if needed). */
int update_install_apply(const char *root,char *error,size_t bytes);
bool update_install_save_plan(const char *root,const UpdateInstallPlan *plan,const char *name);
bool update_install_path(char *out,size_t bytes,const char *root,const char *area,const char *relative);
bool update_install_parents(const char *path);
bool update_install_hash(const char *path,uint64_t *bytes,char hash[65]);
bool update_install_copy(const char *source,const char *destination);
bool update_install_reset(const char *root);
#endif
