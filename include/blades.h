#ifndef BLADES_H
#define BLADES_H
#include <stdbool.h>
#include <stdint.h>
#include "audio.h"
#include "disc.h"
#include "library.h"
#include "profile.h"
#include "achievements.h"
#include "ftp.h"
#include "video.h"
#include "jellyfin.h"
#include "updater.h"
typedef enum { LIB_UI_NONE,LIB_UI_LIST,LIB_UI_ROOTS,LIB_UI_BROWSE,LIB_UI_CATEGORY,LIB_UI_ROOT_ACTION,LIB_UI_DEPTH,LIB_UI_FILES,LIB_UI_VIDEO_SOURCES } LibraryPage;
typedef enum { JELLY_UI_NONE,JELLY_UI_SETTINGS,JELLY_UI_BROWSE } JellyPage;
enum { WIDTH=1280, HEIGHT=720 };
typedef enum { BLADE_MARKETPLACE, BLADE_LIVE, BLADE_GAMES, BLADE_MEDIA, BLADE_SYSTEM, BLADE_COUNT } Blade;
enum { UP=1, DOWN=2, LEFT=4, RIGHT=8, SELECT=16, BACK=32, EXIT=64, PROFILE=128, EJECT=256 };
typedef struct { unsigned held, previous, repeat_direction; float repeat_time; bool armed; } Input;
typedef struct {
    int blade, previous_blade, previous_depth, selected[BLADE_COUNT], depth, child;
    float position, target, origin, transition, elapsed, highlight, menu_transition,startup_elapsed;
    bool dialog, exit_requested, exit_menu,startup_active;
    const char *message;
    unsigned actions;
    UiSound sound;
    DiscStatus disc;
    VideoSnapshot video;VideoPicture video_picture;
    bool video_view,video_has_picture,video_controls,video_stretch,video_guide,video_resume_guide;
    float video_controls_time;char video_error[256];
    DiscCommand disc_command;
    float disc_elapsed;
    const LibrarySnapshot *library;
    unsigned library_revision;
    LibraryPage library_page,previous_library_page;
    LibraryCategory library_filter;
    int library_row,library_scroll,library_root;
    bool library_wait,library_depth_edit,library_scan_details;
    char library_path[LIBRARY_PATH];
    LibraryCommand library_command;
    const ProfileSnapshot *profile;unsigned profile_revision,profile_source_revision;
    ProfileCommand profile_command;
    bool profile_card_selected;
    const AchievementSnapshot *achievements;unsigned achievement_revision;
    AchievementRequest achievement_command;
    FtpSnapshot ftp;bool ftp_toggle;unsigned ftp_edit;
    UpdateSnapshot update;bool update_check,update_download,update_cancel;
    const JellySnapshot *jelly;unsigned jelly_revision,jelly_start,jelly_depth;JellyPage jelly_page,previous_jelly_page;
    JellyCommand jelly_command;unsigned jelly_edit;int jelly_row,jelly_scroll;bool jelly_wait,jelly_root_pending;
    float list_transition;char list_location[LIBRARY_PATH];
    bool achievement_page,previous_achievement_page,achievement_grid,achievement_signin_pending;
    int achievement_game,achievement_index,achievement_scroll;unsigned achievement_user;uint64_t achievement_xuid;
} Dashboard;
struct CanvasOps;
typedef struct { uint32_t *pixels; const struct CanvasOps *ops; void *context; } Canvas;
unsigned input_update(Input *in, unsigned buttons, float dt, bool connected);
void dashboard_init(Dashboard *d);
/* Application launch: play the blade opening, then enter the Games root. */
void dashboard_begin_startup(Dashboard *d);
void dashboard_update(Dashboard *d, unsigned actions, float dt);
void dashboard_set_profile(Dashboard *d,const ProfileSnapshot *profile);
bool dashboard_ftp_page(const Dashboard *d);
bool dashboard_update_page(const Dashboard *d);
void dashboard_set_jellyfin(Dashboard *,const JellySnapshot *);
bool dashboard_jellyfin_update(Dashboard *,unsigned);
int dashboard_jellyfin_count(const Dashboard *);
const char *dashboard_jellyfin_label(const Dashboard *,int);
void dashboard_jellyfin_dispatch(Dashboard *,JellyService *);
bool dashboard_video_http_open(Dashboard *,VideoPlayer *,const VideoHttpInput *);
void dashboard_set_achievements(Dashboard *d,const AchievementSnapshot *snapshot);
void dashboard_achievements_open(Dashboard *d);
bool dashboard_achievements_update(Dashboard *d,unsigned actions);
void dashboard_set_library(Dashboard *d,const LibrarySnapshot *library);
bool dashboard_library_update(Dashboard *d,unsigned actions);
void dashboard_library_open(Dashboard *d,LibraryPage page,LibraryCategory filter);
bool dashboard_video_open(Dashboard *,VideoPlayer *,const char *);
bool dashboard_video_update(Dashboard *,VideoPlayer *,unsigned,float,bool);
void dashboard_video_render(Canvas *,const Dashboard *);
int dashboard_library_count(const Dashboard *d);
int dashboard_library_root_count(const Dashboard *d);
int dashboard_library_root_index(const Dashboard *d,int row);
const char *dashboard_library_title(const Dashboard *d);
const char *dashboard_library_label(const Dashboard *d,int row);
int menu_count(int blade, int depth, int selected);
const char *menu_label(int blade, int depth, int selected, int row);
bool blade_has_profile(int blade);
bool blade_has_tray(int blade);
extern const char *const blade_names[BLADE_COUNT];
void dashboard_render(Canvas *c, const Dashboard *d);
void fill_rect(Canvas *c, int x,int y,int w,int h,uint32_t color);
void gradient(Canvas *c,int x,int y,int w,int h,uint32_t top,uint32_t bottom);
void text(Canvas *c,int x,int y,int size,uint32_t color,const char *s);
int text_width(int size,const char *s);
void circle(Canvas *c,int x,int y,int r,uint32_t color);
float selection_sample(unsigned action,unsigned index);
void text_down(Canvas *c,int x,int y,int size,uint32_t color,const char *s);
void line(Canvas *c,int x0,int y0,int x1,int y1,uint32_t color);
void ellipse(Canvas *c,int x,int y,int rx,int ry,uint32_t color);
#endif
