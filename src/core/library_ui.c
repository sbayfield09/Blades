#include "blades.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
int dashboard_library_root_index(const Dashboard *d,int row) {
    if(d->library&&row>=0)for(size_t i=0;i<d->library->root_count;i++)
        if(library_media_category(d->library->roots[i].category)==(d->blade==BLADE_MEDIA)&&row--==0)return (int)i;
    return -1;
}
int dashboard_library_root_count(const Dashboard *d) {
    int count=0;if(d->library)for(size_t i=0;i<d->library->root_count;i++)
        count+=library_media_category(d->library->roots[i].category)==(d->blade==BLADE_MEDIA);
    return count;
}
int dashboard_library_count(const Dashboard *d) {
    const LibrarySnapshot *s=d->library;
    switch(d->library_page) {
    case LIB_UI_VIDEO_SOURCES:return 2;
    case LIB_UI_LIST:return (int)library_count(s,d->library_filter);
    case LIB_UI_ROOTS:return dashboard_library_root_count(d)+2;
    case LIB_UI_BROWSE:case LIB_UI_FILES:return (int)(s?s->folder_count:0);
    case LIB_UI_CATEGORY:return 4;
    case LIB_UI_ROOT_ACTION:return 3;
    case LIB_UI_DEPTH:return LIBRARY_DEPTH_MAX+2;
    default:return 0;
    }
}
const char *dashboard_library_title(const Dashboard *d) {
    switch(d->library_page) {
    case LIB_UI_VIDEO_SOURCES:return "Video";
    case LIB_UI_LIST:return d->library_filter==LIB_VIDEO?"Local":library_category_name(d->library_filter);
    case LIB_UI_ROOTS:return d->blade==BLADE_MEDIA?"Media Scan Folders":"Scan Folders";
    case LIB_UI_BROWSE:return "Choose Scan Folder";
    case LIB_UI_CATEGORY:return "Folder Category";
    case LIB_UI_ROOT_ACTION:return "Scan Folder";
    case LIB_UI_DEPTH:return "Scan Depth";
    case LIB_UI_FILES:return "File Manager";
    default:return "Games";
    }
}
const char *dashboard_library_label(const Dashboard *d,int row) {
    const LibrarySnapshot *s=d->library;
    switch(d->library_page) {
    case LIB_UI_VIDEO_SOURCES:return row==0?"Jellyfin":"Local";
    case LIB_UI_LIST: {const LibraryEntry *e=library_entry(s,d->library_filter,(size_t)row);return e?e->name:"";}
    case LIB_UI_ROOTS: {
        int root=dashboard_library_root_index(d,row);
        if(root>=0)return s->roots[root].path;
        return row==dashboard_library_root_count(d)?"Add Folder":"Scan Now";
    }
    case LIB_UI_BROWSE:case LIB_UI_FILES:return s&&row<(int)s->folder_count?s->folders[row].name:"";
    case LIB_UI_CATEGORY:return row<3?library_category_name((LibraryCategory)(row)):"Cancel";
    case LIB_UI_ROOT_ACTION:return row==0?"Change Scan Depth":row==1?"Remove Scan Folder":"Back";
    case LIB_UI_DEPTH: {
        static char label[64];library_depth_label(row==0?LIBRARY_DEPTH_ALL:(unsigned)(row-1),label,sizeof label);return label;
    }
    default:return "";
    }
}
static void clamp(Dashboard *d) {
    int previous=d->library_scroll;
    int n=dashboard_library_count(d);if(d->library_row>=n)d->library_row=n?n-1:0;if(d->library_row<0)d->library_row=0;
    if(d->library_scroll>d->library_row)d->library_scroll=d->library_row;
    if(d->library_row>=d->library_scroll+LIBRARY_VISIBLE)d->library_scroll=d->library_row-LIBRARY_VISIBLE+1;
    d->highlight+=(float)(previous-d->library_scroll);
    if(d->highlight<0)d->highlight=0;
    if(d->highlight>LIBRARY_VISIBLE-1)d->highlight=LIBRARY_VISIBLE-1;
}
void dashboard_set_library(Dashboard *d,const LibrarySnapshot *s) {
    unsigned before=d->library_revision;d->library=s;d->library_revision=s?s->revision:0;
    if(s&&s->revision!=before&&!s->busy)d->library_wait=false;
    if(d->library_page)clamp(d);
    if(d->library_page&&(d->library_page==LIB_UI_BROWSE||d->library_page==LIB_UI_FILES)&&s&&!s->busy&&strcmp(d->list_location,s->browse_path)){
        snprintf(d->list_location,sizeof d->list_location,"%s",s->browse_path);d->list_transition=0;d->highlight=(float)(d->library_row-d->library_scroll);
    }
    bool scan_now=s&&d->library_page==LIB_UI_ROOTS&&d->library_row==dashboard_library_root_count(d)+1;
    if(!scan_now)d->library_scan_details=false;
    else if(s->progress.scanning)d->library_scan_details=true;
}
void dashboard_library_open(Dashboard *d,LibraryPage page,LibraryCategory filter) {
    d->library_page=page;d->library_filter=filter;d->library_row=d->library_scroll=0;d->library_scan_details=false;
    d->highlight=0;d->list_transition=0;d->list_location[0]=0;
}
static void request(Dashboard *d,LibraryCommandType type,const char *path) {
    if(!d->library||d->library_wait||d->library->busy)return;
    d->library_command.type=type;
    if(type==LIB_CMD_SCAN&&d->library_page==LIB_UI_ROOTS&&d->library_row==dashboard_library_root_count(d)+1)d->library_scan_details=true;
    if(path)snprintf(d->library_command.path,sizeof d->library_command.path,"%s",path);
    d->library_wait=true;
}
bool dashboard_library_update(Dashboard *d,unsigned a) {
    if(!d->library_page)return false;
    const LibrarySnapshot *s=d->library;
    if(a&(LEFT|RIGHT|EXIT))return false;
    if(a&BACK) {
        d->sound=UI_SOUND_BACK;
        switch(d->library_page) {
        case LIB_UI_LIST:
            if(d->blade==BLADE_MEDIA&&d->selected[BLADE_MEDIA]==2){dashboard_library_open(d,LIB_UI_VIDEO_SOURCES,LIB_VIDEO);d->library_row=1;break;}
            /* fall through */
        case LIB_UI_VIDEO_SOURCES:
            if(d->library_page==LIB_UI_VIDEO_SOURCES){d->previous_library_page=d->library_page;d->library_page=LIB_UI_NONE;d->previous_depth=d->depth;d->depth=0;d->menu_transition=0;d->highlight=(float)d->selected[d->blade];d->sound=UI_SOUND_MENU_CLOSE;break;}
            d->previous_library_page=d->library_page;d->library_page=LIB_UI_NONE;
            if(d->blade!=BLADE_GAMES||d->selected[BLADE_GAMES]!=1){d->previous_depth=d->depth;d->depth=0;d->menu_transition=0;d->highlight=(float)d->selected[d->blade];d->sound=UI_SOUND_MENU_CLOSE;}
            break;
        case LIB_UI_BROWSE:case LIB_UI_FILES:
            if(d->library_wait||(s&&s->busy))break;
            if(s&&s->browse_path[0]) {char p[LIBRARY_PATH];snprintf(p,sizeof p,"%s",s->browse_path);library_parent(p);request(d,d->library_page==LIB_UI_FILES?LIB_CMD_FILES:LIB_CMD_BROWSE,p);d->library_row=d->library_scroll=0;}
            else if(d->library_page==LIB_UI_FILES)d->library_page=LIB_UI_NONE;
            else dashboard_library_open(d,LIB_UI_ROOTS,LIB_ALL);
            break;
        case LIB_UI_CATEGORY:dashboard_library_open(d,LIB_UI_BROWSE,LIB_ALL);break;
        case LIB_UI_ROOT_ACTION:dashboard_library_open(d,LIB_UI_ROOTS,LIB_ALL);break;
        case LIB_UI_DEPTH:dashboard_library_open(d,d->library_depth_edit?LIB_UI_ROOT_ACTION:d->blade==BLADE_MEDIA?LIB_UI_BROWSE:LIB_UI_CATEGORY,d->library_filter);break;
        default:
            d->previous_library_page=d->library_page;
            d->library_page=LIB_UI_NONE;
            if(d->blade!=BLADE_GAMES||d->selected[BLADE_GAMES]!=1){d->previous_depth=d->depth;d->depth=0;d->menu_transition=0;d->highlight=(float)d->selected[d->blade];d->sound=UI_SOUND_MENU_CLOSE;}
            break;
        }
        return true;
    }
    int n=dashboard_library_count(d);
    if(n&&(a&(UP|DOWN))) {
        d->library_row=(d->library_row+(a&UP?n-1:1))%n;clamp(d);d->sound=UI_SOUND_FOCUS;
        if(d->library_page!=LIB_UI_ROOTS||!s||d->library_row!=dashboard_library_root_count(d)+1)d->library_scan_details=false;
    }
    if(a&PROFILE&&d->library_page!=LIB_UI_VIDEO_SOURCES&&s&&s->busy&&s->progress.scanning) {
        d->library_command.type=LIB_CMD_STOP;d->sound=UI_SOUND_SELECT;return true;
    }
    if(d->library_page!=LIB_UI_VIDEO_SOURCES&&(d->library_wait||(s&&s->busy)))return true;
    if((a&EJECT)&&s&&d->library_page==LIB_UI_ROOTS&&dashboard_library_root_index(d,d->library_row)>=0) {
        request(d,LIB_CMD_REMOVE,NULL);d->library_command.root=(size_t)dashboard_library_root_index(d,d->library_row);d->sound=UI_SOUND_SELECT;return true;
    }
    if(a&PROFILE) {
        if(d->library_page==LIB_UI_LIST||d->library_page==LIB_UI_ROOTS)request(d,LIB_CMD_SCAN,NULL);
        if(d->library_page==LIB_UI_BROWSE&&s&&s->browse_path[0]) {
            snprintf(d->library_path,sizeof d->library_path,"%s",s->browse_path);
            d->library_depth_edit=false;
            dashboard_library_open(d,d->blade==BLADE_MEDIA?LIB_UI_DEPTH:LIB_UI_CATEGORY,d->blade==BLADE_MEDIA?LIB_MEDIA_FOLDER:LIB_ALL);
            if(d->blade==BLADE_MEDIA){d->library_row=LIBRARY_DEPTH_DEFAULT+1;clamp(d);}
        }
        if(d->library_page==LIB_UI_FILES)request(d,LIB_CMD_FILES,s?s->browse_path:"");
        d->sound=UI_SOUND_SELECT;return true;
    }
    if(!(a&SELECT))return true;
    if(d->library_page==LIB_UI_FILES&&s&&d->library_row<(int)s->folder_count) {
        const LibraryFolder *f=s->folders+d->library_row;
        if(f->is_file&&!f->executable)return true;
    }
    d->sound=UI_SOUND_SELECT;
    switch(d->library_page) {
    case LIB_UI_VIDEO_SOURCES:
        if(d->library_row==1)dashboard_library_open(d,LIB_UI_LIST,LIB_VIDEO);
        else {d->highlight=0;d->list_transition=0;d->library_page=LIB_UI_NONE;d->jelly_page=JELLY_UI_BROWSE;d->jelly_row=d->jelly_scroll=0;d->jelly_command=(JellyCommand){.type=JELLY_ROOT};d->jelly_wait=true;d->jelly_root_pending=true;}
        break;
    case LIB_UI_LIST: {
        const LibraryEntry *e=library_entry(s,d->library_filter,(size_t)d->library_row);
        if(e){request(d,LIB_CMD_LAUNCH,NULL);if(d->library_command.type)d->library_command.entry=*e;}
        break;
    }
    case LIB_UI_ROOTS:
        if(dashboard_library_root_index(d,d->library_row)>=0){d->library_root=dashboard_library_root_index(d,d->library_row);dashboard_library_open(d,LIB_UI_ROOT_ACTION,LIB_ALL);}
        else if(d->library_row==dashboard_library_root_count(d)){dashboard_library_open(d,LIB_UI_BROWSE,LIB_ALL);request(d,LIB_CMD_BROWSE,"");}
        else request(d,LIB_CMD_SCAN,NULL);
        break;
    case LIB_UI_BROWSE:
        if(s&&d->library_row<(int)s->folder_count){request(d,LIB_CMD_BROWSE,s->folders[d->library_row].path);d->library_row=d->library_scroll=0;}
        break;
    case LIB_UI_FILES:
        if(s&&d->library_row<(int)s->folder_count) {
            const LibraryFolder *f=s->folders+d->library_row;
            if(f->executable) {
                request(d,LIB_CMD_LAUNCH,NULL);d->library_command.direct_launch=true;
                d->library_command.entry=(LibraryEntry){.kind=LIB_XEX,.category=LIB_HOMEBREW};
                snprintf(d->library_command.entry.path,LIBRARY_PATH,"%s",f->path);
            } else if(!f->is_file){request(d,LIB_CMD_FILES,f->path);d->library_row=d->library_scroll=0;}
        }
        break;
    case LIB_UI_CATEGORY:
        if(d->library_row<3){LibraryCategory c=(LibraryCategory)(d->library_row);d->library_depth_edit=false;dashboard_library_open(d,LIB_UI_DEPTH,c);d->library_row=LIBRARY_DEPTH_DEFAULT+1;clamp(d);}
        else dashboard_library_open(d,LIB_UI_BROWSE,LIB_ALL);
        break;
    case LIB_UI_ROOT_ACTION:
        if(!d->library_row&&s&&d->library_root<(int)s->root_count) {
            unsigned depth=s->roots[d->library_root].max_depth;d->library_depth_edit=true;
            dashboard_library_open(d,LIB_UI_DEPTH,LIB_ALL);d->library_row=depth==LIBRARY_DEPTH_ALL?0:(int)depth+1;clamp(d);break;
        }
        if(d->library_row==1){request(d,LIB_CMD_REMOVE,NULL);d->library_command.root=(size_t)d->library_root;}
        dashboard_library_open(d,LIB_UI_ROOTS,LIB_ALL);break;
    case LIB_UI_DEPTH: {
        LibraryCategory c=d->library_filter;unsigned depth=d->library_row==0?LIBRARY_DEPTH_ALL:(unsigned)(d->library_row-1);
        request(d,d->library_depth_edit?LIB_CMD_DEPTH:LIB_CMD_ADD,d->library_depth_edit?NULL:d->library_path);
        d->library_command.root=(size_t)d->library_root;d->library_command.max_depth=depth;d->library_command.category=c;
        dashboard_library_open(d,LIB_UI_ROOTS,LIB_ALL);break;
    }
    default:break;
    }
    return true;
}
