#include "library.h"
#include "disc_metadata.h"
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

struct LibraryService {
    pthread_t thread;pthread_mutex_t lock;pthread_cond_t wake;
    unsigned stop,scan_abort,launch_epoch,taken_serial,revision,progress_serial,progress_seen;bool busy,scanning;
    LibraryProgress progress;double next_progress;LibraryCommand command;LibraryHooks hooks;
    LibrarySnapshot working,*pending,*front;
    char config[LIBRARY_PATH],self[LIBRARY_PATH];
};
static void copy(char *out,size_t size,const char *in) {
    if(!size)return;
    if(!in)in="";
    size_t n=0;while(n+1<size&&in[n]){out[n]=in[n];n++;}out[n]=0;
}
const char *library_category_name(LibraryCategory c) {
    static const char *const names[]={"Games","Apps","Homebrew","Xbox Live Arcade","Demos","Game Library","Music","Pictures","Video","Media"};
    return names[c>=LIB_GAMES&&c<=LIB_MEDIA_FOLDER?c:LIB_ALL];
}
bool library_media_category(LibraryCategory c) {return c>=LIB_MUSIC&&c<=LIB_MEDIA_FOLDER;}
static bool root_category(LibraryCategory c) {return (c>=LIB_GAMES&&c<=LIB_HOMEBREW)||library_media_category(c);}
bool library_media_supported(const char *path,LibraryCategory c) {
    if(c==LIB_MEDIA_FOLDER)return library_media_supported(path,LIB_MUSIC)||library_media_supported(path,LIB_PICTURES)||library_media_supported(path,LIB_VIDEO);
    const char *name=path;for(const char *p=path;*p;p++)if(*p=='/'||*p=='\\')name=p+1;
    const char *ext=strrchr(name,'.');if(!ext||ext==name)return false;
    static const char *const music[]={".mp3",".wma",".m4a",NULL};
    static const char *const pictures[]={".jpg",".jpeg",".png",".bmp",".gif",".tif",".tiff",NULL};
    static const char *const video[]={".mkv",".wmv",".asf",".avi",".mp4",".m4v",".mov",".mp4v",".ts",".m2ts",".mpg",".mpeg",".flv",NULL};
    const char *const *types=c==LIB_MUSIC?music:c==LIB_PICTURES?pictures:c==LIB_VIDEO?video:NULL;
    if(types)for(;*types;types++)if(!strcasecmp(ext,*types))return true;
    return false;
}
static bool matches(const LibraryEntry *e,LibraryCategory c) {
    return c==LIB_ALL?(e->category==LIB_GAMES||e->category==LIB_ARCADE||e->category==LIB_DEMOS):e->category==c;
}
size_t library_count(const LibrarySnapshot *s,LibraryCategory c) {
    size_t n=0;if(s)for(size_t i=0;i<s->count;i++)if(matches(s->entries+i,c))n++;return n;
}
const LibraryEntry *library_entry(const LibrarySnapshot *s,LibraryCategory c,size_t row) {
    if(s)for(size_t i=0;i<s->count;i++)if(matches(s->entries+i,c)&&row--==0)return s->entries+i;
    return NULL;
}
bool library_join(char out[LIBRARY_PATH],const char *a,const char *b) {
    size_t n=strlen(a),m=strlen(b);bool slash=n&&a[n-1]!='/';
    if(n+slash+m>=LIBRARY_PATH)return false;
    memcpy(out,a,n);if(slash)out[n++]='/';memcpy(out+n,b,m+1);return true;
}
void library_parent(char p[LIBRARY_PATH]) {
    size_t root=0,n=strlen(p);char *colon=strchr(p,':');
    if(colon)root=(size_t)(colon-p)+2;
    else if(!strncmp(p,"/Device/",8)) {
        char *dev=p+8,*end=strchr(dev,'/');
        if(end&&(!strncmp(dev,"Harddisk",8)||strstr(dev,"PartitionFile")))end=strchr(end+1,'/');
        root=end?(size_t)(end-p)+1:n+1;
    } else if(p[0]=='/')root=1;
    if(n<=root){p[0]=0;return;}
    while(n>root&&p[n-1]=='/')p[--n]=0;
    char *slash=strrchr(p,'/');
    if(!slash){p[0]=0;return;}
    if((size_t)(slash-p)<root)p[root]=0;
    else *slash=0;
}
static bool clean_path(const char *p) {
    return p&&p[0]&&strlen(p)<LIBRARY_PATH&&!strchr(p,'\n')&&!strchr(p,'\r')&&!strchr(p,'\t');
}
void library_snapshot_free(LibrarySnapshot *s) {if(s){free(s->entries);free(s->folders);s->entries=NULL;s->folders=NULL;s->count=s->folder_count=0;}}
static uint32_t be32(const unsigned char *p) {return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
bool library_plain_xex(const char *path) {
    const char *name=path;
    for(const char *p=path;*p;p++)if(*p=='/'||*p=='\\')name=p+1;
    const char *dot=strchr(name,'.');return dot&&dot!=name&&!strcasecmp(dot,".xex");
}
bool library_xex_header(FILE *f,uint32_t *id) {
    unsigned char first[24];*id=0;
    if(fseek(f,0,SEEK_SET)||fread(first,1,sizeof first,f)!=sizeof first||memcmp(first,"XEX2",4))return false;
    size_t size=be32(first+8),count=be32(first+20),sec=be32(first+16);
    if(size<24||size>1024*1024||count>(size-24)/8||sec>size||size-sec<0x180)return false;
    unsigned char *h=malloc(size);if(!h)return false;
    bool ok=!fseek(f,0,SEEK_SET)&&fread(h,1,size,f)==size;
    if(ok)for(size_t i=0;i<count;i++)if(be32(h+24+i*8)==0x40006) {
        size_t at=be32(h+28+i*8);if(at>size||size-at<24){ok=false;break;}*id=be32(h+at+12);
    }
    free(h);return ok;
}
static void clean_title(char *title) {
    /* The current font covers ASCII. Omit unsupported glyphs as well as
       literal question marks, so UTF-8 trademark bytes do not draw as ???. */
    size_t n=0;bool space=false;
    for(const unsigned char *p=(const unsigned char *)title;*p;p++) {
        if(*p=='?'||*p>=127)continue;
        if(*p<=32){space=n!=0;continue;}
        if(space)title[n++]=' ';
        title[n++]=(char)*p;space=false;
    }
    title[n]=0;
}
static bool launch_type(uint32_t type) {return type==0x7000||type==0xd0000||type==0x80000||type==0xe0000;}
static bool display_name(FILE *f,char *out) {
    unsigned char p[256];
    if(fseek(f,0x411,SEEK_SET)||fread(p,1,sizeof p,f)!=sizeof p)return false;
    /* Package display name, unlike TitleName, identifies individual apps and
       arcade titles. Ignore malformed UTF-16 and fall back to TitleName. */
    size_t n=0;
    for(size_t i=0;i<128;i++) {
        uint32_t cp=(uint32_t)p[i*2]<<8|p[i*2+1];if(!cp)break;
        if(cp>=0xd800&&cp<=0xdbff) {
            if(++i>=128)return false;
            uint32_t low=(uint32_t)p[i*2]<<8|p[i*2+1];
            if(low<0xdc00||low>0xdfff)return false;
            cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);
        } else if(cp>=0xdc00&&cp<=0xdfff)return false;
        unsigned bytes=cp<128?1:cp<2048?2:cp<65536?3:4;if(n+bytes>=DISC_TITLE_BYTES)break;
        if(bytes==1)out[n++]=(char)(cp<32?' ':cp);
        else {out[n++]=(char)((bytes==2?0xc0:bytes==3?0xe0:0xf0)|(cp>>(6*(bytes-1))));for(unsigned j=bytes-1;j;j--)out[n++]=(char)(0x80|((cp>>(6*(j-1)))&63));}
    }
    out[n]=0;return n!=0;
}
static void fallback_name(LibraryEntry *e) {
    char parent[LIBRARY_PATH];copy(parent,sizeof parent,e->path);
    char *file=strrchr(parent,'/');file=file?file+1:parent;
    if(!strcasecmp(file,"default.xex")) {library_parent(parent);file=strrchr(parent,'/');file=file?file+1:parent;}
    disc_title_copy(e->name,sizeof e->name,file[0]?file:"Xbox 360 Game");
    char *dot=strrchr(e->name,'.');if(dot&&!strcasecmp(dot,".xex"))*dot=0;
}
bool library_read_entry(FILE *f,const char *path,LibraryCategory cat,const LibraryHooks *h,LibraryEntry *e) {
    unsigned char header[4];memset(e,0,sizeof *e);copy(e->path,sizeof e->path,path);e->category=cat;
    if(library_media_category(cat)) {
        if(!library_media_supported(path,cat))return false;
        if(cat==LIB_MEDIA_FOLDER)for(LibraryCategory type=LIB_MUSIC;type<=LIB_VIDEO;type++)if(library_media_supported(path,type)){e->category=type;break;}
        e->kind=LIB_MEDIA;fallback_name(e);char *dot=strrchr(e->name,'.');if(dot)*dot=0;
        clean_title(e->name);if(!e->name[0])copy(e->name,sizeof e->name,"Media file");return true;
    }
    size_t bytes=strlen(path);
    if(bytes>=4&&!strcasecmp(path+bytes-4,".xex")&&!library_plain_xex(path))return false;
    if(fseek(f,0,SEEK_SET)||fread(header,1,4,f)!=4)return false;
    if(!memcmp(header,"XEX2",4)) {
        e->kind=LIB_XEX;
        if(!library_plain_xex(path))return false;
        if(!library_xex_header(f,&e->title_id)||e->title_id==0x424c4453)return false;
        if(e->title_id) {
            uint32_t id=e->title_id;
            if(h&&h->xex_title)(void)h->xex_title(h->context,f,&id,e->name);
            else (void)disc_xex_title(f,NULL,&id,e->name);
        }
    } else if(!memcmp(header,"LIVE",4)||!memcmp(header,"PIRS",4)||!memcmp(header,"CON ",4)) {
        unsigned char info[0x20];e->kind=LIB_PACKAGE;
        if(fseek(f,0x344,SEEK_SET)||fread(info,1,sizeof info,f)!=sizeof info)return false;
        e->content_type=be32(info);e->title_id=be32(info+0x1c);
        if(!launch_type(e->content_type))return false;
        if(cat==LIB_GAMES)e->category=e->content_type==0xd0000?LIB_ARCADE:e->content_type==0x80000?LIB_DEMOS:LIB_GAMES;
        if(!display_name(f,e->name))(void)disc_stfs_title(f,e->name);
    } else return false;
    clean_title(e->name);
    if(!e->name[0]){fallback_name(e);clean_title(e->name);}
    if(!e->name[0])copy(e->name,sizeof e->name,"Xbox 360 Game");
    return true;
}
static int entry_compare(const void *a,const void *b) {
    const LibraryEntry *x=a,*y=b;int n=strcasecmp(x->name,y->name);return n?n:strcasecmp(x->path,y->path);
}
static bool stopped(const unsigned *stop) {return stop&&__atomic_load_n(stop,__ATOMIC_ACQUIRE);}
static int path_stat(const char *path,struct stat *st) {
#ifdef BLADES_XBOX
    /* FATX/USB have no POSIX symlinks. */
    return stat(path,st);
#else
    return lstat(path,st);
#endif
}
typedef struct Work {struct Work *next;char path[LIBRARY_PATH];unsigned depth;} Work;
static bool push(Work **list,const char *p,unsigned depth) {
    Work *w=malloc(sizeof *w);if(!w)return false;copy(w->path,sizeof w->path,p);w->depth=depth;w->next=*list;*list=w;return true;
}
void library_depth_label(unsigned depth,char *out,size_t size) {
    if(depth==LIBRARY_DEPTH_ALL)copy(out,size,"Unlimited");
    else if(!depth)copy(out,size,"Selected folder only (0)");
    else snprintf(out,size,"%u subfolder level%s",depth,depth==1?"":"s");
}
static void scan_progress(LibrarySnapshot *s,const LibraryHooks *h,const char *path,LibraryScanKind kind,unsigned depth) {
    if(path)copy(s->progress.path,sizeof s->progress.path,path);
    s->progress.kind=kind;s->progress.depth=depth;s->progress.found=s->count;
    if(h&&h->progress)h->progress(h->progress_context,&s->progress);
}
bool library_scan(LibrarySnapshot *s,const LibraryHooks *h,const char *self,const unsigned *stop) {
    free(s->entries);s->entries=NULL;s->count=0;s->errors=0;size_t capacity=0;s->progress=(LibraryProgress){.scanning=true};
    for(size_t root=0;root<s->root_count&&!stopped(stop);root++) {
        Work *list=NULL;if(!push(&list,s->roots[root].path,0)){s->errors++;break;}
        while(list&&!stopped(stop)) {
            Work *w=list;list=w->next;s->progress.folders++;
            scan_progress(s,h,w->path,LIB_SCAN_FOLDER,w->depth);DIR *dir=opendir(w->path);
            if(!dir){s->errors++;free(w);continue;}
            struct dirent *de;
            while(!stopped(stop)) {
                scan_progress(s,h,w->path,LIB_SCAN_FOLDER,w->depth);
                errno=0;de=readdir(dir);if(!de){if(errno)s->errors++;break;}
                if(!strcmp(de->d_name,".")||!strcmp(de->d_name,".."))continue;
                char path[LIBRARY_PATH];struct stat st;
                if(!library_join(path,w->path,de->d_name)){s->errors++;continue;}
                scan_progress(s,h,path,LIB_SCAN_ENTRY,w->depth);
                if(path_stat(path,&st)){s->errors++;continue;}
                if(S_ISDIR(st.st_mode)) {
                    size_t n=strlen(de->d_name);
                    if(!strcasecmp(de->d_name,"$SystemUpdate")||(n>=5&&!strcasecmp(de->d_name+n-5,".data")))continue;
                    if(w->depth<s->roots[root].max_depth&&!push(&list,path,w->depth+1))s->errors++;
                    continue;
                }
                if(!S_ISREG(st.st_mode))continue;
                s->progress.files++;scan_progress(s,h,path,LIB_SCAN_FILE,w->depth);
                if(self&&*self&&!strcasecmp(path,self))continue;
                bool media=library_media_category(s->roots[root].category);
                if(media&&!library_media_supported(path,s->roots[root].category))continue;
                size_t n=strlen(de->d_name);bool xex=n>=4&&!strcasecmp(de->d_name+n-4,".xex");
                if(xex&&!library_plain_xex(de->d_name))continue;
                /* Package names are normally extensionless. Do not read large
                   media files, artwork or SVOD data chunks as executables. */
                const char *ext=strrchr(de->d_name,'.');
                if(!media&&!xex&&ext&&strcasecmp(ext,".con")&&strcasecmp(ext,".live")&&strcasecmp(ext,".pirs")&&strcasecmp(ext,".stfs"))continue;
                bool duplicate=false;for(size_t i=0;i<s->count;i++)if((s->entries[i].kind==LIB_MEDIA)==media&&!strcasecmp(s->entries[i].path,path)){duplicate=true;break;}
                if(duplicate)continue;
                /* A directory containing default.xex represents one extracted
                   title. Additional .xex modules in it are not extra games. */
                if(xex&&s->roots[root].category==LIB_GAMES&&strcasecmp(de->d_name,"default.xex")) {
                    char primary[LIBRARY_PATH];uint32_t primary_id=0;bool has_primary=false;
                    if(library_join(primary,w->path,"default.xex")) {
                        scan_progress(s,h,primary,LIB_SCAN_FILE,w->depth);
                        FILE *primary_file=fopen(primary,"rb");
                        if(primary_file){has_primary=library_xex_header(primary_file,&primary_id)&&primary_id!=0x424c4453;fclose(primary_file);}
                    }
                    if(has_primary)continue;
                }
                scan_progress(s,h,path,LIB_SCAN_FILE,w->depth);
                FILE *f=fopen(path,"rb");if(!f){s->errors++;continue;}
                LibraryEntry e;bool valid=library_read_entry(f,path,s->roots[root].category,h,&e);fclose(f);if(!valid)continue;
                if(s->count==capacity) {
                    size_t next=capacity?capacity*2:64;
                    void *p=next<=SIZE_MAX/sizeof *s->entries?realloc(s->entries,next*sizeof *s->entries):NULL;
                    if(!p){s->errors++;closedir(dir);free(w);while(list){Work *q=list;list=q->next;free(q);}goto complete;}
                    s->entries=p;capacity=next;
                }
                s->entries[s->count++]=e;
            }
            closedir(dir);free(w);
        }
        while(list){Work *w=list;list=w->next;free(w);}
    }
complete:
    if(s->count>1)qsort(s->entries,s->count,sizeof *s->entries,entry_compare);
    snprintf(s->status,sizeof s->status,"%zu items found%s",s->count,s->errors?" - some folders could not be read":"");
    if(!s->root_count)copy(s->status,sizeof s->status,"Add a scan folder to find your games, apps or media.");
    else if(!s->count&&!s->errors)copy(s->status,sizeof s->status,"Scan complete - no games, apps, homebrew or media found.");
    if(stopped(stop))snprintf(s->status,sizeof s->status,"Scan stopped - %zu titles found",s->count);
    s->progress.scanning=false;s->progress.stopping=stopped(stop);scan_progress(s,h,NULL,s->progress.kind,s->progress.depth);
    s->status_error=s->errors!=0;
    return !s->errors&&!stopped(stop);
}
bool library_read_config(const char *path,LibrarySnapshot *s) {
    s->root_count=0;
    FILE *f=fopen(path,"rb");if(!f)return errno==ENOENT;
    char line[LIBRARY_PATH+32];bool ok=fgets(line,sizeof line,f)!=NULL;
    bool legacy=ok&&!strcmp(line,"BLADES-LIBRARY-1\n");
    bool media_config=ok&&!strcmp(line,"BLADES-LIBRARY-3\n");
    bool shared_config=ok&&!strcmp(line,"BLADES-LIBRARY-4\n");
    ok=ok&&(legacy||media_config||shared_config||!strcmp(line,"BLADES-LIBRARY-2\n"));
    while(ok&&fgets(line,sizeof line,f)) {
        size_t n=strlen(line);if(!n||line[n-1]!='\n'){ok=false;break;}line[n-1]=0;
        if(line[0]<'0'||line[0]>(shared_config?'9':media_config?'8':'2')||line[1]!='\t'||!root_category((LibraryCategory)(line[0]-'0'))||s->root_count==LIBRARY_ROOTS){ok=false;break;}
        char *path=line+2;unsigned depth=LIBRARY_DEPTH_DEFAULT;
        if(!legacy) {
            char *tab=strchr(path,'\t');if(!tab){ok=false;break;}*tab=0;
            if(!strcmp(path,"all"))depth=LIBRARY_DEPTH_ALL;
            else {
                char *end;errno=0;unsigned long v=strtoul(path,&end,10);
                if(errno||*path<'0'||*path>'9'||*end||v>LIBRARY_DEPTH_MAX){ok=false;break;}depth=(unsigned)v;
            }
            path=tab+1;
        }
        if(!clean_path(path)){ok=false;break;}
        LibraryRoot *r=s->roots+s->root_count++;r->category=(LibraryCategory)(line[0]-'0');r->max_depth=depth;copy(r->path,sizeof r->path,path);
    }
    if(ferror(f))ok=false;
    fclose(f);if(!ok)s->root_count=0;return ok;
}
bool library_write_config(const char *path,const LibrarySnapshot *s) {
    char temp[LIBRARY_PATH];if(strlen(path)+4>=sizeof temp)return false;snprintf(temp,sizeof temp,"%s.tmp",path);
    char parent[LIBRARY_PATH];copy(parent,sizeof parent,path);library_parent(parent);
    if(parent[0]&&mkdir(parent,0777)&&errno!=EEXIST)return false;
    bool media=false,shared=false;for(size_t i=0;i<s->root_count;i++){media|=library_media_category(s->roots[i].category);shared|=s->roots[i].category==LIB_MEDIA_FOLDER;}
    FILE *f=fopen(temp,"wb");if(!f)return false;bool ok=fputs(shared?"BLADES-LIBRARY-4\n":media?"BLADES-LIBRARY-3\n":"BLADES-LIBRARY-2\n",f)>=0;
    for(size_t i=0;i<s->root_count&&ok;i++) {
        const LibraryRoot *r=s->roots+i;char depth[16];
        if(r->max_depth==LIBRARY_DEPTH_ALL)copy(depth,sizeof depth,"all");else snprintf(depth,sizeof depth,"%u",r->max_depth);
        ok=clean_path(r->path)&&root_category(r->category)&&(r->max_depth<=LIBRARY_DEPTH_MAX||r->max_depth==LIBRARY_DEPTH_ALL)&&fprintf(f,"%d\t%s\t%s\n",r->category,depth,r->path)>0;
    }
    if(fflush(f)||ferror(f))ok=false;
    if(ok&&fsync(fileno(f)))ok=false;
    if(fclose(f))ok=false;
    if(ok&&rename(temp,path))ok=false;
    if(!ok)remove(temp);
    return ok;
}
static LibrarySnapshot *clone(const LibrarySnapshot *s) {
    LibrarySnapshot *p=malloc(sizeof *p);if(!p)return NULL;*p=*s;p->entries=NULL;p->folders=NULL;
    if(s->count){p->entries=malloc(s->count*sizeof *p->entries);if(!p->entries)goto fail;memcpy(p->entries,s->entries,s->count*sizeof *p->entries);}
    if(s->folder_count){p->folders=malloc(s->folder_count*sizeof *p->folders);if(!p->folders)goto fail;memcpy(p->folders,s->folders,s->folder_count*sizeof *p->folders);}
    return p;
fail:
    /* Keep the folder settings usable when a very large library cannot be
       duplicated for display, so the user can narrow their scan locations. */
    library_snapshot_free(p);p->status_error=true;copy(p->status,sizeof p->status,"Not enough memory to display this library. Remove a scan folder.");return p;
}
static void publish(LibraryService *l) {
    LibrarySnapshot *p=clone(&l->working);if(!p)return;
    pthread_mutex_lock(&l->lock);LibrarySnapshot *old=l->pending;l->pending=p;p->revision=++l->revision;l->busy=l->working.busy;if(!l->busy)l->scanning=false;pthread_mutex_unlock(&l->lock);
    if(old){library_snapshot_free(old);free(old);}
}
static void report_progress(void *context,const LibraryProgress *p) {
    LibraryService *l=context;pthread_mutex_lock(&l->lock);
    l->progress=*p;l->progress.stopping=stopped(&l->scan_abort);l->progress_serial++;
    pthread_mutex_unlock(&l->lock);
    if(l->hooks.progress)l->hooks.progress(l->hooks.progress_context,p);
}
static void run_scan(LibraryService *l) {
    LibraryHooks hooks=l->hooks;hooks.progress=report_progress;hooks.progress_context=l;
    library_scan(&l->working,&hooks,l->self,&l->scan_abort);
}
static int folder_compare(const void *a,const void *b) {
    const LibraryFolder *x=a,*y=b;
    if(x->is_file!=y->is_file)return x->is_file?1:-1;
    return strcasecmp(x->name,y->name);
}
static bool add_folder(LibrarySnapshot *s,const char *path,const char *name,bool is_file,bool executable) {
    void *p=realloc(s->folders,(s->folder_count+1)*sizeof *s->folders);if(!p)return false;s->folders=p;
    LibraryFolder *f=s->folders+s->folder_count++;copy(f->path,sizeof f->path,path);copy(f->name,sizeof f->name,name);f->is_file=is_file;f->executable=executable;return true;
}
static void browse(LibraryService *l,const char *path,bool files) {
    LibrarySnapshot *s=&l->working;free(s->folders);s->folders=NULL;s->folder_count=0;copy(s->browse_path,sizeof s->browse_path,path);
    bool ok=true;
    if(!path[0]) {
#ifdef BLADES_XBOX
        static const LibraryFolder devices[]={
            {"Hard Drive","hdd:/",false,false},
            {"USB0","usb0:/",false,false},{"USB1","usb1:/",false,false},{"USB2","usb2:/",false,false},
            {"USB Memory 0","/Device/Mass0PartitionFile/Storage/",false,false},
            {"USB Memory 1","/Device/Mass1PartitionFile/Storage/",false,false},
            {"USB Memory 2","/Device/Mass2PartitionFile/Storage/",false,false},
            {"Memory Unit 0","mu0:/",false,false},{"Memory Unit 1","mu1:/",false,false},
            {"Internal Memory","bladesmu:/",false,false}};
#else
        static const LibraryFolder devices[]={{"Desktop files","/",false,false}};
#endif
        for(size_t i=0;i<sizeof devices/sizeof devices[0]&&!stopped(&l->stop);i++){DIR *d=opendir(devices[i].path);if(d){closedir(d);if(!add_folder(s,devices[i].path,devices[i].name,false,false))ok=false;}}
    } else {
        DIR *d=opendir(path);if(!d)ok=false;
        if(d) {
            while(!stopped(&l->stop)) {
                errno=0;struct dirent *e=readdir(d);if(!e){if(errno)ok=false;break;}
                if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
                char child[LIBRARY_PATH];struct stat st;
                if(!library_join(child,path,e->d_name)||path_stat(child,&st)){ok=false;continue;}
                bool is_file=S_ISREG(st.st_mode),executable=is_file&&library_plain_xex(e->d_name);
                if((S_ISDIR(st.st_mode)||(files&&is_file))&&!add_folder(s,child,e->d_name,is_file,executable)){ok=false;break;}
            }
            closedir(d);
        }
    }
    if(s->folder_count>1)qsort(s->folders,s->folder_count,sizeof *s->folders,folder_compare);
    s->status_error=!ok;
    copy(s->status,sizeof s->status,ok?(files?"A: Open folder or launch XEX. B: Parent folder.":"Choose a folder, then press X to use it."):"Some folders could not be read. Check the storage device.");
}
static void *worker(void *context) {
    LibraryService *l=context;if(l->hooks.worker_start)l->hooks.worker_start(l->hooks.context);
    bool config_ok=library_read_config(l->config,&l->working);
    for(;;) {
        pthread_mutex_lock(&l->lock);
        while(!l->command.type&&!stopped(&l->stop))pthread_cond_wait(&l->wake,&l->lock);
        if(stopped(&l->stop)){pthread_mutex_unlock(&l->lock);break;}
        LibraryCommand cmd=l->command;l->command.type=LIB_CMD_NONE;
        l->scanning=cmd.type==LIB_CMD_SCAN||cmd.type==LIB_CMD_ADD||cmd.type==LIB_CMD_REMOVE||cmd.type==LIB_CMD_DEPTH;
        __atomic_store_n(&l->scan_abort,0,__ATOMIC_RELEASE);pthread_mutex_unlock(&l->lock);
        l->working.busy=true;l->working.launch_ready=false;l->working.status_error=false;copy(l->working.status,sizeof l->working.status,cmd.type==LIB_CMD_LAUNCH?"Preparing to launch...":cmd.type==LIB_CMD_FILES?"Loading files...":cmd.type==LIB_CMD_BROWSE?"Loading folders...":"Scanning folders...");publish(l);
        switch(cmd.type) {
        case LIB_CMD_BROWSE:case LIB_CMD_FILES:browse(l,cmd.path,cmd.type==LIB_CMD_FILES);break;
        case LIB_CMD_SCAN:run_scan(l);break;
        case LIB_CMD_ADD:case LIB_CMD_REMOVE:case LIB_CMD_DEPTH: {
            LibraryRoot previous[LIBRARY_ROOTS];size_t count=l->working.root_count;memcpy(previous,l->working.roots,sizeof previous);bool valid=false;
            if(cmd.type==LIB_CMD_ADD&&count<LIBRARY_ROOTS&&clean_path(cmd.path)&&root_category(cmd.category)&&(cmd.max_depth<=LIBRARY_DEPTH_MAX||cmd.max_depth==LIBRARY_DEPTH_ALL)) {
                valid=true;for(size_t i=0;i<count;i++)if((cmd.category==l->working.roots[i].category||(!library_media_category(cmd.category)&&!library_media_category(l->working.roots[i].category)))&&!strcasecmp(cmd.path,l->working.roots[i].path))valid=false;
                if(valid){LibraryRoot *r=l->working.roots+l->working.root_count++;copy(r->path,sizeof r->path,cmd.path);r->category=cmd.category;r->max_depth=cmd.max_depth;}
            } else if(cmd.type==LIB_CMD_REMOVE&&cmd.root<count) {
                memmove(l->working.roots+cmd.root,l->working.roots+cmd.root+1,(count-cmd.root-1)*sizeof *previous);l->working.root_count--;valid=true;
            }
            if(cmd.type==LIB_CMD_DEPTH&&cmd.root<count&&(cmd.max_depth<=LIBRARY_DEPTH_MAX||cmd.max_depth==LIBRARY_DEPTH_ALL)){l->working.roots[cmd.root].max_depth=cmd.max_depth;valid=true;}
            if(valid&&library_write_config(l->config,&l->working))run_scan(l);
            else {l->working.status_error=true;memcpy(l->working.roots,previous,sizeof previous);l->working.root_count=count;copy(l->working.status,sizeof l->working.status,valid?"Could not save scan folders. Check free space and write access.":"This folder is already added, or the folder list is full.");}
            break;
        }
        case LIB_CMD_LAUNCH: {
            l->working.status_error=true;
            FILE *f=(!cmd.direct_launch||library_plain_xex(cmd.entry.path))?fopen(cmd.entry.path,"rb"):NULL;LibraryEntry fresh;
            bool valid=f&&library_read_entry(f,cmd.entry.path,cmd.entry.category,NULL,&fresh);
            if(valid&&fresh.kind==LIB_MEDIA){struct stat st;valid=!cmd.direct_launch&&!path_stat(fresh.path,&st)&&S_ISREG(st.st_mode);}
            valid=valid&&(cmd.direct_launch?fresh.kind==LIB_XEX:
                fresh.kind==cmd.entry.kind&&fresh.title_id==cmd.entry.title_id&&fresh.content_type==cmd.entry.content_type);
            if(f)fclose(f);
            if(!valid)copy(l->working.status,sizeof l->working.status,"The title is unavailable or has changed. Rescan its folder.");
            else if(!l->hooks.prepare)copy(l->working.status,sizeof l->working.status,"Launching Xbox software requires an Xbox 360.");
            else if(l->hooks.prepare(l->hooks.context,cmd.direct_launch?&fresh:&cmd.entry,l->working.launch_path,l->working.status,sizeof l->working.status)) {
                l->working.status_error=false;
                if(cmd.launch_epoch==__atomic_load_n(&l->launch_epoch,__ATOMIC_ACQUIRE)) {
                    l->working.launch_ready=true;l->working.launch_serial++;l->working.launch_epoch=cmd.launch_epoch;l->working.launch_category=fresh.category;l->working.launch_kind=fresh.kind;copy(l->working.status,sizeof l->working.status,"Launching...");
                } else copy(l->working.status,sizeof l->working.status,"Launch cancelled.");
            }
            break;
        }
        default:break;
        }
        if(!config_ok){l->working.status_error=true;copy(l->working.status,sizeof l->working.status,"Scan folder settings could not be read. Add your folders again.");config_ok=true;}
        l->working.busy=false;publish(l);
    }
    return NULL;
}
LibraryService *library_open(const char *config,const char *self,const LibraryHooks *hooks) {
    if(!clean_path(config))return NULL;
    LibraryService *l=calloc(1,sizeof *l);if(!l)return NULL;
    copy(l->config,sizeof l->config,config);copy(l->self,sizeof l->self,self);if(hooks)l->hooks=*hooks;
    pthread_mutex_init(&l->lock,NULL);pthread_cond_init(&l->wake,NULL);copy(l->working.status,sizeof l->working.status,"Loading scan folders...");l->working.busy=true;l->working.revision=l->revision=1;
    l->front=clone(&l->working);l->command.type=LIB_CMD_SCAN;l->busy=true;
    pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,256*1024);
    int error=l->front?pthread_create(&l->thread,&attr,worker,l):-1;pthread_attr_destroy(&attr);
    if(error){if(l->front)free(l->front);pthread_mutex_destroy(&l->lock);pthread_cond_destroy(&l->wake);free(l);return NULL;}return l;
}
const LibrarySnapshot *library_poll(LibraryService *l) {
    if(!l)return NULL;
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);double now=t.tv_sec+t.tv_nsec/1e9;
    pthread_mutex_lock(&l->lock);LibrarySnapshot *p=l->pending;l->pending=NULL;
    LibrarySnapshot *front=p?p:l->front;
    if(l->progress_serial!=l->progress_seen&&(p||now>=l->next_progress)) {
        front->progress=l->progress;front->revision=++l->revision;
        l->progress_seen=l->progress_serial;l->next_progress=now+.1;
    }
    pthread_mutex_unlock(&l->lock);
    if(p){library_snapshot_free(l->front);free(l->front);l->front=p;}return l->front;
}
bool library_request(LibraryService *l,const LibraryCommand *c) {
    if(!l||!c||!c->type)return false;
    pthread_mutex_lock(&l->lock);
    if(c->type==LIB_CMD_STOP) {
        bool active=l->scanning&&!stopped(&l->stop);
        if(active){__atomic_store_n(&l->scan_abort,1,__ATOMIC_RELEASE);l->progress.stopping=true;l->progress_serial++;}
        pthread_mutex_unlock(&l->lock);return active;
    }
    bool ok=!l->busy&&!stopped(&l->stop);
    if(ok){l->command=*c;if(c->type==LIB_CMD_LAUNCH)l->command.launch_epoch=__atomic_add_fetch(&l->launch_epoch,1,__ATOMIC_ACQ_REL);l->busy=true;pthread_cond_signal(&l->wake);}pthread_mutex_unlock(&l->lock);return ok;
}
void library_cancel_launch(LibraryService *l) {if(l)__atomic_add_fetch(&l->launch_epoch,1,__ATOMIC_ACQ_REL);}
bool library_take_launch(LibraryService *l,char path[LIBRARY_PATH]) {
    return library_take_launch_ex(l,path,NULL,NULL);
}
bool library_take_launch_ex(LibraryService *l,char path[LIBRARY_PATH],LibraryCategory *category,LibraryKind *kind) {
    if(!l||!l->front||!l->front->launch_ready||l->front->launch_serial==l->taken_serial||
       l->front->launch_epoch!=__atomic_load_n(&l->launch_epoch,__ATOMIC_ACQUIRE))return false;
    l->taken_serial=l->front->launch_serial;copy(path,LIBRARY_PATH,l->front->launch_path);
    if(category)*category=l->front->launch_category;
    if(kind)*kind=l->front->launch_kind;
    return true;
}
void library_close(LibraryService *l) {
    if(!l)return;
    __atomic_store_n(&l->stop,1,__ATOMIC_RELEASE);__atomic_store_n(&l->scan_abort,1,__ATOMIC_RELEASE);pthread_mutex_lock(&l->lock);pthread_cond_signal(&l->wake);pthread_mutex_unlock(&l->lock);pthread_join(l->thread,NULL);
    library_snapshot_free(&l->working);library_snapshot_free(l->front);library_snapshot_free(l->pending);free(l->front);free(l->pending);pthread_mutex_destroy(&l->lock);pthread_cond_destroy(&l->wake);free(l);
}
