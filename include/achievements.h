#ifndef BLADES_ACHIEVEMENTS_H
#define BLADES_ACHIEVEMENTS_H
#include "profile.h"
enum { ACH_TITLE_LIMIT=1024,ACH_ITEM_LIMIT=512,ACH_GRID=18,ACH_TILES=21 };
typedef struct {uint32_t id,possible,earned,score_possible,score_earned;uint64_t played;char name[128];} AchievementTitle;
typedef struct {uint32_t id,image,score,flags;uint64_t unlocked;bool earned,secret;char name[128],description[512];} AchievementItem;
typedef struct {bool valid;unsigned index;uint32_t title;uint8_t pixels[PROFILE_PICTURE_BYTES];} AchievementTile;
typedef struct {
    unsigned user,revision;uint64_t xuid;uint32_t title_id;
    unsigned title_count,item_count;bool busy,error,truncated;unsigned error_code;
    AchievementTitle titles[ACH_TITLE_LIMIT];AchievementItem items[ACH_ITEM_LIMIT];
    AchievementTile tiles[ACH_TILES];
    uint16_t order[ACH_ITEM_LIMIT],score_order[ACH_ITEM_LIMIT];bool ordered;
} AchievementSnapshot;
typedef enum { ACH_NONE,ACH_LOAD,ACH_GAME,ACH_IMAGES,ACH_GUIDE_GAME,ACH_GUIDE_DETAIL } AchievementAction;
typedef struct {AchievementAction type;unsigned user;uint64_t xuid;uint32_t title_id,achievement_id;unsigned first,game_first;} AchievementRequest;
typedef bool (*AchievementCancelled)(void *);
typedef struct {
    void *context;
    bool (*identity)(void *,unsigned,uint64_t);
    unsigned (*load)(void *,const AchievementRequest *,AchievementSnapshot *,AchievementCancelled,void *);
    void (*worker_start)(void *);
} AchievementHooks;
typedef struct AchievementService AchievementService;
AchievementService *achievements_open(const AchievementHooks *hooks);
void achievements_set_user(AchievementService *service,unsigned user,uint64_t xuid);
bool achievements_request(AchievementService *service,AchievementRequest request);
void achievements_cancel(AchievementService *service);
const AchievementSnapshot *achievements_poll(AchievementService *service);
void achievements_close(AchievementService *service);
bool achievement_title_record(const uint8_t *data,size_t bytes,AchievementTitle *title);
bool achievement_item_record(const uint8_t *data,size_t bytes,uint32_t base,size_t offset,AchievementItem *item);
void achievement_prepare(AchievementSnapshot *snapshot);
unsigned achievement_grid_count(const AchievementSnapshot *snapshot);
unsigned achievement_view_index(const AchievementSnapshot *snapshot,unsigned row,bool score);
const AchievementItem *achievement_grid_item(const AchievementSnapshot *snapshot,unsigned row,unsigned *index);
void achievement_date(uint64_t filetime,char date[16]);
#endif
