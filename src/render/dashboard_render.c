#include "theme.h"
#include "draw_list.h"
#include "skin.h"
#include <stdio.h>
#include "resources.h"
#include "animation.h"
#include "startup.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The selected lip meets the divider location; its shadow replaces that rule. */
enum { MENU_ROW_PITCH=53, MENU_DIVIDER_OFFSET=51, MENU_DIVIDER_HEIGHT=3,
       MENU_SHADOW_OFFSET=MENU_DIVIDER_OFFSET,
       MENU_TOP_OFFSET=MENU_DIVIDER_OFFSET-MENU_ROW_PITCH,
       MENU_FILL_HEIGHT=MENU_DIVIDER_OFFSET-MENU_TOP_OFFSET };

static void prompt(Canvas *c,int x,int y,char button,uint32_t color,bool enabled) {
    char composed[40];snprintf(composed,sizeof composed,"prompt/%c%s.png",button,enabled?"":"-disabled");
    if(resources_draw(c,composed,x-16,y-16,32,32))return;
    char filename[]="A-Button.png";filename[0]=button;
    char label[2]={button,0};
    if(resources_draw(c,enabled?filename:"disabled-Button.png",x-16,y-16,32,32)) {
        text(c,x-text_width(22,label)/2,y-12,22,enabled?0xff101c18:0x90506048,label);
        return;
    }
    circle(c,x+1,y+2,16,0x50000000);
    for(int r=15;r>0;r--)circle(c,x,y,r,theme_mix(0xff344338,color,(float)r/15));
    ellipse(c,x-3,y-6,10,5,0x60ffffff);
    text(c,x-text_width(22,label)/2,y-12,22,0xff182c30,label);
}
static void sphere(Canvas *c,int x,int y) {
    circle(c,x+2,y+2,41,0x45000000);
    for(int r=40;r>0;r--)circle(c,x,y,r,theme_mix(0xff3c5151,0xffd3e2d7,1-(float)r/40));
    /* Original vector rendering of the crossed green ribbons. */
    for(int k=-26;k<=26;k++) {
        int yy=(int)(k*0.9f),width=3+(int)(fabsf((float)k)*0.13f);
        uint32_t color=theme_mix(0xff335d19,0xffa8d872,1-fabsf((float)k)/35);
        fill_rect(c,x+k-width,y+yy,width*2,3,color);
        fill_rect(c,x+k-width,y-yy,width*2,3,color);
    }
    ellipse(c,x-14,y-19,12,6,0x50ffffff);
}
static void brand(Canvas *c,int blade,int x,int y) {
    if(blade==BLADE_LIVE) {
        if(resources_draw_fit(c,"livelogo.png",x-25,y-15,180,113))return;
        text(c,x+1,y+2,36,0xff5b3b27,"XBOX");text(c,x,y,36,0xffbad69e,"XBOX");
        text(c,x-21,y+40,49,0xfff0cda0,"LIVE");
        text(c,x-19,y+42,45,0xff653c23,"LIVE");
    } else {
        /* Derived from the same logo alpha; cached PNG, no per-frame blur. */
        resources_draw_opacity(c,"branding/360-shadow.png",x-38,y-33,202,130,120);
        if(resources_draw_fit(c,"splash_360.png",x-34,y-30,190,118))return;
        sphere(c,x+76,y+9);
        text(c,x-24,y+61,35,0xff355c22,"XBOX 360");
        text(c,x-25,y+60,35,0xffb7d78b,"XBOX 360");
    }
}
static void brand_divider(Canvas *c,int b,int x) {
    int left=x+420,width=theme_right(b)-left-30;
    /* The reference groove fades out at both ends below the branding area.
       Keep it with root content, so submenu/content fades hide it too. */
    for(int i=0;i<width;i++) {
        float t=(float)i/(width-1),fade=sinf(t*3.14159265f);fade*=fade;
        unsigned dark=(unsigned)(43*fade),light=(unsigned)(13*fade);
        fill_rect(c,left+i,232,1,2,dark<<24);
        fill_rect(c,left+i,234,1,1,(light<<24)|0xffffff);
    }
}
static unsigned focus_alpha(int position,int width){
    /* Games 04:28/04:37 and Media 05:09: a broad, slightly warm sheen,
       rather than a narrow white spotlight. Endpoints remain transparent. */
    float t=((float)position+.5f)/width,curve=sinf(t*3.14159265f);
    return (unsigned)(150*powf(curve,1.6f));
}
static void gamer_card_shadow(Canvas *c,int x) {
    /* Rounded frame's actual lower contour: its two 10px corners rise
       toward the sides. Fade the penumbra there instead of cutting a strip. */
    static const unsigned char curve[]={0,4,6,7,8,8,9,9,9,9};
    for(int k=0;k<11;k++) {
        unsigned tail=(unsigned)(36*(10-k)/10),last_alpha=0;int begin=0,last_y=-1;
        for(int i=0;i<=400;i++) {
            int side=i<200?i:399-i;
            int y=side<0?-1:224-(side<10?10-curve[side]:0)+k;
            unsigned alpha=side<0?0:side<10?tail*(unsigned)(side*2+1)/20:tail;
            if(y!=last_y||alpha!=last_alpha) {
                if(last_alpha)fill_rect(c,x+begin,last_y,i-begin,1,last_alpha<<24);
                begin=i;last_y=y;last_alpha=alpha;
            }
        }
    }
}
static void gamer_card(Canvas *c,int b,int x,const Dashboard *d) {
    const ProfileSnapshot *p=d->profile;bool signed_in=profile_signed_in(p);
    gamer_card_shadow(c,x);
    theme_round(c,x,76,400,148,10,0x10ffffff);
    /* Share the row sheen, reaching the rounded inner frame. The name band
       stays above the focus layer, as in the signed-out/signed-in footage. */
    if(d->profile_card_selected)for(int i=0;i<392;i++) {
        int edge=i<8?8-i:i>=384?i-383:0;
        int inset=edge?8-(int)sqrtf((float)(64-edge*edge)):0;
        fill_rect(c,x+4+i,80+inset,1,141-2*inset,(focus_alpha(i,392)<<24)|0xe2eadf);
    }
    for(int y=0;y<34;y++) {
        unsigned alpha=y<22?32:(unsigned)(32*(33-y)/12);
        fill_rect(c,x+4,79+y,392,1,alpha<<24);
    }
    /* Finish at the inner border; keep both frame edges above the sheen.
       The curved shadow was drawn behind the frame, outside its contour. */
    theme_outline(c,x,76,400,148,10,0x65000000);
    theme_outline(c,x+2,78,396,144,8,0x50ffffff);
    if(!signed_in) {
        theme_label(c,x+14,77,29,"Sign In",false);
        fill_rect(c,x+16,126,84,84,0x70000000);
        resources_draw_fit(c,"ico_64x_DefaultPicture.png",x+18,128,80,80);
        char found[64];
        if(p&&p->profiles_known)snprintf(found,sizeof found,"%u Profile%s Found",p->available_profiles,p->available_profiles==1?"":"s");
        else snprintf(found,sizeof found,"Choose a Profile");
        theme_label_fit(c,x+108,139,24,278,found);
        brand(c,b,x+548,117);brand_divider(c,b,x);return;
    }
    theme_label_fit(c,x+14,77,29,370,p->identity.name);
    fill_rect(c,x+16,126,84,84,0x70000000);
    bool picture=false;
    if(p->has_picture) {
        static unsigned image_id;if(!image_id)image_id=canvas_image_id();
        canvas_image_tagged(c,(DrawImage){.pixels=p->picture,.id=image_id,.revision=p->picture_revision,.width=64,.height=64,.format=IMAGE_RGBA},x+18,128,80,80);picture=true;
    }
    if(!picture)resources_draw_fit(c,"ico_64x_DefaultPicture.png",x+18,128,80,80);
    theme_label(c,x+108,115,28,"Games",false);
    /* Restore the pre-0.36 placement requested after console comparison. */
    bool score_icon=resources_draw_fit(c,"GScore.png",x+104,148,32,32);
    theme_label(c,x+108+(score_icon?28:0),146,28,score_icon?"amerscore":"Gamerscore",false);
    theme_label(c,x+108,177,28,"Achievements",false);
    const unsigned flags[]={PROFILE_GAMES,PROFILE_SCORE,PROFILE_ACHIEVEMENTS};
    const uint32_t values[]={p->games,p->score,p->achievements};
    for(unsigned row=0;row<3;row++) {
        char value[32];if(!(p->known&flags[row]))snprintf(value,sizeof value,"--");else snprintf(value,sizeof value,"%u",values[row]);
        const char *labels[]={"Games",score_icon?"amerscore":"Gamerscore","Achievements"};
        int budget=386-108-text_width(28,labels[row])-(row==1&&score_icon?28:0)-12;
        int size=28;while(size>14&&text_width(size,value)>budget)size--;
        theme_label(c,x+386-text_width(size,value),115+(int)row*31+(28-size)/2,size,value,false);
    }
    brand(c,b,x+548,117);
    brand_divider(c,b,x);
}
static int icon_for(int blade,int row) {
    static const int icons[BLADE_COUNT][6]={{3,7,1,0,0,0},{0,1,1,4,0,0},{2,3,10,11,4,0},{6,7,7,7,5,4},{9,1,8,1,5,9}};
    return icons[blade][row%6];
}
static void selection(Canvas *c,int x,int y,int w,int height,uint32_t base) {
    (void)base;
    /* The original selection is a translucent horizontal sheen. The satin
       remains visible, and menu lettering stays dark on the selected row. */
    for(int i=0;i<w;i++) {
        unsigned alpha=focus_alpha(i,w);
        fill_rect(c,x+i,y+2,1,height-2,(alpha<<24)|0xe2eadf);
        fill_rect(c,x+i,y+1,1,1,((alpha*3/4)<<24)|0xe2eadf);
        fill_rect(c,x+i,y,1,1,((alpha/2)<<24)|0xe2eadf);
        fill_rect(c,x+i,y+height-1,1,1,((alpha/4)<<24)|0xffffff);
    }
}
static void menu_top_divider(Canvas *c,int x,int first,int width) {
    fill_rect(c,x,first-4,width,2,0x35000000);
    fill_rect(c,x,first-2,width,1,0x20ffffff);
}
static bool menu_focus_covers_divider(int divider,int row) {
    /* Fixed grooves must not cut through either the moving sheen or its
       contact shadows. Other grooves inside the two recessed groups remain. */
    return divider>=row+MENU_TOP_OFFSET-15&&divider<row+MENU_SHADOW_OFFSET+15;
}
static void menu_divider(Canvas *c,int x,int y,int width,bool covered) {
    /* Under the selected lip the continuous penumbra replaces this groove.
       Drawing it again above the shadow would leave a separate hard bar. */
    if(covered)return;
    fill_rect(c,x,y+MENU_DIVIDER_OFFSET,width,2,0x35000000);
    fill_rect(c,x,y+MENU_DIVIDER_OFFSET+2,width,1,0x20ffffff);
}
static void tray(Canvas *c,int b,int x,int right,bool selected,uint32_t base,const DiscStatus *disc) {
    char label[DISC_TITLE_BYTES+16];disc_label(disc,label,sizeof label);
    if(skin_tray(c,b,selected,label))return;
    theme_round(c,x+12,536,right-x-57,50,25,selected?0x28ffffff:0x08000000);
    theme_outline(c,x+12,536,right-x-57,50,25,0x40000000);
    (void)base;
    circle(c,x+36,561,40,0x08000000);
    ellipse(c,x+36,561,39,39,0x08ffffff);
    line(c,x+29,561,x+40,550,0x70617566);line(c,x+29,561,x+40,572,0x70617566);
    line(c,x+40,550,x+40,572,0x70617566);fill_rect(c,x+45,550,2,23,0x70617566);
    theme_label_fit(c,x+87,541,29,right-x-144,label);
}
static const char *description(int b,int row) {
    static const char *const system[6]={
        "Edit your Xbox 360 system\nsettings, including:\n\n- Display\n- Audio\n- Language\n- Remote control\n- and more",
        "Manage settings for your\nfamily and your console.",
        "View storage devices and\nmanage content.",
        "Review network settings\nand connection information.",
        "View FTP server status\nand transfer files with\na computer on your network.",
        "Review the console's\ninitial setup options."
    };
    if(b==BLADE_SYSTEM)return row==6?"Check GitHub Releases and\ndownload a newer version\nof Blades.":system[row%6];
    if(b==BLADE_GAMES) {
        static const char *const game[5]={"Track your gaming\naccomplishments.","View the games you have\nplayed on your Xbox 360.","Enjoy these casual games and\narcade classics. Check back\noften for new games.","Play demos and games\ncreated with XNA Game Studio\nExpress.","Open the console's disc tray."};
        return game[row%5];
    }
    if(b==BLADE_MEDIA) {
        static const char *const media[6]={"Turn your Xbox 360 into a\nmusic player.","View your pictures and\nphoto collections.","Browse Jellyfin and\nlocal videos.","Manage scan folders for\nmusic, pictures and video.","Manage your Jellyfin\nserver connection.","Open the console's disc tray."};
        return media[row%6];
    }
    if(b==BLADE_MARKETPLACE) {
        static const char *const marketplace[3]={"Discover games, demos\nand downloadable content.","Discover movies and\ntelevision entertainment.","Review your Xbox Live\naccount options."};
        return marketplace[row%3];
    }
    return "Games. Tournaments.\nEntertainment. All the rewards.\nEndless possibilities. What are\nyou waiting for?";
}
static void footer(Canvas *c,int left,int right,bool depth,const DiscStatus *disc,const Dashboard *d) {
    int a=right-42,b=a-19;
    bool library_x=d->library_page==LIB_UI_LIST||d->library_page==LIB_UI_ROOTS||d->library_page==LIB_UI_FILES||
        (d->library_page==LIB_UI_BROWSE&&d->library&&d->library->browse_path[0]);
    bool scanning=d->library_page&&d->library_page!=LIB_UI_VIDEO_SOURCES&&d->library&&d->library->busy&&d->library->progress.scanning;
    bool updating=dashboard_update_page(d)&&(d->update.state==UPDATE_DOWNLOADING||d->update.state==UPDATE_CHECKING||d->update.state==UPDATE_PREPARING||d->update.state==UPDATE_READY);
    library_x|=scanning||d->achievement_page||d->jelly_page==JELLY_UI_BROWSE||updating;
    prompt(c,left+65,669,'X',0xff3d91db,scanning||((!depth||library_x)&&!d->library_wait&&!(d->profile&&d->profile->busy)));
    if(!depth)theme_label(c,left+89,651,29,profile_signed_in(d->profile)?"Sign Out":"Sign In",true);
    if(library_x)theme_label(c,left+89,651,29,updating?"Cancel":d->achievement_page||d->jelly_page==JELLY_UI_BROWSE?"Refresh":scanning?"Stop Scan":d->library_page==LIB_UI_BROWSE?"Use Folder":d->library_page==LIB_UI_FILES?"Refresh":"Rescan",true);
    bool remove=d->library_page==LIB_UI_ROOTS&&d->library&&dashboard_library_root_index(d,d->library_row)>=0;
    bool eject=remove?(!d->library->busy&&!d->library_wait):disc_can_eject(disc);
    prompt(c,left+85,637,'Y',0xffd5bf48,eject);
    if(eject)theme_label(c,left+109,619,29,remove?"Remove":"Eject",true);
    prompt(c,b,637,'B',depth?0xffc44935:0xffcbbdb0,depth);
    if(depth)theme_label(c,b-24-text_width(29,"Back"),619,29,"Back",true);
    const char *select=d->profile_card_selected&&!depth?(profile_signed_in(d->profile)?"Profile":"Sign In"):"Select";
    bool can_select=!d->achievement_page||(d->achievements&&!d->achievements->busy&&!d->achievements->error&&(d->achievement_grid?achievement_grid_count(d->achievements):d->achievements->title_count));
    if(d->jelly_page)can_select=d->jelly&&!d->jelly->busy&&!d->jelly_wait&&dashboard_jellyfin_count(d)>0;
    if(dashboard_update_page(d))can_select=!updating&&(!d->child||((d->update.state==UPDATE_AVAILABLE||d->update.state==UPDATE_ERROR||d->update.state==UPDATE_CANCELLED)&&update_version_compare(d->update.version,BLADES_VERSION)>0));
    if(d->library_page==LIB_UI_FILES){
        const LibrarySnapshot *s=d->library;int row=d->library_row;
        const LibraryFolder *f=s&&row>=0&&row<(int)s->folder_count?s->folders+row:NULL;
        can_select=f&&!s->busy&&!d->library_wait&&(!f->is_file||f->executable);
    }
    if(can_select)theme_label(c,a-24-text_width(29,select),651,29,select,true);
    prompt(c,a,669,'A',0xffc0d47c,can_select);
}
static void library_path(Canvas *c,int x,int y,const char *path,int bottom) {
    /* Keep the filename visible when the full location is taller than the
       details panel. The omitted prefix is marked; no duplicate path footer. */
    const char *start=path;char fitted[LIBRARY_PATH+4];
    for(;;) {
        snprintf(fitted,sizeof fitted,"%s%s",start==path?"":"...",start);
        const char *p=fitted;int lines=0;
        while(*p) {
            char line[96]={0};size_t n=0;
            while(p[n]&&n<sizeof line-1){line[n]=p[n];line[n+1]=0;if(text_width(22,line)>285)break;n++;}
            if(!n)break;
            p+=n;lines++;
        }
        if(lines<=(bottom-y+27)/28||!*start){path=fitted;break;}
        start++;
    }
    while(*path&&y<bottom) {
        char line[96];size_t n=0;
        while(path[n]&&n<sizeof line-1) {line[n]=path[n];line[n+1]=0;if(text_width(22,line)>285)break;n++;}
        if(!n)break;
        line[n]=0;theme_label(c,x,y,22,line,false);path+=n;y+=28;
    }
}
static void library_content(Canvas *c,const Dashboard *d) {
    const LibrarySnapshot *s=d->library;int x=SUBMENU_CONTENT,n=dashboard_library_count(d),row=d->library_row;
    bool scanning=s&&s->busy&&s->progress.scanning;
    bool scan_details=scanning||(s&&d->library_scan_details&&d->library_page==LIB_UI_ROOTS&&row==dashboard_library_root_count(d)+1);
    theme_label(c,x+14,22,35,dashboard_library_title(d),true);
    if(n) {
        int hy=87+(int)lroundf(d->highlight*MENU_ROW_PITCH);
        menu_top_divider(c,x,87,625);
        theme_menu_upper_shadow(c,x,87,625,hy+MENU_TOP_OFFSET,true);
        selection(c,x,hy+MENU_TOP_OFFSET,625,MENU_FILL_HEIGHT,theme_color(d->blade));theme_menu_shadow(c,x,hy+MENU_SHADOW_OFFSET,625,571,true);
        for(int i=d->library_scroll;i<n&&i<d->library_scroll+LIBRARY_VISIBLE;i++) {
            int y=87+(i-d->library_scroll)*MENU_ROW_PITCH;
            menu_divider(c,x,y,613,menu_focus_covers_divider(y+MENU_DIVIDER_OFFSET,hy));
            theme_label_fit(c,x+14,y+5,28,590,dashboard_library_label(d,i));
        }
    } else theme_label(c,x+14,96,28,scanning?"Scanning...":d->library_page==LIB_UI_BROWSE?"No subfolders":d->library_page==LIB_UI_FILES?(s&&s->busy?"Loading...":"No files or folders"):d->blade==BLADE_MEDIA?"No media found":"No games found",false);
    int root=dashboard_library_root_index(d,row);
    int info=815;char count[80];snprintf(count,sizeof count,"%d of %d",n?row+1:0,n);
    theme_label(c,info,94,25,count,false);
    const char *path=NULL;
    if(scan_details) {
        const LibraryProgress *p=&s->progress;char totals[128];
        const char *heading=scanning?(p->stopping?"Stopping after current read":p->kind==LIB_SCAN_FOLDER?"Scanning folder":p->kind==LIB_SCAN_FILE?"Scanning file":"Checking entry"):
            (d->library_wait||s->busy)?"Starting scan...":p->stopping?"Scan stopped":s->errors?"Scan complete with errors":"Scan complete";
        theme_label(c,info,140,24,heading,false);
        snprintf(totals,sizeof totals,"%u folders checked\n%u files checked\n%zu items found\nCurrent depth: %u",p->folders,p->files,p->found,p->depth);
        theme_label(c,info,189,23,totals,false);path=p->path;
    } else if(d->library_page==LIB_UI_VIDEO_SOURCES){
        theme_label(c,info,140,24,row?"Local videos":"Jellyfin videos",false);
        theme_label(c,info,192,23,row?"Videos from your saved\nscan folders. Manage paths\nunder Video Marketplace.":"Browse your Jellyfin\nlibraries, movies and TV.\n\nManage your connection\nunder Media Center.",false);
    } else if(d->library_page==LIB_UI_LIST) {
        const LibraryEntry *e=library_entry(s,d->library_filter,(size_t)row);
        theme_label(c,info,140,24,e?library_category_name(e->category):d->blade==BLADE_MEDIA?"Add folders under\nVideo Marketplace.":"Add folders under\nPlayed Games > Scan Folders.",false);
        if(e){theme_label(c,info,184,22,e->kind==LIB_MEDIA?(e->category==LIB_VIDEO?"A: Play video":"Playback not available yet"):e->kind==LIB_PACKAGE?"Downloaded title":"Xbox 360 executable",false);path=e->path;}
    } else if(d->library_page==LIB_UI_ROOTS) {
        char depth[80];
        if(s&&root>=0){char label[64];library_depth_label(s->roots[root].max_depth,label,sizeof label);snprintf(depth,sizeof depth,"Depth: %s",label);}
        else snprintf(depth,sizeof depth,"Default depth: %u",LIBRARY_DEPTH_DEFAULT);
        theme_label(c,info,140,23,depth,false);
        if(s&&root>=0){theme_label(c,info,184,24,s->roots[root].category==LIB_MEDIA_FOLDER?"Music, Pictures and Video":library_category_name(s->roots[root].category),false);path=s->roots[root].path;}
        else theme_label(c,info,193,23,d->blade==BLADE_MEDIA?"Add a folder and choose\nits scan depth.\n\nMusic, Pictures and Video\nare filtered by file type.\nSaved folders scan at startup.":"Add a folder to search for\ngames, apps or homebrew.\n\nA saved folder is scanned\nwhen Blades starts.",false);
    } else if(d->library_page==LIB_UI_BROWSE) {
        theme_label(c,info,140,23,d->blade==BLADE_MEDIA?"A: Open folder\nX: Use current folder\nB: Parent folder\n\nNext: choose scan depth.\nFile types are sorted\nautomatically.":"A: Open folder\nX: Use current folder\nB: Parent folder\n\nChoose scan depth after\nchoosing a category.",false);
        if(s)path=s->browse_path;
    } else if(d->library_page==LIB_UI_CATEGORY) {
        theme_label(c,info,140,23,"Choose where titles from\nthis folder should appear.\n\nArcade games and demos\nare identified automatically\nin a Games folder.",false);path=d->library_path;
    } else if(d->library_page==LIB_UI_ROOT_ACTION) {
        theme_label(c,info,140,23,"Change how many levels\nof subfolders are scanned.\n\nRemoving a scan folder\nkeeps your games and files\non the device.",false);
        if(s&&d->library_root<(int)s->root_count)path=s->roots[d->library_root].path;
    } else if(d->library_page==LIB_UI_FILES) {
        const LibraryFolder *f=s&&row<(int)s->folder_count?s->folders+row:NULL;
        theme_label(c,info,140,24,s&&s->busy?"Working...":!f?"":f->executable?"Xbox 360 executable":f->is_file?"File":"Folder",false);
        theme_label(c,info,184,23,f&&f->is_file&&!f->executable?"This file cannot be launched.\nB: Parent folder\nX: Refresh":"A: Open folder / launch XEX\nB: Parent folder\nX: Refresh\n\nNo scan folder is needed.",false);
        if(f)path=f->path;else if(s)path=s->browse_path;
    } else if(d->library_page==LIB_UI_DEPTH) {
        theme_label(c,info,140,23,"0: Selected folder only\n1: Include child folders\n2: Include their children\n\nDefault: 3 levels\nUnlimited searches every\nsubfolder.",false);
        if(d->library_depth_edit&&s&&d->library_root<(int)s->root_count)path=s->roots[d->library_root].path;else path=d->library_path;
    }
    int path_y=d->library_page==LIB_UI_DEPTH&&!scan_details?410:d->library_page==LIB_UI_FILES&&!scan_details?380:335;
    if(path)library_path(c,info,path_y,path,s&&s->status_error?470:550);
    if(!s)theme_label(c,info,510,22,"Library is unavailable.",false);
    else if(s->status_error)library_path(c,info,490,s->status,580);
    footer(c,SUBMENU_LEFT,SUBMENU_RIGHT,true,&d->disc,d);
}
static bool achievement_tile(Canvas *c,const Dashboard *d,bool game,unsigned index,int x,int y,int side) {
    const AchievementSnapshot *s=d->achievements;if(!s)return false;
    static unsigned ids[ACH_TILES];
    for(unsigned n=game?ACH_GRID:0;n<(game?ACH_GRID+3:ACH_GRID);n++) {
        const AchievementTile *tile=s->tiles+n;
        if(!tile->valid||tile->index!=index||(!game&&tile->title!=s->title_id))continue;
        if(!ids[n])ids[n]=canvas_image_id();
        canvas_image_tagged(c,(DrawImage){.pixels=tile->pixels,.id=ids[n],.revision=s->revision,.width=64,.height=64,.format=IMAGE_RGBA},x,y,side,side);return true;
    }
    return false;
}
static void achievement_content(Canvas *c,const Dashboard *d) {
    const AchievementSnapshot *s=d->achievements;int game=d->achievement_game;
    theme_label(c,SUBMENU_CONTENT+14,22,35,"Achievements",true);
    const AchievementTitle *title=s&&game>=0&&game<(int)s->title_count?s->titles+game:NULL;
    int first=game/3*3;
    /* Preserve the existing artwork/row spacing. Inset only the focus box
       below the preceding two-pixel divider, leaving a clear gap at each end. */
    if(s)for(int n=0;n<3&&first+n<(int)s->title_count;n++) {
        int y=202+n*116;bool selected=first+n==game;
        if(selected){theme_round(c,154,y-4,144,108,8,d->achievement_grid?0x18ffffff:0x45ffffff);theme_outline(c,154,y-4,144,108,8,0x60000000);}
        if(!achievement_tile(c,d,true,(unsigned)(first+n),182,y+4,88))resources_draw_fit(c,"ico_64x_controller.png",182,y+4,88,88);
        if(n<2&&first+n+1<(int)s->title_count){fill_rect(c,154,y+108,144,1,0x35000000);fill_rect(c,154,y+109,144,1,0x18ffffff);}
    }
    if(title){
        theme_round(c,310,92,800,88,10,0x16ffffff);theme_outline(c,310,92,800,88,10,0x65000000);
        theme_label_fit(c,328,98,28,500,title->name);
        char label[80],date[16];snprintf(label,sizeof label,"%u achievement%s",title->earned,title->earned==1?"":"s");theme_label(c,328,138,25,label,false);
        achievement_date(title->played,date);theme_label(c,1092-text_width(25,date),98,25,date,false);
        snprintf(label,sizeof label,"%u / %u",title->score_earned,title->score_possible);int size=26;while(size>16&&text_width(size,label)>280)size--;
        theme_label(c,1060-text_width(size,label),138+(26-size)/2,size,label,false);resources_draw_fit(c,"GScore.png",1068,137,30,30);
    }
    if(!s)theme_label(c,342,264,28,"Achievements are unavailable.\nPress B to return.",false);
    else if(s->busy)theme_label(c,342,264,28,"Loading achievements...",false);
    else if(s->error){char error[112];snprintf(error,sizeof error,"Achievements could not be read.\nPress X to refresh. (Code %u)",s->error_code);theme_label(c,342,264,26,error,false);}
    else if(!s->title_count)theme_label(c,342,264,28,"No played games found\nfor this profile.",false);
    else if(!achievement_grid_count(s))theme_label(c,342,264,28,title&&title->earned?"Achievement details are unavailable.\nPress X to refresh.":"Unlock achievements by\nplaying games!",false);
    else{
        for(int n=0;n<ACH_GRID&&d->achievement_scroll+n<(int)achievement_grid_count(s);n++){
            unsigned index=(unsigned)(d->achievement_scroll+n);const AchievementItem *item=achievement_grid_item(s,index,NULL);int x=342+n%6*128,y=202+n/6*116;
            bool selected=d->achievement_grid&&index==(unsigned)d->achievement_index;
            theme_round(c,x,y,96,96,10,selected?0x60ffffff:0x14000000);theme_outline(c,x,y,96,96,10,selected?0xe0ffffff:0x85000000);
            if(!achievement_tile(c,d,false,index,x+10,y+10,76))resources_draw_fit(c,item->earned?"ico_64x_trophy.png":item->secret?"secretAchievement.png":"unearnedAchievement.png",x+10,y+10,76,76);
        }
        char count[48];snprintf(count,sizeof count,"%u of %u",(unsigned)d->achievement_index+1,achievement_grid_count(s));if(d->achievement_grid)theme_label(c,1066-text_width(22,count),558,22,count,false);
        if(d->achievement_scroll>0)resources_draw_fit(c,"scroll.png",1008,566,24,20);
        if(d->achievement_scroll+ACH_GRID<(int)achievement_grid_count(s)){line(c,1060,574,1070,584,0xb0ffffff);line(c,1070,584,1080,574,0xb0ffffff);}
    }
    if(first>0)resources_draw_fit(c,"scroll.png",204,568,24,20);
    if(s&&first+3<(int)s->title_count){line(c,238,574,248,584,0xb0ffffff);line(c,248,584,258,574,0xb0ffffff);}
    if(s&&s->truncated)theme_label(c,328,588,20,"Some entries are beyond the display limit.",false);
    footer(c,SUBMENU_LEFT,SUBMENU_RIGHT,true,&d->disc,d);
}
/* Return the first free row so long saved login values push the following
   status field down without colliding or extending into the footer. */
