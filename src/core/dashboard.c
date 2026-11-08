#include "blades.h"
#include "animation.h"
#include "startup.h"
#include <math.h>
bool dashboard_ftp_page(const Dashboard *d){return d->blade==BLADE_SYSTEM&&d->depth&&d->selected[BLADE_SYSTEM]==4;}
bool dashboard_update_page(const Dashboard *d){return d->blade==BLADE_SYSTEM&&d->depth&&d->selected[BLADE_SYSTEM]==6;}
void dashboard_init(Dashboard *d) {
    *d=(Dashboard){.blade=BLADE_GAMES,.previous_blade=BLADE_GAMES,.position=BLADE_GAMES,.target=BLADE_GAMES,.origin=BLADE_GAMES,
        .transition=1,.menu_transition=1,.list_transition=1};
}
void dashboard_begin_startup(Dashboard *d) {
    dashboard_init(d);d->startup_active=true;d->startup_elapsed=0;
}
void dashboard_set_profile(Dashboard *d,const ProfileSnapshot *profile) {
    unsigned revision=profile?profile->revision:0;
    if(d->profile!=profile||d->profile_source_revision!=revision)d->profile_revision++;
    d->profile=profile;d->profile_source_revision=revision;
    if(d->achievement_page&&(!profile_signed_in(profile)||profile->identity.user!=d->achievement_user||profile->identity.xuid!=d->achievement_xuid)){
        d->achievement_page=d->previous_achievement_page=d->achievement_grid=false;d->depth=d->previous_depth=0;d->menu_transition=1;
    }
}
void dashboard_update(Dashboard *d,unsigned a,float dt) {
    /* Background motion follows wall time even if a frame takes over 100 ms.
       Keep the input/transition step bounded independently. */
    float elapsed=isfinite(dt)?fmaxf(0,dt):0;
    d->elapsed+=elapsed;dt=fminf(.1f,elapsed);
    d->disc_elapsed=d->disc.state==DISC_READING?d->disc_elapsed+elapsed:0;
    d->actions=a;d->sound=UI_SOUND_NONE;d->disc_command=DISC_NONE;d->library_command=(LibraryCommand){0};d->profile_command=(ProfileCommand){0};d->achievement_command=(AchievementRequest){0};
    d->ftp_toggle=false;
    d->ftp_edit=0;
    d->update_check=d->update_download=d->update_cancel=false;
    d->jelly_command=(JellyCommand){0};d->jelly_edit=0;
    if(d->startup_active) {
        float previous=d->startup_elapsed;
        d->startup_elapsed=fminf(STARTUP_SECONDS,previous+elapsed);
        if(previous<STARTUP_SOUND_TIME&&d->startup_elapsed>=STARTUP_SOUND_TIME)d->sound=UI_SOUND_STARTUP;
        d->actions=0;
        if(d->startup_elapsed>=STARTUP_SECONDS)d->startup_active=false;
        return;
    }
    if(d->achievement_signin_pending&&profile_signed_in(d->profile)&&!d->dialog&&!d->exit_menu){dashboard_achievements_open(d);return;}
    float cursor=(float)(d->depth?d->child:d->selected[d->blade]);
    if(d->jelly_page==JELLY_UI_BROWSE)cursor=(float)(d->jelly_row-d->jelly_scroll);
    else if(d->library_page)cursor=(float)(d->library_row-d->library_scroll);
    d->highlight+=(cursor-d->highlight)*fminf(1,dt*20);
    d->transition=fminf(1,d->transition+dt/BLADE_SECONDS);
    d->menu_transition=fminf(1,d->menu_transition+dt/MENU_SECONDS);
    d->list_transition=fminf(1,d->list_transition+dt/.15f);
    d->position=d->origin+(d->target-d->origin)*anim_smooth(d->transition);
    if(d->exit_menu) {
        if(a&(UP|DOWN))d->sound=UI_SOUND_FOCUS;
        if(a&BACK)d->sound=UI_SOUND_BACK;
        if(a&SELECT)d->sound=UI_SOUND_SELECT;
        if(a&BACK)d->exit_menu=false;
        if(a&(UP|DOWN))d->child=1-d->child;
        if(a&SELECT) {if(d->child)d->exit_requested=true;else d->exit_menu=false;}
        return;
    }
    if(d->dialog) {if(a&(BACK|SELECT)){d->dialog=false;d->sound=a&BACK?UI_SOUND_BACK:UI_SOUND_SELECT;}return;}
    bool remove_path=d->library_page==LIB_UI_ROOTS&&dashboard_library_root_index(d,d->library_row)>=0;
    if((a&EJECT)&&!remove_path) {
        if(disc_can_eject(&d->disc)){d->disc_command=DISC_EJECT;d->sound=UI_SOUND_SELECT;}
        return;
    }
    if(dashboard_achievements_update(d,a))return;
    if(dashboard_jellyfin_update(d,a))return;
    if(dashboard_library_update(d,a))return;
    if(dashboard_update_page(d)&&(a&PROFILE)){d->update_cancel=true;return;}
    if(a&PROFILE) {
        if(d->depth||(d->profile&&d->profile->busy))return;
        d->sound=UI_SOUND_SELECT;
        if(profile_signed_in(d->profile))d->profile_command=(ProfileCommand){.type=PROFILE_SIGN_OUT,.user=d->profile->identity.user,.xuid=d->profile->identity.xuid};
        else d->profile_command=(ProfileCommand){.type=PROFILE_SIGN_IN};
        return;
    }
    if(a&BACK) {
        d->achievement_signin_pending=false;
        if(d->depth) {
            d->sound=UI_SOUND_MENU_CLOSE;
            d->previous_depth=d->depth;d->depth=0;d->menu_transition=0;
            d->highlight=(float)d->selected[d->blade];
        }
        return;
    }
    if(a&EXIT) {d->exit_menu=true;d->child=0;d->sound=UI_SOUND_SELECT;return;}
    int change=(!!(a&RIGHT))-(!!(a&LEFT));
    if(change) {
        int next=d->blade+change;
        if(next>=0&&next<BLADE_COUNT) {
            int boundary=next<d->blade?next:d->blade;
            d->sound=(UiSound)(UI_SOUND_MARKETPLACE_LIVE+boundary);
            d->previous_blade=d->blade;d->blade=next;
            d->origin=d->position;d->target=(float)next;d->transition=0;
            d->depth=0;d->previous_depth=0;d->menu_transition=1;
            d->library_page=LIB_UI_NONE;
            d->jelly_page=d->previous_jelly_page=JELLY_UI_NONE;d->jelly_wait=d->jelly_root_pending=false;
            d->previous_library_page=LIB_UI_NONE;
            d->previous_jelly_page=JELLY_UI_NONE;
            d->profile_card_selected=false;d->achievement_signin_pending=false;d->previous_achievement_page=false;
            d->highlight=(float)d->selected[next];
        }
        return;
    }
    if(dashboard_update_page(d)){
        if(a&(UP|DOWN)){d->child=1-d->child;d->sound=UI_SOUND_FOCUS;}
        if(a&PROFILE)d->update_cancel=true;
        if(a&SELECT){d->update_check=!d->child;d->update_download=d->child!=0;d->sound=UI_SOUND_SELECT;}
        return;
    }
    if(dashboard_ftp_page(d)){
        if(a&UP){d->child=(d->child+2)%3;d->sound=UI_SOUND_FOCUS;}
        if(a&DOWN){d->child=(d->child+1)%3;d->sound=UI_SOUND_FOCUS;}
        if(a&SELECT){if(!d->child)d->ftp_toggle=true;else d->ftp_edit=(unsigned)d->child;d->sound=UI_SOUND_SELECT;}
        return;
    }
    int *row=d->depth?&d->child:&d->selected[d->blade];
    int n=menu_count(d->blade,d->depth,d->selected[d->blade]);
    if(a&(UP|DOWN))d->sound=UI_SOUND_FOCUS;
    bool blank_card=!d->depth&&blade_has_profile(d->blade);
    if(blank_card&&(a&(UP|DOWN))) {
        if(d->profile_card_selected){d->profile_card_selected=false;*row=(a&UP)?n-1:0;}
        else if(((a&UP)&&*row==0)||((a&DOWN)&&*row==n-1))d->profile_card_selected=true;
        else *row=(*row+(a&UP?n-1:1))%n;
        return;
    }
    if(a&UP)*row=(*row+n-1)%n;
    if(a&DOWN)*row=(*row+1)%n;
    if(a&SELECT) {
        if(blank_card&&d->profile_card_selected) {d->profile_command=profile_signed_in(d->profile)?(ProfileCommand){.type=PROFILE_VIEW,.user=d->profile->identity.user,.xuid=d->profile->identity.xuid}:(ProfileCommand){.type=PROFILE_SIGN_IN};d->sound=UI_SOUND_SELECT;return;}
        if(!d->depth) {
            if(blade_has_tray(d->blade)&&*row==n-1) {
                d->disc_command=disc_select(&d->disc);
                if(d->disc_command)d->sound=UI_SOUND_SELECT;
                return;
            }
            if(d->blade==BLADE_GAMES&&*row==0){dashboard_achievements_open(d);return;}
            d->previous_achievement_page=false;
            d->sound=UI_SOUND_MENU_OPEN;
            d->previous_library_page=LIB_UI_NONE;
            d->previous_depth=0;d->depth=1;d->child=0;d->highlight=0;
            d->menu_transition=0;
            if(d->blade==BLADE_GAMES&&(*row==2||*row==3))
                dashboard_library_open(d,LIB_UI_LIST,*row==2?LIB_ARCADE:LIB_DEMOS);
            if(d->blade==BLADE_MEDIA&&*row<=3)
                dashboard_library_open(d,*row==3?LIB_UI_ROOTS:*row==2?LIB_UI_VIDEO_SOURCES:LIB_UI_LIST,*row==0?LIB_MUSIC:*row==1?LIB_PICTURES:LIB_VIDEO);
            if(d->blade==BLADE_MEDIA&&*row==4)d->jelly_page=JELLY_UI_SETTINGS;
            if(dashboard_update_page(d))d->update_check=true;
        } else {
            d->sound=UI_SOUND_SELECT;
            if(d->blade==BLADE_GAMES&&d->selected[BLADE_GAMES]==1) {
                dashboard_library_open(d,d->child==4?LIB_UI_ROOTS:d->child==3?LIB_UI_FILES:LIB_UI_LIST,
                    d->child==0?LIB_ALL:d->child==1?LIB_APPS:LIB_HOMEBREW);
                if(d->child==3){d->library_command=(LibraryCommand){.type=LIB_CMD_FILES};d->library_wait=true;}
                return;
            }
            d->dialog=true;
            d->message="This service is not implemented in this build.\nDemo entries do not read or change your console.\nPress B to return.";
            if(d->blade==BLADE_SYSTEM&&d->selected[BLADE_SYSTEM]==0&&d->child==3)
                d->message="Blades 0.51.0 - standalone homebrew dashboard\nLocal artwork and Convection font support.\nBuilt with the open-source xsdk.\nNot affiliated with Microsoft. Press B to return.";
        }
    }
}
