#include "blades.h"
#include "resources.h"
#include "gl_renderer.h"
#include "startup.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static double worst_mean;static unsigned worst_peak,states;
static void save(Canvas *c,const char *name,unsigned n) {
    const char *dir=getenv("BLADES_GPU_SNAPSHOTS");if(!dir)return;
    char path[1024];snprintf(path,sizeof path,"%s/%03u-%s.ppm",dir,n,name);FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n%d %d\n255\n",WIDTH,HEIGHT);
    for(int i=0;i<WIDTH*HEIGHT;i++){uint32_t p=c->pixels[i];unsigned char rgb[]={p>>16,p>>8,p};assert(fwrite(rgb,1,3,f)==3);}
    assert(!fclose(f));
}
static void check(GlRenderer *r,Dashboard *d,Canvas *cpu,Canvas *gpu) {
    dashboard_render(cpu,d);assert(gl_renderer_frame(r,d,gpu));
    unsigned peak=0,over8=0;uint64_t error=0;
    for(int i=0;i<WIDTH*HEIGHT;i++) {
        unsigned pixel_peak=0;
        for(unsigned s=0;s<=16;s+=8) {
            unsigned e=(unsigned)abs((int)((cpu->pixels[i]>>s)&255)-(int)((gpu->pixels[i]>>s)&255));
            error+=e;if(e>pixel_peak)pixel_peak=e;if(e>peak)peak=e;
        }
        over8+=pixel_peak>8;
    }
    double mean=(double)error/(WIDTH*HEIGHT*3);
    if(mean>worst_mean)worst_mean=mean;if(peak>worst_peak)worst_peak=peak;
    printf("state=%u blade=%d depth=%d transition=%.3f menu=%.3f mean=%.6f peak=%u over8=%u\n",states,d->blade,d->depth,d->transition,d->menu_transition,mean,peak,over8);fflush(stdout);
    save(cpu,"cpu",states);save(gpu,"gpu",states);states++;
    assert(mean<.55&&peak<=5&&over8==0);
}
int main(int argc,char **argv) {
    const char *pack=argc>1?argv[1]:getenv("BLADES_RESOURCE_PACK");assert(pack&&resources_open(pack));
    GlRenderer *r=gl_renderer_open();assert(r);
    Canvas cpu={.pixels=malloc(WIDTH*HEIGHT*4)},gpu={.pixels=malloc(WIDTH*HEIGHT*4)};assert(cpu.pixels&&gpu.pixels);
    Dashboard d;
    JellySnapshot jelly={.state=JELLY_CONNECTED,.revision=1,.loaded=true,.page={.count=10,.total=25}};
    strcpy(jelly.server,"10.0.0.20:8096");strcpy(jelly.username,"temporary-test-account");strcpy(jelly.status,"Connected. Browse Video > Jellyfin.");strcpy(jelly.title,"Test Movies");
    for(unsigned i=0;i<10;i++){snprintf(jelly.page.items[i].name,sizeof jelly.page.items[i].name,"Test movie %u with a long name",i);jelly.page.items[i].folder=i==0;}
    dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;d.depth=d.previous_depth=1;d.selected[BLADE_MEDIA]=4;d.jelly_page=JELLY_UI_SETTINGS;dashboard_set_jellyfin(&d,&jelly);
    for(unsigned i=0;i<5;i++){d.child=(int)i;d.highlight=(float)i;check(r,&d,&cpu,&gpu);}
    d.selected[BLADE_MEDIA]=2;d.jelly_page=JELLY_UI_BROWSE;
    for(unsigned i=0;i<3;i++){d.jelly_row=i?9:0;d.jelly_scroll=i?2:0;jelly.busy=i==2;jelly.revision++;dashboard_set_jellyfin(&d,&jelly);check(r,&d,&cpu,&gpu);}
    d.jelly_row=1;d.jelly_scroll=0;jelly.busy=false;
    strcpy(jelly.page.items[1].id,"11111111111111111111111111111111");strcpy(jelly.page.items[1].type,"Movie");
    strcpy(jelly.page.items[1].overview,"A long description wraps into the right hand panel without crossing the list or the footer. The selected artwork must keep its proportions and the runtime remains visible. Extra text ends with an ellipsis when there is no space for another line.");jelly.page.items[1].ticks=61200000000ULL;
    strcpy(jelly.artwork.item,jelly.page.items[1].id);jelly.artwork.width=100;jelly.artwork.height=150;
    for(unsigned i=0;i<sizeof jelly.artwork.pixels;i+=4){jelly.artwork.pixels[i]=32;jelly.artwork.pixels[i+1]=132;jelly.artwork.pixels[i+2]=205;jelly.artwork.pixels[i+3]=255;}
    jelly.artwork.revision++;jelly.revision++;dashboard_set_jellyfin(&d,&jelly);check(r,&d,&cpu,&gpu);
    jelly.artwork.width=288;jelly.artwork.height=176;jelly.artwork.revision++;jelly.revision++;dashboard_set_jellyfin(&d,&jelly);check(r,&d,&cpu,&gpu);
    jelly.artwork.width=0;jelly.artwork.height=0;jelly.page.items[1].overview[0]=0;jelly.page.items[1].ticks=0;jelly.revision++;dashboard_set_jellyfin(&d,&jelly);check(r,&d,&cpu,&gpu);
    d.jelly_page=JELLY_UI_NONE;dashboard_library_open(&d,LIB_UI_VIDEO_SOURCES,LIB_VIDEO);check(r,&d,&cpu,&gpu);d.library_row=1;check(r,&d,&cpu,&gpu);
    d.list_transition=.4f;d.highlight=.32f;check(r,&d,&cpu,&gpu);d.list_transition=.8f;d.highlight=.68f;check(r,&d,&cpu,&gpu);d.list_transition=1;d.highlight=1;check(r,&d,&cpu,&gpu);
    dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.position=d.origin=d.target=BLADE_SYSTEM;d.depth=d.previous_depth=1;d.selected[BLADE_SYSTEM]=6;d.update=(UpdateSnapshot){.state=UPDATE_DOWNLOADING,.total=6000000,.received=2600000,.revision=1};strcpy(d.update.version,"0.51.0");strcpy(d.update.message,"Downloading the USB ZIP...");check(r,&d,&cpu,&gpu);
    dashboard_init(&d);d.blade=BLADE_SYSTEM;d.position=d.target=d.origin=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=4;d.depth=d.previous_depth=1;
    for(int state=FTP_OFF;state<=FTP_ERROR;state++){
        d.ftp=(FtpSnapshot){.state=(FtpState)state,.enabled=state!=FTP_OFF,.devices=state==FTP_RUNNING?2:0,.port=21};
        snprintf(d.ftp.ip,sizeof d.ftp.ip,"192.168.1.50");check(r,&d,&cpu,&gpu);
    }
    d.ftp.save_error=true;check(r,&d,&cpu,&gpu);d.ftp.enabled=false;d.ftp.state=FTP_OFF;check(r,&d,&cpu,&gpu);
    /* Long editable credentials must remain inside the status column in
       both renderers, with either edit button highlighted. */
    d.ftp=(FtpSnapshot){.state=FTP_RUNNING,.enabled=true,.devices=4,.port=21};
    snprintf(d.ftp.ip,sizeof d.ftp.ip,"192.168.1.50");
    memset(d.ftp.username,'W',63);memset(d.ftp.password,'W',63);
    for(int row=1;row<=2;row++){d.child=row;d.highlight=(float)row;check(r,&d,&cpu,&gpu);}
    /* FTP uses the same interpolated cursor as other menus. */
    const float ftp_rows[]={0,.25f,.75f,1,1.5f,2};
    strcpy(d.ftp.username,"blades");strcpy(d.ftp.password,"blades");
    for(unsigned i=0;i<sizeof ftp_rows/sizeof ftp_rows[0];i++){d.child=2;d.highlight=ftp_rows[i];check(r,&d,&cpu,&gpu);}
    /* Upper group grows/shrinks while the highlight slides, including the
       first-row boundary. Use the actual foreground command cache. */
    dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=2;
    const float rows[]={0,.25f,.75f,1,1.5f,2};
    for(unsigned i=0;i<sizeof rows/sizeof rows[0];i++){d.highlight=rows[i];check(r,&d,&cpu,&gpu);}
    /* Cross the rounding boundary inside the same truncated pixel in both
       directions. The GPU's cached foreground must drop Chat's lower lip. */
    dashboard_init(&d);d.blade=d.previous_blade=BLADE_LIVE;d.selected[BLADE_LIVE]=2;d.elapsed=3.25f;
    const float chat_pixels[]={105.25f,105.75f,106.25f,106.75f,106.25f,105.75f};
    for(unsigned i=0;i<sizeof chat_pixels/sizeof chat_pixels[0];i++){d.highlight=chat_pixels[i]/53;check(r,&d,&cpu,&gpu);}
    LibraryEntry titles[16]={0};LibraryFolder folders[12]={0};LibrarySnapshot library={.entries=titles,.count=16,.folders=folders,.folder_count=12,.root_count=1,.revision=1};
    snprintf(library.status,sizeof library.status,"16 titles found");snprintf(library.browse_path,sizeof library.browse_path,"hdd:/Games");
    snprintf(library.roots[0].path,LIBRARY_PATH,"hdd:/Games/Some long folder name");
    for(unsigned i=0;i<16;i++){snprintf(titles[i].name,sizeof titles[i].name,"Example Game %02u - long title for clipping and scrolling",i);snprintf(titles[i].path,LIBRARY_PATH,"hdd:/Games/Folder %02u/default.xex",i);}
    for(unsigned i=0;i<12;i++)snprintf(folders[i].name,sizeof folders[i].name,"Game folder %u",i);
    for(int page=LIB_UI_LIST;page<=LIB_UI_FILES;page++) {
        dashboard_init(&d);d.depth=d.previous_depth=1;d.selected[BLADE_GAMES]=1;d.elapsed=4.25f;dashboard_set_library(&d,&library);
        dashboard_library_open(&d,(LibraryPage)page,LIB_ALL);snprintf(d.library_path,sizeof d.library_path,"hdd:/Games/Folder to add");
        check(r,&d,&cpu,&gpu);
        if(page==LIB_UI_LIST||page==LIB_UI_BROWSE){dashboard_update(&d,UP,0);check(r,&d,&cpu,&gpu);}
        if(page==LIB_UI_FILES){
            folders[1]=(LibraryFolder){.name="App.XEX",.path="hdd:/Games/App.XEX",.is_file=true,.executable=true};
            folders[2]=(LibraryFolder){.name="cover.png",.path="hdd:/Games/cover.png",.is_file=true};
            folders[3]=(LibraryFolder){.name="module.dll.xex",.path="hdd:/Games/module.dll.xex",.is_file=true};
            for(int row=1;row<=3;row++){d.library_row=row;library.revision++;dashboard_set_library(&d,&library);check(r,&d,&cpu,&gpu);}
        }
        d.library_wait=true;check(r,&d,&cpu,&gpu);
    }
    library.busy=true;library.progress=(LibraryProgress){.scanning=true,.folders=18,.files=43,.found=0,.depth=3,.kind=LIB_SCAN_FILE};
    snprintf(library.progress.path,LIBRARY_PATH,"hdd:/Games/An intentionally long directory path to exercise the visible scan filename/default.xex");
    for(int stop=0;stop<2;stop++) {
        library.progress.stopping=stop!=0;library.revision++;dashboard_init(&d);d.depth=d.previous_depth=1;d.selected[BLADE_GAMES]=1;d.elapsed=5.5f;dashboard_set_library(&d,&library);dashboard_library_open(&d,LIB_UI_ROOTS,LIB_ALL);check(r,&d,&cpu,&gpu);
    }
    library.busy=false;library.progress.scanning=false;library.progress.found=42;library.count=0;library.revision++;
    dashboard_init(&d);d.depth=d.previous_depth=1;d.selected[BLADE_GAMES]=1;d.elapsed=5.75f;dashboard_set_library(&d,&library);dashboard_library_open(&d,LIB_UI_ROOTS,LIB_ALL);d.library_row=(int)library.root_count+1;d.library_scan_details=true;check(r,&d,&cpu,&gpu);
    dashboard_update(&d,UP,0);check(r,&d,&cpu,&gpu);
    library.status_error=true;snprintf(library.status,sizeof library.status,"The storage device could not be read.");library.revision++;dashboard_set_library(&d,&library);check(r,&d,&cpu,&gpu);library.status_error=false;
    library.busy=false;library.progress.scanning=false;library.revision++;
    library.count=library.folder_count=0;library.revision++;
    for(int page=LIB_UI_LIST;page<=LIB_UI_BROWSE;page++) {
        dashboard_init(&d);d.depth=d.previous_depth=1;d.elapsed=5;dashboard_set_library(&d,&library);dashboard_library_open(&d,(LibraryPage)page,LIB_APPS);check(r,&d,&cpu,&gpu);
    }
    for(int blade=BLADE_LIVE;blade<=BLADE_MEDIA;blade++)for(int state=DISC_EMPTY;state<=DISC_UNAVAILABLE;state++) {
        dashboard_init(&d);d.blade=d.previous_blade=blade;d.elapsed=3.25f;
        d.selected[blade]=menu_count(blade,0,0)-1;d.highlight=(float)d.selected[blade];
        d.disc.state=(DiscState)state;
        if(state==DISC_READY)snprintf(d.disc.title,sizeof d.disc.title,"Forza Motorsport 2 - a deliberately very long disc title to check clipping");
        check(r,&d,&cpu,&gpu);
        if(state==DISC_READING)for(unsigned frame=1;frame<16;frame++) {
            d.disc_elapsed=frame/27.2f+.001f;check(r,&d,&cpu,&gpu);
        }
    }
    const DiscMedia media_kinds[]={DISC_MEDIA_AUDIO_CD,DISC_MEDIA_DVD_VIDEO,DISC_MEDIA_DATA};
    for(unsigned i=0;i<3;i++) {
        dashboard_init(&d);d.disc.state=DISC_READY;d.disc.media=media_kinds[i];d.selected[BLADE_GAMES]=4;d.highlight=4;d.elapsed=6;check(r,&d,&cpu,&gpu);
    }
    ProfileSnapshot profile={.identity={.state=PROFILE_LOCAL,.xuid=42,.name="BladesAtomic"},.known=7,.games=1,.score=65432,.achievements=1234,.has_picture=true,.revision=1,.picture_revision=1,.available_profiles=2,.profiles_known=true};
    for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){unsigned at=(y*64+x)*4;profile.picture[at]=x*4;profile.picture[at+1]=y*4;profile.picture[at+2]=160;profile.picture[at+3]=255;}
    for(int blade=BLADE_LIVE;blade<=BLADE_MEDIA;blade++) {
        dashboard_init(&d);d.blade=d.previous_blade=blade;d.elapsed=4;d.profile_card_selected=true;ProfileSnapshot signed_out={.identity={.state=PROFILE_SIGNED_OUT},.available_profiles=2,.profiles_known=true,.revision=1};dashboard_set_profile(&d,&signed_out);check(r,&d,&cpu,&gpu);
        dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);
        dashboard_update(&d,PROFILE,0);check(r,&d,&cpu,&gpu);dashboard_update(&d,DOWN,0);check(r,&d,&cpu,&gpu);
    }
    dashboard_init(&d);profile.identity.state=PROFILE_LIVE;snprintf(profile.identity.name,sizeof profile.identity.name,"A deliberately long profile name for clipping");profile.score=2147483647;profile.achievements=2147483647;profile.games=2147483647;profile.revision++;dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);
    profile.known=0;profile.revision++;dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);
    profile.known=7;profile.revision++;profile.picture_revision++;memset(profile.picture,220,sizeof profile.picture);dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);
    profile.identity=(ProfileIdentity){.state=PROFILE_SIGNED_OUT};profile.known=0;profile.has_picture=false;profile.revision++;dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);
    for(unsigned count=0;count<3;count++){profile.available_profiles=count;profile.profiles_known=true;profile.revision++;dashboard_set_profile(&d,&profile);check(r,&d,&cpu,&gpu);}
    AchievementSnapshot *ach=calloc(1,sizeof *ach);assert(ach);
    dashboard_init(&d);d.depth=d.previous_depth=1;d.achievement_page=true;ach->busy=true;ach->revision=1;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);
    ach->busy=false;ach->error=true;ach->error_code=5;ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);
    ach->error=false;ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);
    ach->title_count=5;ach->item_count=47;ach->title_id=123;
    for(unsigned n=0;n<5;n++){ach->titles[n]=(AchievementTitle){.id=123+n,.earned=47,.possible=70,.score_earned=920,.score_possible=1500,.played=132223104000000000ULL};snprintf(ach->titles[n].name,sizeof ach->titles[n].name,"Example Game %u",n+1);}
    for(unsigned n=0;n<47;n++){AchievementItem *item=ach->items+n;item->earned=n<40;item->secret=n>=40;item->score=10;item->unlocked=132223104000000000ULL;snprintf(item->name,sizeof item->name,"%s %u",item->secret?"Secret Achievement":"Example Achievement",n+1);snprintf(item->description,sizeof item->description,"Example description to exercise line wrapping, selected detail rendering and a score read from the profile metadata.");}
    for(unsigned n=0;n<ACH_TILES;n++){AchievementTile *tile=ach->tiles+n;tile->valid=true;tile->title=n<ACH_GRID?123:123+n-ACH_GRID;tile->index=n<ACH_GRID?n:n-ACH_GRID;for(unsigned p=0;p<64*64;p++){tile->pixels[p*4]=n*11;tile->pixels[p*4+1]=p%64*4;tile->pixels[p*4+2]=p/64*4;tile->pixels[p*4+3]=255;}}
    achievement_prepare(ach);ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);d.achievement_grid=true;check(r,&d,&cpu,&gpu);
    for(unsigned i=0;i<3;i++){d.achievement_scroll=(int)i*18;d.achievement_index=d.achievement_scroll;for(unsigned n=0;n<ACH_GRID;n++)ach->tiles[n].index=i*18+n;ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);}
    d.achievement_grid=true;d.achievement_index=0;check(r,&d,&cpu,&gpu);
    d.achievement_grid=false;d.achievement_game=1;check(r,&d,&cpu,&gpu);d.achievement_game=2;check(r,&d,&cpu,&gpu);d.achievement_game=3;ach->title_id=126;ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);
    ach->truncated=true;ach->revision++;dashboard_set_achievements(&d,ach);check(r,&d,&cpu,&gpu);free(ach);d.achievements=NULL;
    dashboard_begin_startup(&d);
    for(unsigned frame=0;frame<=35;frame++){d.startup_elapsed=frame/30.f;d.elapsed=d.startup_elapsed;check(r,&d,&cpu,&gpu);}
    dashboard_init(&d);assert(gl_renderer_frame(r,&d,&cpu));d.elapsed=1.0f/60;
    assert(gl_renderer_frame(r,&d,&gpu));
    for(int i=0;i<WIDTH*HEIGHT;i++)assert(cpu.pixels[i]==gpu.pixels[i]);
    d.elapsed=2;assert(gl_renderer_frame(r,&d,&gpu));unsigned moved=0;
    for(int i=0;i<WIDTH*HEIGHT;i++)moved+=cpu.pixels[i]!=gpu.pixels[i];
    assert(moved>WIDTH*HEIGHT/3);
    for(int b=0;b<BLADE_COUNT;b++)for(int row=0;row<menu_count(b,0,0);row++) {
        dashboard_init(&d);d.blade=d.previous_blade=b;d.selected[b]=row;d.highlight=(float)row;d.elapsed=2.125f;
        check(r,&d,&cpu,&gpu);d.depth=d.previous_depth=1;d.highlight=0;check(r,&d,&cpu,&gpu);
    }
    const float phases[]={0,1.0f/30,2,5,37.125f,784};
    for(int b=0;b<BLADE_COUNT;b++)for(unsigned t=0;t<sizeof phases/sizeof phases[0];t++) {
        dashboard_init(&d);d.blade=d.previous_blade=b;d.elapsed=phases[t];check(r,&d,&cpu,&gpu);
    }
    const float times[]={0,.0667f,.1333f,.2f,.2667f,.3333f,.4f,.4667f,.5333f,.7f,.9f};
    for(int kind=0;kind<3;kind++)for(unsigned i=0;i<sizeof times/sizeof times[0];i++) {
        dashboard_init(&d);d.elapsed=5.75f;
        if(kind==0){dashboard_update(&d,RIGHT,0);d.transition=times[i]/.55f;if(d.transition>1)d.transition=1;}
        else {d.previous_depth=kind==1?0:1;d.depth=kind==1?1:0;d.menu_transition=times[i]/.94f;if(d.menu_transition>1)d.menu_transition=1;}
        check(r,&d,&cpu,&gpu);
    }
    dashboard_init(&d);d.elapsed=37.125f;dashboard_update(&d,PROFILE,0);check(r,&d,&cpu,&gpu);
    dashboard_init(&d);d.elapsed=784;dashboard_update(&d,EXIT,0);check(r,&d,&cpu,&gpu);
    /* Media apps use filtered catalogues; Video Marketplace manages only
       media paths even when game and media roots are interleaved. */
    LibraryEntry media[3]={0};LibrarySnapshot ms={.entries=media,.count=3,.root_count=4,.revision=1};
    const char *media_names[]={"A long music file name for list clipping","A family photo","A video clip"};
    for(int i=0;i<3;i++){media[i].kind=LIB_MEDIA;media[i].category=(LibraryCategory)(LIB_MUSIC+i);snprintf(media[i].name,sizeof media[i].name,"%s",media_names[i]);snprintf(media[i].path,LIBRARY_PATH,"usb0:/Media/%s",media_names[i]);}
    for(int i=0;i<4;i++){ms.roots[i].category=i==1?LIB_GAMES:(LibraryCategory)(LIB_MUSIC+(i==0?0:i-1));ms.roots[i].max_depth=LIBRARY_DEPTH_ALL;snprintf(ms.roots[i].path,LIBRARY_PATH,"usb0:/Folder %d",i);}
    for(int i=0;i<4;i++) {
        dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;d.selected[BLADE_MEDIA]=i;d.depth=d.previous_depth=1;d.elapsed=3.25f;dashboard_set_library(&d,&ms);
        dashboard_library_open(&d,i==3?LIB_UI_ROOTS:LIB_UI_LIST,(LibraryCategory)(LIB_MUSIC+(i%3)));check(r,&d,&cpu,&gpu);
        if(i<3){ms.count=0;ms.revision++;dashboard_set_library(&d,&ms);check(r,&d,&cpu,&gpu);ms.count=3;ms.revision++;}
    }
    const LibraryPage pages[]={LIB_UI_CATEGORY,LIB_UI_DEPTH,LIB_UI_ROOT_ACTION};
    for(unsigned i=0;i<sizeof pages/sizeof pages[0];i++){dashboard_library_open(&d,pages[i],LIB_VIDEO);check(r,&d,&cpu,&gpu);}
    /* Actual YUV atlas shader: 1080p Y + side-by-side U/V, BT601/709,
       limited/full range, every RGB channel against scalar bilinear reference. */
    unsigned vw=1920,vh=1080;unsigned char *yuv=malloc(vw*vh*3/2);assert(yuv);
    for(unsigned y=0;y<vh;y++)for(unsigned x=0;x<vw;x++)yuv[y*vw+x]=(unsigned char)(16+x*219/(vw-1));
    for(unsigned y=0;y<vh/2;y++)for(unsigned x=0;x<vw/2;x++){yuv[(vh+y)*vw+x]=(unsigned char)(80+y*96/(vh/2-1));yuv[(vh+y)*vw+vw/2+x]=(unsigned char)(176-x*96/(vw/2-1));}
    for(unsigned format=VIDEO_YUV601;format<=VIDEO_YUV709_FULL;format++){
        for(unsigned y=0;y<vh/2;y++)for(unsigned x=0;x<vw/2;x++){yuv[(vh+y)*vw+x]=(unsigned char)(80+y*96/(vh/2-1));yuv[(vh+y)*vw+vw/2+x]=(unsigned char)(176-x*96/(vw/2-1));}
        dashboard_init(&d);d.video_view=true;d.video_has_picture=true;d.video_controls=false;
        d.video_picture=(VideoPicture){yuv,vw,vh,0,format,0,16./9,(VideoPixelFormat)format};check(r,&d,&cpu,&gpu);
        for(unsigned y=0;y<vh/2;y++)for(unsigned x=0;x<vw/2;x++){yuv[(vh+y)*vw+x]=128;yuv[(vh+y)*vw+vw/2+x]=128;}
        d.video_picture.revision+=10;check(r,&d,&cpu,&gpu);
    }
    free(yuv);
    gl_renderer_close(r);resources_close();free(cpu.pixels);free(gpu.pixels);
    printf("PASS: %u desktop GPU comparisons; worst mean %.6f, peak %u; Xbox GPU performance unmeasured\n",states,worst_mean,worst_peak);
}