static int ftp_login_text(Canvas *c,int x,int y,int size,const char *value){
    while(*value){char line[FTP_LOGIN_BYTES];size_t n=0;
        while(value[n]&&n<sizeof line-1){line[n]=value[n];line[n+1]=0;if(text_width(size,line)>400&&n)break;n++;}
        line[n]=0;theme_label(c,x,y,size,line,false);value+=n;y+=size==27?36:26;
    }
    return y;
}
static void info_paragraph(Canvas *c,int x,int y,int width,int bottom,const char *text){
    char line[256]={0};size_t used=0;
    while(*text){while(*text==' '||*text=='\n')text++;if(!*text)break;
        const char *word=text;while(*text&&*text!=' '&&*text!='\n')text++;size_t length=(size_t)(text-word);
        if(length+used+2>=sizeof line)return;
        char candidate[256];memcpy(candidate,line,used);size_t next=used;if(next)candidate[next++]=' ';memcpy(candidate+next,word,length);next+=length;candidate[next]=0;
        if(used&&text_width(28,candidate)>width){theme_label(c,x,y,28,line,false);y+=38;if(y+32>bottom)return;used=0;line[0]=0;}
        if(used)line[used++]=' ';
        memcpy(line+used,word,length);used+=length;line[used]=0;
    }
    if(used&&y+32<=bottom)theme_label(c,x,y,28,line,false);
}
/* Server descriptions stay inside the right panel, including unbroken words.
   Ellipsis makes truncation visible without shifting the list or its footer. */
