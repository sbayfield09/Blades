#ifndef BLADES_PROFILE_H
#define BLADES_PROFILE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { PROFILE_USERS=4,PROFILE_PICTURE_SIDE=64,PROFILE_PICTURE_BYTES=64*64*4 };
enum { PROFILE_GAMES=1,PROFILE_SCORE=2,PROFILE_ACHIEVEMENTS=4 };
typedef enum { PROFILE_UNAVAILABLE,PROFILE_SIGNED_OUT,PROFILE_LOCAL,PROFILE_LIVE } ProfileState;
typedef struct {unsigned user;ProfileState state;uint64_t xuid;char name[64];bool guest;} ProfileIdentity;
typedef struct {
    ProfileIdentity identity;
    uint32_t games,score,achievements,known;
    unsigned available_profiles;bool profiles_known;
    bool loading,busy,has_picture;
    unsigned revision,picture_revision,error_serial,error_code;
    char error[160];uint8_t picture[PROFILE_PICTURE_BYTES];
} ProfileSnapshot;
typedef enum { PROFILE_NONE,PROFILE_SIGN_IN,PROFILE_SIGN_OUT,PROFILE_VIEW } ProfileCommandType;
typedef struct {ProfileCommandType type;unsigned user;uint64_t xuid;} ProfileCommand;
typedef struct {
    void *context;
    bool (*identity)(void *,unsigned,ProfileIdentity *);
    void (*details)(void *,ProfileSnapshot *);
    bool (*profiles)(void *,unsigned *);
    unsigned (*sign_out)(void *,unsigned,uint64_t);
    void (*worker_start)(void *);
} ProfileHooks;
typedef struct ProfileService ProfileService;
bool profile_signed_in(const ProfileSnapshot *profile);
bool profile_count_packages(const char *const *devices,size_t count,unsigned *total);
ProfileService *profile_open(const ProfileHooks *hooks);
void profile_set_user(ProfileService *service,unsigned user);
void profile_refresh(ProfileService *service);
const ProfileSnapshot *profile_poll(ProfileService *service);
bool profile_request(ProfileService *service,ProfileCommand command);
void profile_close(ProfileService *service);
/* Bounded, endian-independent readers for the XAM result and picture buffers. */
bool profile_settings(const uint8_t *data,size_t size,uint32_t address,ProfileSnapshot *profile,char picture_key[64]);
bool profile_picture_key(const char *key,uint32_t *title,uint64_t *image);
bool profile_picture_argb(const uint8_t *data,size_t bytes,size_t pitch,uint8_t rgba[PROFILE_PICTURE_BYTES]);
bool profile_signout_plan(const ProfileIdentity users[4],unsigned user,uint64_t expected,uint64_t xuids[4]);
#endif
