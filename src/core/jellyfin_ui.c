#include "blades.h"
#include <string.h>
static void clamp(Dashboard *d){int previous=d->jelly_scroll,n=dashboard_jellyfin_count(d);if(d->jelly_row>=n)d->jelly_row=n?n-1:0;if(d->jelly_row<0)d->jelly_row=0;if(d->jelly_scroll>d->jelly_row)d->jelly_scroll=d->jelly_row;if(d->jelly_row>=d->jelly_scroll+LIBRARY_VISIBLE)d->jelly_scroll=d->jelly_row-LIBRARY_VISIBLE+1;d->highlight+=(float)(previous-d->jelly_scroll);if(d->highlight<0)d->highlight=0;if(d->highlight>LIBRARY_VISIBLE-1)d->highlight=LIBRARY_VISIBLE-1;}
void dashboard_set_jellyfin(Dashboard *d,const JellySnapshot *s){
    if(s&&s->revision!=d->jelly_revision&&!s->busy)d->jelly_wait=false;
    if(s&&(s->start!=d->jelly_start||s->depth!=d->jelly_depth)){d->jelly_row=d->jelly_scroll=0;d->highlight=0;d->list_transition=0;}
    d->jelly=s;d->jelly_revision=s?s->revision:0;d->jelly_start=s?s->start:0;d->jelly_depth=s?s->depth:0;if(d->jelly_page==JELLY_UI_BROWSE)clamp(d);
}
int dashboard_jellyfin_count(const Dashboard *d){
    if(d->jelly_page==JELLY_UI_SETTINGS)return 5;
    const JellySnapshot *s=d->jelly;if(!s||!s->loaded)return 0;
    return (int)s->page.count+(s->start?1:0)+(s->start+s->page.count<s->page.total?1:0);
}
const char *dashboard_jellyfin_label(const Dashboard *d,int row){
    if(d->jelly_page==JELLY_UI_SETTINGS){static const char *const labels[]={"Server Address","Username","Password","Connect","Remove Connection"};return row==3&&d->jelly&&d->jelly->state==JELLY_CONNECTED?"Disconnect":labels[row>=0&&row<5?row:0];}
    const JellySnapshot *s=d->jelly;if(!s||row<0)return "";
    if((unsigned)row<s->page.count)return s->labels[row][0]?s->labels[row]:s->page.items[row].name;
    if(s->start&&(unsigned)row==s->page.count)return "Previous Page";
    return "Next Page";
}
bool dashboard_jellyfin_update(Dashboard *d,unsigned a){
    if(d->jelly_page==JELLY_UI_NONE)return false;
    if(a&(LEFT|RIGHT|EXIT))return false;
    const JellySnapshot *s=d->jelly;bool busy=d->jelly_wait||(s&&s->busy);
    if(a&BACK){
        d->sound=UI_SOUND_BACK;
        if(d->jelly_page==JELLY_UI_BROWSE&&s&&s->depth&&!busy){d->jelly_command=(JellyCommand){.type=JELLY_BACK};d->jelly_wait=true;return true;}
        d->previous_jelly_page=d->jelly_page;d->jelly_page=JELLY_UI_NONE;d->jelly_wait=d->jelly_root_pending=false;
        if(d->selected[BLADE_MEDIA]==2){dashboard_library_open(d,LIB_UI_VIDEO_SOURCES,LIB_VIDEO);d->previous_jelly_page=JELLY_UI_NONE;}
        else {d->previous_depth=d->depth;d->depth=0;d->menu_transition=0;d->highlight=(float)d->selected[d->blade];d->sound=UI_SOUND_MENU_CLOSE;}
        return true;
    }
    if(busy)return true;
    int *row=d->jelly_page==JELLY_UI_SETTINGS?&d->child:&d->jelly_row;int n=dashboard_jellyfin_count(d);
    if(n&&(a&(UP|DOWN))){*row=(*row+(a&UP?n-1:1))%n;if(d->jelly_page==JELLY_UI_BROWSE)clamp(d);d->sound=UI_SOUND_FOCUS;}
    if(d->jelly_page==JELLY_UI_BROWSE&&(a&PROFILE)){d->jelly_command=(JellyCommand){.type=JELLY_REFRESH};d->jelly_wait=true;d->sound=UI_SOUND_SELECT;}
    if(!(a&SELECT))return true;
    d->sound=UI_SOUND_SELECT;
    if(d->jelly_page==JELLY_UI_SETTINGS){
        if(*row<3)d->jelly_edit=(unsigned)*row+1;
        else {d->jelly_command=(JellyCommand){.type=*row==4?JELLY_REMOVE:s&&s->state==JELLY_CONNECTED?JELLY_DISCONNECT:JELLY_CONNECT};d->jelly_wait=true;}
    }else if(s&&*row<(int)s->page.count){
        const xf_item *i=s->page.items+*row;
        if(!i->folder&&!jellyfin_playable(i)&&strcmp(i->type,"Series")&&strcmp(i->type,"Season")&&strcmp(i->type,"BoxSet"))return true;
        d->jelly_command=(JellyCommand){.type=jellyfin_playable(i)?JELLY_PLAY:JELLY_OPEN,.row=(unsigned)*row};d->jelly_wait=true;
    }else if(n){d->jelly_command=(JellyCommand){.type=s&&s->start&&(unsigned)*row==s->page.count?JELLY_PREVIOUS:JELLY_NEXT};d->jelly_wait=true;}
    return true;
}
void dashboard_jellyfin_dispatch(Dashboard *d,JellyService *s){
    if(d->jelly_command.type){
        if(jellyfin_request(s,&d->jelly_command))d->jelly_root_pending=false;
        else d->jelly_wait=false;
    }
    if(d->jelly_root_pending&&d->jelly_page==JELLY_UI_BROWSE&&d->jelly&&!d->jelly->busy&&!d->jelly_wait){
        if(jellyfin_request(s,&(JellyCommand){.type=JELLY_ROOT})){d->jelly_root_pending=false;d->jelly_wait=true;}
    }
    const xf_item *item=d->jelly_page==JELLY_UI_BROWSE&&!d->video_view&&!d->jelly_wait&&d->jelly&&!d->jelly->busy&&d->jelly_row>=0&&(unsigned)d->jelly_row<d->jelly->page.count?d->jelly->page.items+d->jelly_row:NULL;
    jellyfin_select(s,item&&jellyfin_playable(item)?item->id:NULL);
}