static int jelly_text(Canvas *c,int x,int y,int width,int size,unsigned lines,const char *text){
    for(unsigned row=0;row<lines&&*text;row++){
        while(*text==' '||*text=='\n'||*text=='\r'||*text=='\t')text++;
        if(!*text)break;
        char line[256]={0};size_t n=0,last=0;
        while(text[n]&&text[n]!='\n'&&n<sizeof line-8){
            size_t count=((unsigned char)text[n]<128)?1:((unsigned char)text[n]<224)?2:((unsigned char)text[n]<240)?3:4;
            if(n+count>=sizeof line-8||strlen(text+n)<count)break;
            memcpy(line+n,text+n,count);line[n+count]=0;
            if(text_width(size,line)>width){line[n]=0;break;}
            if(text[n]==' ')last=n;
            n+=count;
        }
        if(!n){n=1;line[0]='?';line[1]=0;}
        if(text[n]&&text[n]!='\n'&&last){n=last;line[n]=0;}
        text+=n;while(*text==' '||*text=='\n'||*text=='\r'||*text=='\t')text++;
        if(row+1==lines&&*text){size_t end=strlen(line);if(end>sizeof line-4)end=sizeof line-4;
            while(end){line[end]=0;char shortened[256];memcpy(shortened,line,end);memcpy(shortened+end,"...",4);
                if(text_width(size,shortened)<=width){memcpy(line,shortened,end+4);break;}
                do{end--;}while(end&&((unsigned char)line[end]&192)==128);
            }
        }
        theme_label(c,x,y,size,line,false);y+=size+8;
    }
    return y;
}
static void jelly_details(Canvas *c,int x,const JellySnapshot *s,const xf_item *item,const char *label){
    const JellyArtwork *art=&s->artwork;int y=94;
    if(!strcmp(art->item,item->id)&&art->width&&art->height){
        static unsigned id;if(!id)id=canvas_image_id();
        float scale=fminf(285.f/art->width,176.f/art->height);int w=(int)lroundf(art->width*scale),h=(int)lroundf(art->height*scale);
        canvas_image_tagged(c,(DrawImage){.pixels=art->pixels,.id=id,.revision=art->revision,.width=(int)art->width,.height=(int)art->height,.format=IMAGE_RGBA},x+(285-w)/2,y+(176-h)/2,w,h);
    }else{
        fill_rect(c,x,y,285,176,0x24203242);theme_label(c,x+42,y+73,23,!strcmp(art->item,item->id)?"No artwork available":"Loading artwork...",false);
    }
    y=jelly_text(c,x,284,285,25,2,label);
    char runtime[64];jellyfin_runtime(item->ticks,runtime,sizeof runtime);theme_label(c,x,y+4,23,runtime,false);y+=42;
    jelly_text(c,x,y,285,23,(unsigned)((566-y)/31),item->overview[0]?item->overview:"No description available.");
}
static void scene_content(Canvas *c,const Dashboard *d) {
    if(dashboard_update_page(d)){
        int x=SUBMENU_CONTENT,info=710,hy=87+(int)lroundf(d->highlight*MENU_ROW_PITCH);
        theme_label(c,x+14,22,35,"Update Blades",true);menu_top_divider(c,x,87,495);theme_menu_upper_shadow(c,x,87,495,hy+MENU_TOP_OFFSET,true);
        selection(c,x,hy+MENU_TOP_OFFSET,495,MENU_FILL_HEIGHT,theme_color(BLADE_SYSTEM));theme_menu_shadow(c,x,hy+MENU_SHADOW_OFFSET,495,610,true);
        for(int i=0;i<2;i++){int y=87+i*MENU_ROW_PITCH;menu_divider(c,x,y,483,menu_focus_covers_divider(y+MENU_DIVIDER_OFFSET,hy));theme_label(c,x+14,y+5,29,i?"Update and Restart":"Check for Updates",false);}
        theme_label(c,info,94,25,"Installed version",false);theme_label(c,info,130,27,BLADES_VERSION,false);
        theme_label(c,info,180,25,"Latest release",false);theme_label(c,info,216,27,d->update.version[0]?d->update.version:"Not checked",false);
        jelly_text(c,info,277,400,24,3,d->update.message);
        if(d->update.state==UPDATE_DOWNLOADING||d->update.state==UPDATE_PREPARING){char progress[96];unsigned percent=d->update.total?(unsigned)(100*d->update.received/d->update.total):0;
            snprintf(progress,sizeof progress,"%u%%",percent);theme_label(c,info,399,24,progress,false);
            fill_rect(c,info,441,400,10,0x502b3239);fill_rect(c,info,441,(int)(4*percent),10,0xffd5e6ee);theme_label(c,info,479,23,"X: Cancel",false);
        }
        footer(c,SUBMENU_LEFT,SUBMENU_RIGHT,true,&d->disc,d);return;
    }
    if(d->jelly_page){
        const JellySnapshot *s=d->jelly;bool settings=d->jelly_page==JELLY_UI_SETTINGS;
        int x=SUBMENU_CONTENT,width=settings?495:625,info=settings?710:815;
        int n=dashboard_jellyfin_count(d),row=settings?d->child:d->jelly_row,scroll=settings?0:d->jelly_scroll;
        theme_label(c,x+14,22,35,settings?"Media Center":"Jellyfin",true);
        if(n){int hy=87+(int)lroundf(d->highlight*MENU_ROW_PITCH);
            menu_top_divider(c,x,87,width);theme_menu_upper_shadow(c,x,87,width,hy+MENU_TOP_OFFSET,true);
            selection(c,x,hy+MENU_TOP_OFFSET,width,MENU_FILL_HEIGHT,theme_color(BLADE_MEDIA));theme_menu_shadow(c,x,hy+MENU_SHADOW_OFFSET,width,settings?610:571,true);
            for(int i=scroll;i<n&&i<scroll+LIBRARY_VISIBLE;i++){int y=87+(i-scroll)*MENU_ROW_PITCH;menu_divider(c,x,y,width-12,menu_focus_covers_divider(y+MENU_DIVIDER_OFFSET,hy));
                bool folder=!settings&&s&&(unsigned)i<s->page.count&&s->page.items[i].folder;
                if(folder){fill_rect(c,x+14,y+18,23,17,0x8ce9f2f6);fill_rect(c,x+14,y+13,10,6,0x8ce9f2f6);}
                theme_label_fit(c,x+(folder?47:14),y+5,28,width-(folder?63:30),dashboard_jellyfin_label(d,i));}
        }else theme_label(c,x+14,96,27,d->jelly_wait||(s&&s->busy)?"Loading...":s&&s->state==JELLY_CONNECTED?"No videos found":"Not connected",false);
        if(settings){
            theme_label(c,info,94,27,"Jellyfin status",false);theme_label_fit(c,info,130,27,400,jellyfin_status(s));
            theme_label(c,info,180,27,"Server address",false);theme_label_fit(c,info,216,24,400,s&&s->server[0]?s->server:"Not set");
            theme_label(c,info,266,27,"Username",false);theme_label_fit(c,info,302,24,400,s&&s->username[0]?s->username:"Not set");
            const char *help=row==0?"Enter IPv4:port, such as\n10.0.0.20:8096.\nUses the native HTTP connection.":row==1?"Your Jellyfin account name.":row==2?"Enter your Jellyfin password.\nPasswords are not saved.":row==4?"Remove this server and\nits saved login from Blades.":"Connect to browse videos.\nThe saved session reconnects\nwhen Blades starts.";
            theme_label(c,info,365,23,help,false);
        }else{
            const xf_item *item=s&&row>=0&&(unsigned)row<s->page.count?s->page.items+row:NULL;
            if(item&&jellyfin_playable(item))jelly_details(c,info,s,item,dashboard_jellyfin_label(d,row));
            else {
            theme_label_fit(c,info,94,25,285,s&&s->title[0]?s->title:"Jellyfin");
            char count[80];snprintf(count,sizeof count,"%u - %u of %u",s&&s->page.count?s->start+1:0,s?s->start+s->page.count:0,s?s->page.total:0);theme_label(c,info,137,23,count,false);
            const char *kind=!item?"":item->folder?"Folder":!strcmp(item->type,"Episode")?"TV Episode":"Video";
            theme_label(c,info,185,24,kind,false);
            if(item){theme_label(c,info,230,23,item->folder?"A: Open folder":"A: Play video",false);
                if(item->year){char year[32];snprintf(year,sizeof year,"%u",item->year);theme_label(c,info,275,23,year,false);}}
            if(!s||s->state!=JELLY_CONNECTED)theme_label(c,info,230,23,"Set up your server under\nMedia Center.",false);
            }
        }
        if(s&&s->status[0]&&(settings||s->state!=JELLY_CONNECTED||s->busy||d->jelly_wait||d->jelly_row<0||(unsigned)d->jelly_row>=s->page.count||!jellyfin_playable(s->page.items+d->jelly_row)))library_path(c,info,settings?500:420,s->status,570);
        if(s&&s->save_error)theme_label(c,info,575,20,"Could not save settings.",false);
        footer(c,SUBMENU_LEFT,SUBMENU_RIGHT,true,&d->disc,d);return;
    }
    if(dashboard_ftp_page(d)){
        int x=SUBMENU_CONTENT;char devices[32];snprintf(devices,sizeof devices,"%u",d->ftp.devices);
        theme_label(c,x+14,22,35,"Computers",true);
        int hy=87+(int)lroundf(d->highlight*MENU_ROW_PITCH);
        menu_top_divider(c,x,87,495);
        theme_menu_upper_shadow(c,x,87,495,hy+MENU_TOP_OFFSET,true);
        selection(c,x,hy+MENU_TOP_OFFSET,495,MENU_FILL_HEIGHT,theme_color(BLADE_SYSTEM));theme_menu_shadow(c,x,hy+MENU_SHADOW_OFFSET,495,610,true);
        for(int row=0;row<3;row++) {
            int divider=87+row*MENU_ROW_PITCH+MENU_DIVIDER_OFFSET;
            bool covered=menu_focus_covers_divider(divider,hy);
            menu_divider(c,x,87+row*MENU_ROW_PITCH,483,covered);
        }
        theme_label(c,x+14,92,29,d->ftp.enabled?"Turn FTP Off":"Turn FTP On",false);
        theme_label(c,x+14,145,29,"Change Username",false);theme_label(c,x+14,198,29,"Change Password",false);
        const char *username=d->ftp.username[0]?d->ftp.username:FTP_USERNAME;
        const char *password=d->ftp.password[0]?d->ftp.password:FTP_PASSWORD;
        /* One label/value gap (36) and one value/next-label gap (50),
           matching Server status -> IP. Wrap before advancing the next field. */
        int size=text_width(27,username)<=400&&text_width(27,password)<=400?27:20;
        int line_height=size==27?36:26;
        theme_label(c,710,94,27,"Server status",false);theme_label_fit(c,710,130,size,400,ftp_status(&d->ftp));
        theme_label(c,710,180,27,"IP address",false);theme_label(c,710,216,size,d->ftp.ip[0]?d->ftp.ip:"Unavailable",false);
        theme_label(c,710,266,27,"Username",false);
        int end=ftp_login_text(c,710,302,size,username);
        int password_y=end-line_height+50;
        theme_label(c,710,password_y,27,"Password",false);
        end=ftp_login_text(c,710,password_y+36,size,password);
        int devices_y=end-line_height+50;
        theme_label(c,710,devices_y,27,"Connected devices",false);theme_label(c,710,devices_y+36,size,devices,false);
        footer(c,SUBMENU_LEFT,SUBMENU_RIGHT,true,&d->disc,d);return;
    }
    if(d->achievement_page){achievement_content(c,d);return;}
    if(d->library_page){library_content(c,d);return;}
    int b=d->blade,left=theme_left(b),right=theme_right(b);
    if(d->depth){left=SUBMENU_LEFT;right=SUBMENU_RIGHT;}
    int x=d->depth?SUBMENU_CONTENT:theme_content_left(b),menu_y=blade_has_profile(b)?238:96;
    uint32_t base=theme_color(b);
    theme_label(c,x+(d->depth?14:5),22,35,d->depth?menu_label(b,0,0,d->selected[b]):blade_names[b],true);
    int row=d->depth?d->child:d->selected[b];
    if(blade_has_profile(b)&&!d->depth)gamer_card(c,b,x,d);
    if(d->depth)menu_y=87;
    int menu_x=d->depth?x:x+6,menu_w=d->depth?495:400;
    int n=menu_count(b,d->depth,d->selected[b]);
    int visible=n-(!d->depth&&blade_has_tray(b)?1:0);
    int hy=menu_y+(int)lroundf(d->highlight*MENU_ROW_PITCH);
    if(visible)menu_top_divider(c,menu_x,menu_y,menu_w);
    if(row<visible&&!d->profile_card_selected) {
        theme_menu_upper_shadow(c,menu_x,menu_y,menu_w,hy+MENU_TOP_OFFSET,true);
        selection(c,menu_x,hy+MENU_TOP_OFFSET,menu_w,MENU_FILL_HEIGHT,base);
        /* Live closes at its last button's divider, before the logo tile.
           Selecting that button leaves no lower group for a downwards curve. */
        int end=!d->depth&&b==BLADE_LIVE?menu_y+(visible-1)*MENU_ROW_PITCH+MENU_DIVIDER_OFFSET:
            (!d->depth&&blade_has_tray(b))?526:610;
        theme_menu_shadow(c,menu_x,hy+MENU_SHADOW_OFFSET,menu_w,end,true);
    }
    for(int i=0;i<visible;i++) {
        int y=menu_y+i*MENU_ROW_PITCH;
        /* Keep other row grooves. The selected lower rule is covered by
           its shadow; during motion, omit rules inside the contact band. */
        int divider_y=y+MENU_DIVIDER_OFFSET;
        bool has_shadow=row<visible&&!d->profile_card_selected;
        bool live_bottom=!d->depth&&b==BLADE_LIVE&&i==visible-1;
        int divider_w=has_shadow&&!live_bottom?menu_w-12:menu_w;
        bool covered=has_shadow&&!live_bottom&&menu_focus_covers_divider(divider_y,hy);
        menu_divider(c,menu_x,y,divider_w,covered);
        const char *image=NULL;
        if(!d->depth) {
            if(b==BLADE_LIVE) {
                const char *names[]={"ico_32x_Mail.png","ico_32x_Friend.png","ico_32x_messenger.png"};
                if(i<3)image=names[i];
            } else if(b==BLADE_GAMES) {
                if(i==0)image="ico_64x_trophy.png";
                if(i==1)image="ico_64x_controller.png";
            } else if(b==BLADE_MEDIA) {
                if(i==1)image="ico_32x_camera.PNG";
                if(i==4)image="ico_32x_MCX.png";
            } else if(b==BLADE_SYSTEM&&i==4)image="ico_32x_PC.png";
        }
        char local_icon[64];snprintf(local_icon,sizeof local_icon,"skin/icon/%d/%d.png",b,i);
        if(!d->depth) {
            bool local=resources_draw_fit(c,local_icon,x+10,y+4,40,40);
            if(!local&&(!image||!resources_draw_fit(c,image,x+10,y+4,40,40)))
                theme_icon(c,x+14,y+6,icon_for(b,i),0xffe1e8ef);
        }
        theme_label(c,d->depth?menu_x+14:x+68,y+5,29,menu_label(b,d->depth,d->selected[b],i),false);
    }
    if(!d->depth&&blade_has_tray(b))tray(c,b,x,right,row==n-1&&!d->profile_card_selected,base,&d->disc);
    int info=d->depth?685:x+(b==BLADE_SYSTEM?427:b==BLADE_LIVE?434:420);
    if(!d->depth) {
        bool tray_selected=blade_has_tray(b)&&row==n-1&&!d->profile_card_selected;
        bool heading=(b==BLADE_LIVE||b==BLADE_MARKETPLACE)&&!tray_selected;
        if(heading)theme_label(c,info,menu_y+1,29,blade_names[b],false);
        const char *details=d->profile_card_selected?(profile_signed_in(d->profile)?"View your Xbox profile\nin the Xbox Guide.":"Sign in to use your\nXbox profile."):description(b,row);
        if(blade_has_tray(b)&&row==n-1&&!d->profile_card_selected) {
            static const char *const descriptions[]={"Insert a game, DVD, or\nCD.","Opening","Insert a game, DVD, or\nCD.","Closing","Reading","Play the game in the\nconsole's disc drive.","This disc cannot be played.\nPress Y to eject.","The disc could not be read.\nPress Y to eject.","The disc drive is unavailable."};
            details=descriptions[d->disc.state<=DISC_UNAVAILABLE?d->disc.state:DISC_UNAVAILABLE];
            if(d->disc.state==DISC_READY) {
                if(d->disc.media==DISC_MEDIA_AUDIO_CD)details="Play this audio CD using\nthe Xbox system music player.";
                else if(d->disc.media==DISC_MEDIA_DVD_VIDEO)details="Play this DVD using the\nXbox system DVD player.";
                else if(d->disc.media==DISC_MEDIA_DATA)details="Open this data disc in the\nXbox system dashboard.";
            }
        }
        if(b==BLADE_LIVE&&!tray_selected)info_paragraph(c,info,heading?menu_y+41:menu_y+2,theme_right(b)-info-28,526,details);
        else theme_label(c,info,heading?menu_y+41:menu_y+2,28,details,false);
        if(blade_has_tray(b)&&row==n-1&&d->disc.state==DISC_READING) {
            char image[48];snprintf(image,sizeof image,"disc/reading-%02u.png",disc_reading_frame(d->disc_elapsed));
            resources_draw(c,image,info+100,menu_y+96,96,96);
        }
        if(b==BLADE_LIVE) {
            /* Five transparent pixels around the 405x108 panel preserve its
               subtle outer shadow instead of clipping it to the bevel. */
            bool panel=resources_draw(c,"branding/live-panel.png",x-5,400,415,118);
            if(!panel) {
                theme_round(c,x,405,405,108,12,0xff4c524e);
                theme_outline(c,x,405,405,108,12,0xff606a62);
            }
            if(!resources_draw_fit(c,"livelogo.png",x+172,418,152,84))brand(c,BLADE_LIVE,x+200,426);
            if(!panel)for(int r=26;r<=66;r+=10) {
                for(int t=0;t<70;t++) {
                    float a=(float)t*3.14159f/70-1.57f;
                    int px=x-4+(int)(cosf(a)*r),py=461+(int)(sinf(a)*r);
                    if(py>410&&py<507&&px>x+4)fill_rect(c,px,py,2,1,0xff82913b);
                }
            }
        }
    } else {
        theme_label(c,info,menu_y+7,27,"Blades",false);
        theme_label(c,info,menu_y+55,24,b==BLADE_GAMES&&d->selected[b]==1?
            "Launch games, apps and\nhomebrew from your storage.\n\nChoose Scan Folders to add\nyour library locations.":
            "This menu uses demo data.\nSystem services are not\nimplemented in this build.",false);
    }
    footer(c,left,right,d->depth!=0,&d->disc,d);
}
static void overlay(Canvas *c,const Dashboard *d) {
    if(!d->dialog&&!d->exit_menu)return;
    fill_rect(c,0,0,WIDTH,HEIGHT,0x75000000);
    theme_round(c,163,211,954,302,15,0xff9ba7b2);
    theme_round(c,166,214,948,296,14,theme_mix(theme_color(d->blade),0xffe7eadf,0.65f));
    theme_label(c,196,237,30,d->exit_menu?"Exit Dashboard":"Blades",false);
    if(d->exit_menu) {
        selection(c,194,312+d->child*62,852,MENU_DIVIDER_OFFSET,theme_color(d->blade));
        theme_label(c,216,317,27,"Cancel",d->child==0);
        theme_label(c,216,379,27,"Return to system dashboard",d->child==1);
    } else theme_label(c,196,302,24,d->message,false);
}
static uint32_t *scratch_buffer(void) {
    static uint32_t *pixels;
    if(!pixels)pixels=malloc(WIDTH*HEIGHT*sizeof(uint32_t));
    return pixels;
}
static void content(Canvas *c,const Dashboard *d,float opacity) {
    if((d->library_page||d->jelly_page==JELLY_UI_BROWSE)&&!d->startup_active)opacity*=anim_smooth(d->list_transition);
    if(opacity>0) {
        if(c->ops&&opacity<1)canvas_emit(c,(DrawOp){.kind=DRAW_FADE_BEGIN});
        uint32_t *base=!c->ops&&opacity<1?scratch_buffer():NULL;
        if(base)memcpy(base,c->pixels,WIDTH*HEIGHT*sizeof(uint32_t));
        scene_content(c,d);
        if(c->ops&&opacity<1)canvas_emit(c,(DrawOp){.kind=DRAW_FADE_END,.amount=opacity});
        if(base) {
            unsigned alpha=(unsigned)(anim_clamp(opacity)*256),inverse=256-alpha;
            for(unsigned i=0;i<WIDTH*HEIGHT;i++) {
                uint32_t a=base[i],b=c->pixels[i];
                uint32_t rb=(((a&0x00ff00ff)*inverse+(b&0x00ff00ff)*alpha)>>8)&0x00ff00ff;
                uint32_t g=(((a&0x0000ff00)*inverse+(b&0x0000ff00)*alpha)>>8)&0x0000ff00;
                c->pixels[i]=0xff000000|rb|g;
            }
        }
    }
}
static void paint(Canvas *c,const Dashboard *d,AnimationCaps caps,float depth,float opacity) {
    theme_background(c,d->blade,d->elapsed);
    theme_frame(c,d->blade,depth,caps.top,caps.bottom);
    content(c,d,opacity);
    theme_submenu_chrome(c,d->blade,depth,d->depth==0);
}
void dashboard_render(Canvas *c,const Dashboard *d) {
    if(d->video_view){dashboard_video_render(c,d);return;}
    const AnimationCaps normal={68,610};
    if(d->startup_active) {
        StartupFrame frame=startup_sample(d->startup_elapsed);
        theme_background(c,BLADE_GAMES,d->elapsed);
        resources_draw_opacity(c,"startup/reveal.png",0,0,WIDTH,HEIGHT,(unsigned)(frame.glow*255));
        if(!skin_startup_base(c,d->startup_elapsed)) {
            theme_frame(c,BLADE_GAMES,0,normal.top,normal.bottom);
            theme_submenu_chrome(c,BLADE_GAMES,0,false);
        }
        content(c,d,frame.content);
        if(!skin_startup_shutters(c,d->startup_elapsed)) {
            int left=(int)frame.left,right=(int)frame.right;
            unsigned color=((unsigned)(frame.shutters*255)<<24)|0xb9bec4;
            fill_rect(c,0,0,left,HEIGHT,color);fill_rect(c,right,0,WIDTH-right,HEIGHT,color);
        }
        return;
    }
    if(d->transition<1&&d->previous_blade!=d->blade) {
        float seconds=d->transition*BLADE_SECONDS;
        Dashboard old=*d;old.blade=d->previous_blade;old.depth=0;
        old.jelly_page=JELLY_UI_NONE;
        old.library_page=LIB_UI_NONE;
        old.highlight=(float)d->selected[old.blade];
        if(seconds<BLADE_LEAF_START) {
            paint(c,&old,normal,0,1-seconds/CONTENT_OUT);
        } else if(seconds<BLADE_LEAF_END) {
            float progress=anim_phase(seconds,BLADE_LEAF_START,BLADE_LEAF_END);
            theme_background(c,old.blade,d->elapsed);
            uint32_t *incoming=c->ops?NULL:scratch_buffer();
            if(incoming||c->ops) {
                int moving=d->blade>old.blade?d->blade:old.blade;
                int from=(int)theme_rail_position(old.blade,moving);
                int to=(int)theme_rail_position(d->blade,moving);
                int edge=(int)(from+(to-from)*progress);
                if(edge<0)edge=0;
                if(edge>WIDTH)edge=WIDTH;
                if(c->ops)canvas_emit(c,(DrawOp){.kind=DRAW_AMBIENT,.x=d->blade>old.blade?edge:0,
                    .w=d->blade>old.blade?WIDTH-edge:edge,.color=(uint32_t)d->blade});
                else {
                  Canvas next={.pixels=incoming};theme_background(&next,d->blade,d->elapsed);
                  for(int y=0;y<HEIGHT;y++) {
                    if(d->blade>old.blade)memcpy(c->pixels+y*WIDTH+edge,incoming+y*WIDTH+edge,(WIDTH-edge)*sizeof(uint32_t));
                    else memcpy(c->pixels+y*WIDTH,incoming+y*WIDTH,edge*sizeof(uint32_t));
                  }
                }
            } else theme_background(c,d->blade,d->elapsed);
            theme_leaf(c,old.blade,d->blade,progress);
        } else {
            paint(c,d,anim_caps(seconds,false,false),0,anim_phase(seconds,13.0f/30,16.0f/30));
        }
    } else if(d->menu_transition<1&&d->previous_depth!=d->depth) {
        float seconds=d->menu_transition*MENU_SECONDS;
        if(seconds<CONTENT_OUT) {
            Dashboard old=*d;old.depth=d->previous_depth;
            old.library_page=old.depth?d->previous_library_page:LIB_UI_NONE;
            old.jelly_page=old.depth?d->previous_jelly_page:JELLY_UI_NONE;
            old.achievement_page=old.depth?d->previous_achievement_page:false;
            old.highlight=(float)(old.depth?old.child:old.selected[old.blade]);
            paint(c,&old,normal,(float)old.depth,1-seconds/CONTENT_OUT);
        } else {
            float depth=anim_menu_depth(seconds,d->previous_depth,d->depth);
            paint(c,d,anim_caps(seconds,true,d->depth==0),depth,
                anim_phase(seconds,MENU_CONTENT_START,MENU_CONTENT_END));
        }
    } else paint(c,d,normal,(float)d->depth,1);
    overlay(c,d);
}
