#include "blades.h"
static void command(Dashboard *d,AchievementAction type){
    if(!profile_signed_in(d->profile))return;
    d->achievement_command=(AchievementRequest){.type=type,.user=d->profile->identity.user,.xuid=d->profile->identity.xuid,.first=(unsigned)d->achievement_scroll,.game_first=(unsigned)d->achievement_game/3*3};
    if(d->achievements&&d->achievement_game<(int)d->achievements->title_count)d->achievement_command.title_id=d->achievements->titles[d->achievement_game].id;
}
void dashboard_set_achievements(Dashboard *d,const AchievementSnapshot *s){
    d->achievements=s;d->achievement_revision=s?s->revision:0;
    if(s&&!s->busy){if(d->achievement_game>=(int)s->title_count)d->achievement_game=0;if(d->achievement_index>=(int)achievement_grid_count(s)){d->achievement_index=0;d->achievement_scroll=0;}}
}
void dashboard_achievements_open(Dashboard *d){
    if(!profile_signed_in(d->profile)){d->achievement_signin_pending=true;d->profile_command=(ProfileCommand){.type=PROFILE_SIGN_IN};d->sound=UI_SOUND_SELECT;return;}
    d->achievement_signin_pending=false;d->achievement_page=true;d->achievement_grid=false;
    d->achievement_game=d->achievement_index=d->achievement_scroll=0;d->achievement_user=d->profile->identity.user;d->achievement_xuid=d->profile->identity.xuid;
    d->previous_achievement_page=false;d->previous_depth=0;d->depth=1;d->child=0;d->highlight=0;d->menu_transition=0;d->sound=UI_SOUND_MENU_OPEN;command(d,ACH_LOAD);
}
bool dashboard_achievements_update(Dashboard *d,unsigned a){
    if(!d->achievement_page)return false;
    if(a&EXIT){d->exit_menu=true;d->child=0;d->sound=UI_SOUND_SELECT;return true;}
    if(a&BACK){
        d->sound=UI_SOUND_BACK;
        if(d->achievement_grid)d->achievement_grid=false;
        else {d->achievement_page=false;d->previous_achievement_page=true;d->previous_depth=1;d->depth=0;d->menu_transition=0;d->highlight=0;d->sound=UI_SOUND_MENU_CLOSE;}
        return true;
    }
    if(a&PROFILE){command(d,ACH_LOAD);d->achievement_game=d->achievement_index=d->achievement_scroll=0;d->achievement_grid=false;d->sound=UI_SOUND_SELECT;return true;}
    const AchievementSnapshot *s=d->achievements;if(!s)return true;
    if(!d->achievement_grid){
        int next=d->achievement_game;if(a&UP)next--;if(a&DOWN)next++;
        if(next>=0&&next<(int)s->title_count&&next!=d->achievement_game){d->achievement_game=next;d->achievement_index=d->achievement_scroll=0;command(d,ACH_GAME);d->sound=UI_SOUND_FOCUS;}
        if((a&RIGHT)&&!s->busy&&!s->error&&achievement_grid_count(s)){d->achievement_grid=true;d->sound=UI_SOUND_SELECT;}
        if((a&SELECT)&&!s->busy&&!s->error&&s->title_count){command(d,ACH_GUIDE_GAME);d->sound=UI_SOUND_SELECT;}
    }else{
        if(s->busy||s->error)return true;
        int next=d->achievement_index;
        if(a&LEFT){if(next%6==0){d->achievement_grid=false;d->sound=UI_SOUND_FOCUS;return true;}next--;}
        if(a&RIGHT)next++;
        if(a&UP)next-=6;
        if(a&DOWN)next+=6;
        if(next>=0&&next<(int)achievement_grid_count(s)&&next!=d->achievement_index){d->achievement_index=next;int first=next/ACH_GRID*ACH_GRID;if(first!=d->achievement_scroll){d->achievement_scroll=first;command(d,ACH_IMAGES);}d->sound=UI_SOUND_FOCUS;}
        if(a&SELECT){const AchievementItem *item=achievement_grid_item(s,(unsigned)d->achievement_index,NULL);if(item){command(d,ACH_GUIDE_DETAIL);d->achievement_command.achievement_id=item->id;d->sound=UI_SOUND_SELECT;}}
    }
    return true;
}
