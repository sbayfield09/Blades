#include "blades.h"
#include "draw_list.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static void put32(unsigned char *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static void write_bytes(const char *path,const void *p,size_t n){FILE *f=fopen(path,"wb");assert(f&&fwrite(p,1,n,f)==n&&!fclose(f));}
static void xex(const char *path,uint32_t id) {
    unsigned char p[0x400]={0};memcpy(p,"XEX2",4);put32(p+8,sizeof p);put32(p+16,0x100);put32(p+20,1);
    put32(p+24,0x40006);put32(p+28,0x80);put32(p+0x8c,id);write_bytes(path,p,sizeof p);
}
static void package(const char *path,uint32_t type,const char *title) {
    unsigned char p[0x1711]={0};memcpy(p,"LIVE",4);put32(p+0x344,type);put32(p+0x360,0x58410001);
    for(size_t i=0;i<strlen(title);i++)p[0x411+i*2+1]=(unsigned char)title[i];write_bytes(path,p,sizeof p);
}
static void path(char out[LIBRARY_PATH],const char *a,const char *b){assert(library_join(out,a,b));}
static void mkdir_child(char out[LIBRARY_PATH],const char *a,const char *b){path(out,a,b);assert(!mkdir(out,0700));}
static void remove_tree(const char *p) {
    DIR *d=opendir(p);assert(d);struct dirent *e;while((e=readdir(d))) {
        if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        char child[LIBRARY_PATH];path(child,p,e->d_name);struct stat st;assert(!lstat(child,&st));
        if(S_ISDIR(st.st_mode))remove_tree(child);else assert(!unlink(child));
    }
    closedir(d);assert(!rmdir(p));
}
static const LibrarySnapshot *idle(LibraryService *l,unsigned after) {
    struct timespec delay={0,1000000};
    for(unsigned i=0;i<5000;i++){const LibrarySnapshot *s=library_poll(l);if(s&&s->revision>after&&!s->busy)return s;nanosleep(&delay,NULL);}
    assert(!"library worker timed out");return NULL;
}
typedef struct {atomic_bool entered,release;atomic_uint calls;} Fake;
static bool prepare(void *ctx,const LibraryEntry *e,char *launch,char *error,size_t size) {
    (void)error;(void)size;Fake *f=ctx;atomic_fetch_add(&f->calls,1);atomic_store(&f->entered,true);
    struct timespec delay={0,1000000};while(!atomic_load(&f->release))nanosleep(&delay,NULL);
    snprintf(launch,LIBRARY_PATH,"%s",e->path);return true;
}
static void filesystem(void) {
    char temp[]="/tmp/blades-library-XXXXXX";assert(mkdtemp(temp));
    LibrarySnapshot s={0};char games[LIBRARY_PATH],apps[LIBRARY_PATH],homebrew[LIBRARY_PATH],folder[LIBRARY_PATH],file[LIBRARY_PATH],deep[LIBRARY_PATH];
    mkdir_child(games,temp,"Games");mkdir_child(apps,temp,"Apps");mkdir_child(homebrew,temp,"Homebrew");
    mkdir_child(folder,games,"Zeta Game");path(file,folder,"default.xex");xex(file,0x4d5307e6);
    path(file,folder,"module.xex");xex(file,0x4d5307e6); /* one entry, not two */
    snprintf(deep,sizeof deep,"%s",games);
    for(unsigned i=0;i<180;i++){char next[LIBRARY_PATH];mkdir_child(next,deep,"d");snprintf(deep,sizeof deep,"%s",next);}
    path(file,deep,"default.xex");xex(file,0x11111111); /* beyond usual scan depths */
    path(file,games,"arcade");package(file,0xd0000,"Alpha Arcade");
    path(file,games,"god");package(file,0x7000,"Beta Download");
    path(file,games,"demo");package(file,0x80000,"Charlie Demo");
    path(file,games,"save");package(file,1,"Not a game");
    path(file,games,"dlc");package(file,0x20000,"Not a game");
    path(file,games,"update");package(file,0xb0000,"Not a game");
    path(file,games,"blades.xex");xex(file,0x424c4453);
    path(file,games,"invalid.xex");write_bytes(file,"XEX2",4);
    path(file,games,"loop");assert(!symlink(games,file));
    mkdir_child(folder,games,"god.data");path(file,folder,"Data0000");package(file,0x7000,"Ignore payload");
    path(file,apps,"Application.xex");xex(file,0x22222222);
    path(file,homebrew,"Utility.xex");xex(file,0x33333333);
    path(file,homebrew,"default.xex");xex(file,0x424c4453); /* Blades beside other apps must not hide them */
    s.root_count=4;
    snprintf(s.roots[0].path,LIBRARY_PATH,"%s",games);
    snprintf(s.roots[1].path,LIBRARY_PATH,"%s",games); /* overlapping root */
    snprintf(s.roots[2].path,LIBRARY_PATH,"%s",apps);s.roots[2].category=LIB_APPS;
    snprintf(s.roots[3].path,LIBRARY_PATH,"%s",homebrew);s.roots[3].category=LIB_HOMEBREW;
    for(size_t i=0;i<s.root_count;i++)s.roots[i].max_depth=LIBRARY_DEPTH_ALL;
    assert(library_scan(&s,NULL,NULL,NULL));assert(s.count==7);
    assert(library_count(&s,LIB_ALL)==5&&library_count(&s,LIB_APPS)==1&&library_count(&s,LIB_HOMEBREW)==1);
    assert(library_count(&s,LIB_ARCADE)==1&&library_count(&s,LIB_DEMOS)==1);
    assert(!strcmp(library_entry(&s,LIB_ALL,0)->name,"Alpha Arcade"));
    assert(!strcmp(library_entry(&s,LIB_ALL,4)->name,"Zeta Game"));
    assert(!library_entry(&s,LIB_ALL,5));
    char config[LIBRARY_PATH];path(config,temp,"Config/library-paths.txt");assert(library_write_config(config,&s));
    LibrarySnapshot loaded={0};assert(library_read_config(config,&loaded)&&loaded.root_count==4);
    assert(!strcmp(loaded.roots[2].path,apps)&&loaded.roots[2].category==LIB_APPS);
    Fake fake={0};atomic_store(&fake.release,true);LibraryHooks hooks={.context=&fake,.prepare=prepare};
    LibraryService *l=library_open(config,NULL,&hooks);assert(l);const LibrarySnapshot *front=idle(l,0);assert(front->count==7);
    LibraryCommand c={.type=LIB_CMD_BROWSE};snprintf(c.path,sizeof c.path,"%s",games);unsigned revision=front->revision;
    assert(library_request(l,&c));front=idle(l,revision);assert(front->folder_count==3);assert(!strcmp(front->browse_path,games));
    /* Selection is explicit: scans and folder browsing cannot invoke launch. */
    assert(!atomic_load(&fake.calls));char launch[LIBRARY_PATH];assert(!library_take_launch(l,launch));
    c=(LibraryCommand){.type=LIB_CMD_LAUNCH,.entry=*library_entry(front,LIB_APPS,0)};revision=front->revision;
    assert(library_request(l,&c));front=idle(l,revision);assert(front->launch_ready&&atomic_load(&fake.calls)==1);
    assert(library_take_launch(l,launch)&&!strcmp(launch,c.entry.path));assert(!library_take_launch(l,launch));
    atomic_store(&fake.release,false);atomic_store(&fake.entered,false);revision=front->revision;
    assert(library_request(l,&c));struct timespec delay={0,1000000};
    for(unsigned i=0;!atomic_load(&fake.entered)&&i<5000;i++)nanosleep(&delay,NULL);assert(atomic_load(&fake.entered));
    library_cancel_launch(l);atomic_store(&fake.release,true);front=idle(l,revision);assert(!front->launch_ready&&!library_take_launch(l,launch));
    assert(!unlink(c.entry.path));revision=front->revision;assert(library_request(l,&c));front=idle(l,revision);
    assert(!front->launch_ready&&atomic_load(&fake.calls)==2); /* removed before launch */
    c=(LibraryCommand){.type=LIB_CMD_REMOVE,.root=3};revision=front->revision;assert(library_request(l,&c));front=idle(l,revision);
    assert(front->root_count==3&&library_count(front,LIB_HOMEBREW)==0);assert(library_read_config(config,&loaded)&&loaded.root_count==3);
    path(file,homebrew,"Utility.xex");assert(!access(file,F_OK)); /* remove only the scan path */
    /* A failed config write leaves the previous settings intact. */
    snprintf(loaded.roots[0].path,LIBRARY_PATH,"bad\npath");assert(!library_write_config(config,&loaded));
    assert(library_read_config(config,&loaded)&&loaded.root_count==3&&!strcmp(loaded.roots[0].path,games));
    library_close(l);
    write_bytes(config,"BLADES-LIBRARY-1\n0\tbad",22);assert(!library_read_config(config,&loaded)&&!loaded.root_count);
    s.roots[0].path[0]=0;s.root_count=1;assert(!library_scan(&s,NULL,NULL,NULL)&&s.errors&&s.count==0);
    library_snapshot_free(&s);library_snapshot_free(&loaded);remove_tree(temp);
}
static void press(Dashboard *d,unsigned a){dashboard_update(d,a,1.f/60);}
static void navigation(void) {
    LibraryEntry entries[15]={0};LibraryFolder folder={.name="My Games",.path="hdd:/My Games"};
    LibrarySnapshot s={.entries=entries,.count=15,.revision=1,.folders=&folder,.folder_count=1};
    for(unsigned i=0;i<15;i++){snprintf(entries[i].name,sizeof entries[i].name,"Game %u",i);snprintf(entries[i].path,sizeof entries[i].path,"hdd:/Games/%u/default.xex",i);entries[i].title_id=i+1;}
    Dashboard d;dashboard_init(&d);dashboard_set_library(&d,&s);
    press(&d,DOWN);press(&d,SELECT);assert(d.depth&&d.child==0);press(&d,SELECT);assert(d.library_page==LIB_UI_LIST);
    assert(!d.library_command.type);press(&d,UP);assert(d.library_row==14&&d.library_scroll==6);
    press(&d,DOWN);assert(!d.library_row&&!d.library_scroll);
    press(&d,SELECT);assert(d.library_command.type==LIB_CMD_LAUNCH&&d.library_command.entry.title_id==1);
    press(&d,SELECT);assert(!d.library_command.type);s.revision++;dashboard_set_library(&d,&s);assert(!d.library_wait);
    press(&d,BACK);assert(!d.library_page&&d.depth&&d.child==0);
    press(&d,DOWN);press(&d,DOWN);press(&d,DOWN);press(&d,DOWN);press(&d,SELECT);assert(d.library_page==LIB_UI_ROOTS);
    press(&d,SELECT);assert(d.library_page==LIB_UI_BROWSE&&d.library_command.type==LIB_CMD_BROWSE);
    s.revision++;dashboard_set_library(&d,&s);press(&d,SELECT);assert(!strcmp(d.library_command.path,"hdd:/My Games"));
    snprintf(s.browse_path,sizeof s.browse_path,"hdd:/My Games");s.revision++;dashboard_set_library(&d,&s);
    press(&d,PROFILE);assert(d.library_page==LIB_UI_CATEGORY);press(&d,DOWN);press(&d,SELECT);
    assert(d.library_page==LIB_UI_DEPTH&&d.library_row==LIBRARY_DEPTH_DEFAULT+1);press(&d,SELECT);
    assert(d.library_command.max_depth==LIBRARY_DEPTH_DEFAULT);
    assert(d.library_page==LIB_UI_ROOTS&&d.library_command.type==LIB_CMD_ADD&&d.library_command.category==LIB_APPS&&!strcmp(d.library_command.path,"hdd:/My Games"));
    s.revision++;s.busy=true;dashboard_set_library(&d,&s);press(&d,PROFILE);assert(!d.library_command.type);
    s.revision++;s.busy=false;dashboard_set_library(&d,&s);d.disc.state=DISC_READY;press(&d,EJECT);assert(d.disc_command==DISC_EJECT);
    press(&d,BACK);assert(!d.library_page&&d.depth);press(&d,BACK);assert(!d.depth);
    press(&d,DOWN);press(&d,SELECT);assert(d.library_page==LIB_UI_LIST&&d.library_filter==LIB_ARCADE);
    press(&d,SELECT);assert(!d.library_command.type);press(&d,BACK);assert(!d.depth&&!d.library_page);
    /* The display list must refresh after snapshot contents change in place. */
    d.selected[BLADE_GAMES]=1;d.depth=1;dashboard_library_open(&d,LIB_UI_LIST,LIB_ALL);DrawList list={0};
    assert(draw_list_update(&list,&d));assert(!draw_list_update(&list,&d));s.revision++;snprintf(entries[0].name,sizeof entries[0].name,"Changed title");dashboard_set_library(&d,&s);assert(draw_list_update(&list,&d));
    draw_list_close(&list);
    static const struct {const char *from,*to;} cases[]={
        {"hdd:/Games/Deep","hdd:/Games"},{"hdd:/Games","hdd:/"},{"hdd:/",""},
        {"usb0:/Games","usb0:/"},{"/Device/Harddisk0/Partition1/Games","/Device/Harddisk0/Partition1/"},
        {"/Device/Harddisk0/Partition1/",""},{"/Device/Mass0PartitionFile/Storage/",""},{"/tmp/a","/tmp"}};
    for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){char p[LIBRARY_PATH];snprintf(p,sizeof p,"%s",cases[i].from);library_parent(p);assert(!strcmp(p,cases[i].to));}
}
typedef struct {atomic_bool entered,release;char path[LIBRARY_PATH];} ProgressWait;
static void hold_progress(void *context,const LibraryProgress *p) {
    ProgressWait *w=context;
    if(p->scanning&&p->kind==LIB_SCAN_FILE&&!atomic_exchange(&w->entered,true)) {
        snprintf(w->path,sizeof w->path,"%s",p->path);
        struct timespec delay={0,1000000};while(!atomic_load(&w->release))nanosleep(&delay,NULL);
    }
}
static void depth_empty_and_homebrew(void) {
    char temp[]="/tmp/blades-depth-XXXXXX";assert(mkdtemp(temp));char p[LIBRARY_PATH],child[LIBRARY_PATH],level2[LIBRARY_PATH],level3[LIBRARY_PATH],config[LIBRARY_PATH];
    path(p,temp,"No?TitleID.xex");xex(p,0);char no_id[LIBRARY_PATH];snprintf(no_id,sizeof no_id,"%s",p);
    mkdir_child(child,temp,"Child");path(p,child,"NoExecutionInfo.xex");xex(p,0);
    FILE *f=fopen(p,"r+b");assert(f);unsigned char zero[4]={0};assert(!fseek(f,20,SEEK_SET)&&fwrite(zero,1,4,f)==4&&!fclose(f));
    mkdir_child(level2,child,"Level2");path(p,level2,"Normal.xex");xex(p,0x12345678);
    mkdir_child(level3,level2,"Level3");path(p,level3,"arcade");package(p,0xd0000,"A?rcade");
    LibrarySnapshot s={.root_count=1};snprintf(s.roots[0].path,LIBRARY_PATH,"%s",temp);
    for(unsigned depth=0;depth<=3;depth++){s.roots[0].max_depth=depth;assert(library_scan(&s,NULL,NULL,NULL)&&s.count==depth+1);assert(!s.progress.scanning&&s.progress.depth<=depth);}
    const LibraryEntry *e=library_entry(&s,LIB_ALL,2);assert(e);
    bool missing=false,zero_id=false,clean=false;
    for(size_t i=0;i<s.count;i++){e=s.entries+i;if(!strcmp(e->name,"NoExecutionInfo")){missing=true;assert(!e->title_id);}if(!strcmp(e->name,"NoTitleID")){zero_id=true;assert(!e->title_id&&!strcmp(e->path,no_id));}if(!strcmp(e->name,"Arcade"))clean=true;assert(!strchr(e->name,'?'));}
    assert(missing&&zero_id&&clean);
    LibrarySnapshot apps={.root_count=1};apps.roots[0].category=LIB_APPS;
    char app_dir[LIBRARY_PATH];mkdir_child(app_dir,temp,"Applications");snprintf(apps.roots[0].path,LIBRARY_PATH,"%s",app_dir);
    path(p,app_dir,"default.xex");xex(p,0x12340000);path(p,app_dir,"Utility.xex");xex(p,0);
    assert(library_scan(&apps,NULL,NULL,NULL)&&apps.count==2);library_snapshot_free(&apps);
    path(config,temp,"Config/library-paths.txt");assert(library_write_config(config,&s));
    LibrarySnapshot loaded={0};assert(library_read_config(config,&loaded)&&loaded.roots[0].max_depth==3);
    const char *bad="BLADES-LIBRARY-2\n0\t-1\thdd:/Games\n";write_bytes(config,bad,strlen(bad));assert(!library_read_config(config,&loaded));
    bad="BLADES-LIBRARY-2\n0\t33\thdd:/Games\n";write_bytes(config,bad,strlen(bad));assert(!library_read_config(config,&loaded));
    char legacy[LIBRARY_PATH+64];snprintf(legacy,sizeof legacy,"BLADES-LIBRARY-1\n0\t%s\n",temp);write_bytes(config,legacy,strlen(legacy));assert(library_read_config(config,&loaded)&&loaded.roots[0].max_depth==LIBRARY_DEPTH_DEFAULT);
    Fake launch_fake={0};atomic_store(&launch_fake.release,true);LibraryHooks launch_hooks={.context=&launch_fake,.prepare=prepare};
    assert(library_write_config(config,&s));LibraryService *l=library_open(config,NULL,&launch_hooks);assert(l);const LibrarySnapshot *front=idle(l,0);unsigned rev=front->revision;
    for(size_t i=0;i<2;i++) {
        const LibraryEntry *entry=NULL;for(size_t j=0;j<front->count;j++)if(!strcmp(front->entries[j].name,i?"NoTitleID":"NoExecutionInfo"))entry=front->entries+j;
        assert(entry&&!entry->title_id);LibraryCommand launch={.type=LIB_CMD_LAUNCH,.entry=*entry};char chosen[LIBRARY_PATH];snprintf(chosen,sizeof chosen,"%s",entry->path);
        assert(library_request(l,&launch));front=idle(l,rev);rev=front->revision;char ready[LIBRARY_PATH];assert(library_take_launch(l,ready)&&!strcmp(ready,chosen));
    }
    LibraryCommand cmd={.type=LIB_CMD_DEPTH,.root=0,.max_depth=0};assert(library_request(l,&cmd));front=idle(l,rev);assert(front->count==1&&front->roots[0].max_depth==0);library_close(l);
    char empty[LIBRARY_PATH];mkdir_child(empty,temp,"Empty");snprintf(s.roots[0].path,LIBRARY_PATH,"%s",empty);assert(library_write_config(config,&s));
    l=library_open(config,NULL,NULL);assert(l);front=idle(l,0);assert(!front->count&&!front->busy&&!front->progress.scanning&&strstr(front->status,"Scan complete"));
    Dashboard d;dashboard_init(&d);d.depth=1;dashboard_set_library(&d,front);dashboard_library_open(&d,LIB_UI_ROOTS,LIB_ALL);d.library_wait=true;
    for(unsigned i=0;i<3;i++) {
        rev=front->revision;cmd=(LibraryCommand){.type=LIB_CMD_SCAN};assert(library_request(l,&cmd));front=idle(l,rev);dashboard_set_library(&d,front);
        assert(!d.library_wait&&!front->busy&&!front->progress.scanning&&!front->count);
    }
    library_close(l);
    path(p,empty,"nothing.txt");write_bytes(p,"No runnable content",19);
    ProgressWait wait={0};LibraryHooks hooks={.progress=hold_progress,.progress_context=&wait};
    l=library_open(config,NULL,&hooks);assert(l);struct timespec delay={0,1000000};
    for(unsigned i=0;!atomic_load(&wait.entered)&&i<5000;i++)nanosleep(&delay,NULL);assert(atomic_load(&wait.entered));
    front=library_poll(l);assert(front->busy&&front->progress.scanning&&strstr(front->progress.path,"nothing.txt"));
    dashboard_set_library(&d,front);press(&d,PROFILE);assert(d.library_command.type==LIB_CMD_STOP);
    cmd=(LibraryCommand){.type=LIB_CMD_STOP};assert(library_request(l,&cmd));atomic_store(&wait.release,true);front=idle(l,front->revision);assert(!front->busy&&!front->count&&!front->progress.scanning&&strstr(front->status,"Scan stopped"));
    /* A subsequent zero-result scan must finish too, rather than inheriting cancellation. */
    rev=front->revision;cmd=(LibraryCommand){.type=LIB_CMD_SCAN};assert(library_request(l,&cmd));front=idle(l,rev);assert(!front->busy&&!front->count&&strstr(front->status,"Scan complete"));
    library_close(l);library_snapshot_free(&s);library_snapshot_free(&loaded);remove_tree(temp);
}
static bool checked_metadata(void *context,FILE *f,uint32_t *id,char *title) {
    unsigned *calls=context;(*calls)++;title[0]=0;return library_xex_header(f,id);
}
static void files_and_scan_controls(void) {
    char temp[]="/tmp/blades-files-XXXXXX";assert(mkdtemp(temp));char folder[LIBRARY_PATH],file[LIBRARY_PATH],config[LIBRARY_PATH];
    mkdir_child(folder,temp,"Folder.with.dots");path(file,folder,"Utility.XEX");xex(file,0x11223344);
    const char *skip[]={"file.dll.xex","file.exe.xex","file.any.XEX",".xex","not.an.app.xex"};
    for(size_t i=0;i<sizeof skip/sizeof skip[0];i++) {path(file,folder,skip[i]);xex(file,0x55667788);FILE *f=fopen(file,"rb");assert(f);LibraryEntry e;assert(!library_read_entry(f,file,LIB_APPS,NULL,&e)&&ftell(f)==0);fclose(f);}
    assert(library_plain_xex("hdd:/Parent.with.dots/Utility.XEX"));assert(!library_plain_xex("hdd:/Parent/file.exe.xex"));
    LibrarySnapshot s={.root_count=1};s.roots[0].category=LIB_APPS;snprintf(s.roots[0].path,LIBRARY_PATH,"%s",folder);
    unsigned calls=0;LibraryHooks counting={.context=&calls,.xex_title=checked_metadata};assert(library_scan(&s,&counting,NULL,NULL)&&s.count==1&&calls==1);
    const char *visible[]={"notes.txt","cover.png","settings.ini","archive.zip","README",".hidden","module.dll","tool.exe"};
    for(size_t i=0;i<sizeof visible/sizeof visible[0];i++){path(file,folder,visible[i]);write_bytes(file,"example",7);}
    char subfolder[LIBRARY_PATH];mkdir_child(subfolder,folder,"Z Folder.xex");
    path(config,temp,"Config/library-paths.txt");Fake fake={0};atomic_store(&fake.release,true);LibraryHooks hooks={.context=&fake,.prepare=prepare};
    LibraryService *l=library_open(config,NULL,&hooks);assert(l);const LibrarySnapshot *front=idle(l,0);unsigned revision=front->revision;
    LibraryCommand cmd={.type=LIB_CMD_FILES};snprintf(cmd.path,LIBRARY_PATH,"%s",folder);assert(library_request(l,&cmd));front=idle(l,revision);
    assert(front->folder_count==15&&!strcmp(front->folders[0].name,"Z Folder.xex")&&!front->folders[0].is_file&&!front->folders[0].executable);
    size_t runnable=0;const LibraryFolder *app=NULL;
    for(size_t i=1;i<front->folder_count;i++){
        const LibraryFolder *f=front->folders+i;assert(f->is_file);
        if(i>1)assert(strcasecmp(front->folders[i-1].name,f->name)<=0);
        if(f->executable){runnable++;app=f;assert(!strcmp(f->name,"Utility.XEX"));}
    }
    assert(runnable==1&&app);
    for(size_t i=0;i<sizeof visible/sizeof visible[0];i++){bool found=false;for(size_t n=1;n<front->folder_count;n++)if(!strcmp(front->folders[n].name,visible[i]))found=true;assert(found);}
    cmd=(LibraryCommand){.type=LIB_CMD_LAUNCH,.direct_launch=true,.entry={.kind=LIB_XEX,.category=LIB_HOMEBREW}};snprintf(cmd.entry.path,LIBRARY_PATH,"%s",app->path);revision=front->revision;
    assert(library_request(l,&cmd));front=idle(l,revision);char launch[LIBRARY_PATH];assert(library_take_launch(l,launch)&&!strcmp(launch,cmd.entry.path)&&atomic_load(&fake.calls)==1);
    path(file,folder,"newly-uploaded.txt");write_bytes(file,"new file",8);
    revision=front->revision;cmd=(LibraryCommand){.type=LIB_CMD_FILES};snprintf(cmd.path,LIBRARY_PATH,"%s",folder);
    assert(library_request(l,&cmd));front=idle(l,revision);assert(front->folder_count==16);
    bool uploaded=false;for(size_t i=0;i<front->folder_count;i++)if(!strcmp(front->folders[i].name,"newly-uploaded.txt"))uploaded=true;assert(uploaded);
    assert(!unlink(file));revision=front->revision;assert(library_request(l,&cmd));front=idle(l,revision);assert(front->folder_count==15);
    /* The folder picker still excludes files, and direct requests cannot launch non-XEX data. */
    revision=front->revision;cmd=(LibraryCommand){.type=LIB_CMD_BROWSE};snprintf(cmd.path,LIBRARY_PATH,"%s",folder);
    assert(library_request(l,&cmd));front=idle(l,revision);assert(front->folder_count==1&&!front->folders[0].is_file);
    path(file,folder,"package.bin");package(file,0xd0000,"Not an explorer executable");
    revision=front->revision;cmd=(LibraryCommand){.type=LIB_CMD_LAUNCH,.direct_launch=true,.entry={.kind=LIB_XEX}};snprintf(cmd.entry.path,LIBRARY_PATH,"%s",file);
    assert(library_request(l,&cmd));front=idle(l,revision);assert(front->status_error&&!library_take_launch(l,launch)&&atomic_load(&fake.calls)==1);
    library_close(l);library_snapshot_free(&s);remove_tree(temp);
    LibraryFolder entries[]={{.name="Folder",.path="hdd:/Folder"},{.name="App.xex",.path="hdd:/App.xex",.executable=true,.is_file=true},{.name="notes.txt",.path="hdd:/notes.txt",.is_file=true},{.name="file.dll.xex",.path="hdd:/file.dll.xex",.is_file=true}};
    s=(LibrarySnapshot){.folders=entries,.folder_count=4,.root_count=1,.revision=1};snprintf(s.roots[0].path,LIBRARY_PATH,"hdd:/Games");
    Dashboard d;dashboard_init(&d);dashboard_set_library(&d,&s);press(&d,DOWN);press(&d,SELECT);
    assert(!strcmp(menu_label(BLADE_GAMES,1,1,3),"File Manager"));
    press(&d,DOWN);press(&d,DOWN);press(&d,DOWN);press(&d,SELECT);assert(d.library_page==LIB_UI_FILES&&d.library_command.type==LIB_CMD_FILES);
    s.revision++;dashboard_set_library(&d,&s);press(&d,DOWN);press(&d,SELECT);assert(d.library_command.type==LIB_CMD_LAUNCH&&d.library_command.direct_launch&&!strcmp(d.library_command.entry.path,"hdd:/App.xex"));
    s.revision++;dashboard_set_library(&d,&s);press(&d,DOWN);press(&d,SELECT);assert(d.library_command.type==LIB_CMD_NONE&&!d.library_wait&&d.library_page==LIB_UI_FILES&&d.library_row==2);
    press(&d,DOWN);press(&d,SELECT);assert(d.library_command.type==LIB_CMD_NONE&&!d.library_wait&&d.library_row==3);
    press(&d,DOWN);press(&d,SELECT);assert(d.library_command.type==LIB_CMD_FILES&&!strcmp(d.library_command.path,"hdd:/Folder"));
    s.revision++;dashboard_set_library(&d,&s);press(&d,PROFILE);assert(d.library_command.type==LIB_CMD_FILES&&!d.library_command.direct_launch);
    s.revision++;dashboard_set_library(&d,&s);press(&d,BACK);assert(d.library_page==LIB_UI_NONE&&d.child==3);
    press(&d,DOWN);press(&d,SELECT);assert(d.library_page==LIB_UI_ROOTS);
    d.disc.state=DISC_READY;press(&d,EJECT);assert(d.library_command.type==LIB_CMD_REMOVE&&!d.disc_command&&d.library_command.root==0);
    s.revision++;dashboard_set_library(&d,&s);press(&d,DOWN);press(&d,DOWN);press(&d,SELECT);assert(d.library_command.type==LIB_CMD_SCAN&&d.library_scan_details);
    s.busy=true;s.progress=(LibraryProgress){.scanning=true,.files=50,.folders=8,.found=42};s.revision++;dashboard_set_library(&d,&s);assert(d.library_scan_details);
    s.busy=false;s.progress.scanning=false;s.count=0;s.revision++;dashboard_set_library(&d,&s);press(&d,0);assert(d.library_scan_details&&!d.library_wait);
    DrawList list={0};assert(draw_list_update(&list,&d));press(&d,UP);assert(!d.library_scan_details);assert(draw_list_update(&list,&d));press(&d,DOWN);assert(!d.library_scan_details);draw_list_close(&list);
    dashboard_library_open(&d,LIB_UI_DEPTH,LIB_APPS);assert(!strcmp(dashboard_library_label(&d,0),"Unlimited"));
    press(&d,SELECT);assert(d.library_command.type==LIB_CMD_ADD&&d.library_command.max_depth==LIBRARY_DEPTH_ALL);
}
static void media_libraries(void) {
    char temp[]="/tmp/blades-media-XXXXXX";assert(mkdtemp(temp));
    char file[LIBRARY_PATH],sub[LIBRARY_PATH],config[LIBRARY_PATH];
    const char *files[]={"Song.MP3","Track.wma","Audio.m4a","Photo.JPEG","Image.png","Picture.bmp","Animation.gif","Scan.tiff","Movie.MP4","Clip.wmv","Film.avi","Video.mov","notes.txt","bad.mp3.exe","archive.zip","default.xex"};
    for(size_t i=0;i<sizeof files/sizeof files[0];i++){path(file,temp,files[i]);write_bytes(file,"fixture",7);}
    mkdir_child(sub,temp,"Nested");path(file,sub,"Deep.mp3");write_bytes(file,"fixture",7);
    path(file,temp,"loop");assert(!symlink(temp,file));
    LibrarySnapshot s={.root_count=4};
    for(unsigned i=0;i<4;i++){snprintf(s.roots[i].path,LIBRARY_PATH,"%s",temp);s.roots[i].category=i==3?LIB_GAMES:(LibraryCategory)(LIB_MUSIC+i);s.roots[i].max_depth=0;}
    assert(library_scan(&s,NULL,NULL,NULL));
    assert(library_count(&s,LIB_MUSIC)==3&&library_count(&s,LIB_PICTURES)==5&&library_count(&s,LIB_VIDEO)==4&&library_count(&s,LIB_ALL)==0);
    assert(s.count==12&&!s.progress.scanning);
    s.roots[0].max_depth=LIBRARY_DEPTH_ALL;assert(library_scan(&s,NULL,NULL,NULL));assert(library_count(&s,LIB_MUSIC)==4&&s.count==13);
    for(size_t i=0;i<s.count;i++)assert(s.entries[i].kind==LIB_MEDIA&&library_media_supported(s.entries[i].path,s.entries[i].category));
    assert(!library_media_supported("folder.mp3/song.flac",LIB_MUSIC));assert(library_media_supported("film.mkv",LIB_VIDEO));
    assert(library_media_supported("film.TS",LIB_VIDEO)&&library_media_supported("film.m2ts",LIB_VIDEO)&&library_media_supported("film.mpeg",LIB_VIDEO));
    assert(!library_media_supported("film.webm",LIB_VIDEO)&&!library_media_supported("film.mp4.exe",LIB_VIDEO));
    assert(!library_media_supported("image.jpg.xex",LIB_PICTURES));assert(!library_media_supported(".mp3",LIB_MUSIC));
    path(config,temp,"settings.txt");assert(library_write_config(config,&s));LibrarySnapshot loaded={0};assert(library_read_config(config,&loaded)&&loaded.root_count==4&&loaded.roots[0].max_depth==LIBRARY_DEPTH_ALL&&loaded.roots[0].category==LIB_MUSIC&&loaded.roots[1].category==LIB_PICTURES&&loaded.roots[2].category==LIB_VIDEO);
    Fake fake={.release=true};LibraryHooks hooks={.context=&fake,.prepare=prepare};LibraryService *service=library_open(config,NULL,&hooks);assert(service);
    const LibrarySnapshot *front=idle(service,0);assert(front->count==13&&atomic_load(&fake.calls)==0); /* Never autoplay. */
    LibraryCommand command={.type=LIB_CMD_LAUNCH,.entry=*library_entry(front,LIB_VIDEO,0)};unsigned revision=front->revision;
    assert(library_request(service,&command));front=idle(service,revision);LibraryCategory category=LIB_ALL;LibraryKind kind=LIB_XEX;
    char launch[LIBRARY_PATH];assert(library_take_launch_ex(service,launch,&category,&kind)&&category==LIB_VIDEO&&kind==LIB_MEDIA&&!strcmp(launch,command.entry.path));
    assert(!library_take_launch_ex(service,launch,&category,&kind));
    assert(!unlink(command.entry.path));revision=front->revision;assert(library_request(service,&command));front=idle(service,revision);
    assert(front->status_error&&!library_take_launch(service,launch)&&atomic_load(&fake.calls)==1);
    library_close(service);
    /* Media menu entry points, filtered root indices, category selection and
       depth/removal requests must not address an interleaved Games root. */
    s.revision=1;Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;dashboard_set_library(&d,&s);
    for(int i=0;i<3;i++) {
        d.depth=0;d.library_page=LIB_UI_NONE;d.selected[BLADE_MEDIA]=i;press(&d,SELECT);
        if(i==2){assert(d.library_page==LIB_UI_VIDEO_SOURCES);press(&d,DOWN);press(&d,SELECT);}
        assert(d.library_page==LIB_UI_LIST&&d.library_filter==(LibraryCategory)(LIB_MUSIC+i));
        if(i==2){press(&d,BACK);assert(d.library_page==LIB_UI_VIDEO_SOURCES);}
        press(&d,BACK);assert(!d.library_page&&!d.depth);
    }
    d.selected[BLADE_MEDIA]=3;press(&d,SELECT);assert(d.library_page==LIB_UI_ROOTS&&dashboard_library_root_count(&d)==3&&dashboard_library_count(&d)==5);
    assert(dashboard_library_root_index(&d,2)==2&&dashboard_library_root_index(&d,3)==-1);
    d.library_row=2;press(&d,EJECT);assert(d.library_command.type==LIB_CMD_REMOVE&&d.library_command.root==2);
    s.revision++;snprintf(s.browse_path,LIBRARY_PATH,"%s",temp);dashboard_set_library(&d,&s);dashboard_library_open(&d,LIB_UI_BROWSE,LIB_ALL);press(&d,PROFILE);
    assert(d.library_page==LIB_UI_DEPTH&&d.library_filter==LIB_MEDIA_FOLDER);
    press(&d,BACK);assert(d.library_page==LIB_UI_BROWSE);press(&d,PROFILE);
    assert(d.library_page==LIB_UI_DEPTH&&d.library_filter==LIB_MEDIA_FOLDER);
    d.library_row=0;press(&d,SELECT);assert(d.library_command.type==LIB_CMD_ADD&&d.library_command.category==LIB_MEDIA_FOLDER&&d.library_command.max_depth==LIBRARY_DEPTH_ALL);
    s.revision++;dashboard_set_library(&d,&s);d.blade=BLADE_GAMES;dashboard_library_open(&d,LIB_UI_ROOTS,LIB_ALL);
    assert(dashboard_library_root_count(&d)==1&&dashboard_library_root_index(&d,0)==3);press(&d,EJECT);assert(d.library_command.root==3);
    library_snapshot_free(&s);library_snapshot_free(&loaded);remove_tree(temp);
    puts("PASS: three filtered media libraries, persisted per-category paths/depth, no autoplay, typed/manual/cancel-safe launch requests and Media-only path manager");
}
static void shared_media_folder(void){
    char temp[]="/tmp/blades-shared-media-XXXXXX";assert(mkdtemp(temp));
    char file[LIBRARY_PATH],nested[LIBRARY_PATH],config[LIBRARY_PATH];
    const char *names[]={"Song.MP3","Photo.JpG","Movie.MKV","bad.mp3.exe",".mp3","unknown.flac","notes.txt"};
    for(size_t i=0;i<sizeof names/sizeof names[0];i++){path(file,temp,names[i]);write_bytes(file,"fixture",7);}
    path(file,temp,"homebrew.xex");xex(file,0);path(file,temp,"package");package(file,0x7000,"Game must not appear");
    mkdir_child(nested,temp,"Nested.mp4");path(file,nested,"Clip.TS");write_bytes(file,"fixture",7);
    path(file,nested,"Track.m4a");write_bytes(file,"fixture",7);path(file,nested,"Image.PNG");write_bytes(file,"fixture",7);
    LibrarySnapshot s={.root_count=2};
    for(unsigned i=0;i<2;i++){snprintf(s.roots[i].path,LIBRARY_PATH,"%s",temp);s.roots[i].category=LIB_MEDIA_FOLDER;s.roots[i].max_depth=i?0:LIBRARY_DEPTH_ALL;}
    assert(library_scan(&s,NULL,NULL,NULL)&&s.count==6);assert(library_count(&s,LIB_MUSIC)==2&&library_count(&s,LIB_PICTURES)==2&&library_count(&s,LIB_VIDEO)==2&&!library_count(&s,LIB_ALL));
    for(size_t i=0;i<s.count;i++)assert(s.entries[i].kind==LIB_MEDIA&&s.entries[i].category!=LIB_MEDIA_FOLDER&&library_media_supported(s.entries[i].path,s.entries[i].category));
    s.root_count=1;s.roots[0].max_depth=0;assert(library_scan(&s,NULL,NULL,NULL)&&s.count==3);
    path(config,temp,"library-paths.txt");assert(library_write_config(config,&s));LibrarySnapshot loaded={0};assert(library_read_config(config,&loaded)&&loaded.root_count==1&&loaded.roots[0].category==LIB_MEDIA_FOLDER&&loaded.roots[0].max_depth==0);
    Fake fake={.release=true};LibraryHooks hooks={.context=&fake,.prepare=prepare};LibraryService *service=library_open(config,NULL,&hooks);assert(service);
    const LibrarySnapshot *front=idle(service,0);assert(front->count==3&&!atomic_load(&fake.calls));
    LibraryCommand command={.type=LIB_CMD_DEPTH,.root=0,.max_depth=LIBRARY_DEPTH_ALL};unsigned revision=front->revision;assert(library_request(service,&command));front=idle(service,revision);assert(front->count==6&&front->roots[0].category==LIB_MEDIA_FOLDER);
    command=(LibraryCommand){.type=LIB_CMD_ADD,.category=LIB_MEDIA_FOLDER,.max_depth=0};snprintf(command.path,LIBRARY_PATH,"%s",temp);revision=front->revision;assert(library_request(service,&command));front=idle(service,revision);assert(front->root_count==1&&front->status_error);
    library_close(service);library_snapshot_free(&s);library_snapshot_free(&loaded);remove_tree(temp);
    puts("PASS: one shared media folder filters all three apps by extension, case-insensitive nested scans/depth/deduplication, ignores executables/unsupported files, persists and rescans without autoplay");
}
int main(void){filesystem();navigation();depth_empty_and_homebrew();files_and_scan_controls();media_libraries();shared_media_folder();puts("PASS: recursive library, packages, categories, settings, async launch, cancellation and navigation");}
