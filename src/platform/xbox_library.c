#include "xbox_library.h"
#include "xbox_disc.h"
#include "disc_metadata.h"
#include "xsdk/thread.h"
#include "xsdk/log.h"
#include <stdlib.h>
#include <string.h>
struct XboxLibrary {
    LibraryService *service;bool mounted,linked,preserve,mu_linked;
    uint32_t (*open_file)(uint32_t,const char *,const char *,uint32_t,uint32_t,uint32_t *,XAM_OVERLAPPED *);
};
static const char SOURCE_LINK[]="\\??\\bladescontent:";
static const char PACKAGE_ROOT[]="bladespkg";
static const char MU_LINK[]="\\??\\bladesmu:";
static bool native_path(char out[LIBRARY_PATH],const char *path) {
    static const struct {const char *drive,*target;} drives[]={
        {"hdd:","\\Device\\Harddisk0\\Partition1"},
        {"usb0:","\\Device\\Mass0"},{"usb1:","\\Device\\Mass1"},{"usb2:","\\Device\\Mass2"},
        {"mu0:","\\Device\\Mu0"},{"mu1:","\\Device\\Mu1"},{"bladesmu:","\\Device\\BuiltInMuSfc"}};
    bool found=!strncmp(path,"/Device/",8);
    if(found)snprintf(out,LIBRARY_PATH,"%s",path);
    for(size_t i=0;i<sizeof drives/sizeof drives[0]&&!found;i++) {
        size_t n=strlen(drives[i].drive);
        if(!strncmp(path,drives[i].drive,n)){if(snprintf(out,LIBRARY_PATH,"%s%s",drives[i].target,path+n)>=LIBRARY_PATH)return false;found=true;}
    }
    if(found)for(char *p=out;*p;p++)if(*p=='/')*p='\\';
    return found;
}
static ANSI_STRING ansi(const char *p) {size_t n=strlen(p);return (ANSI_STRING){.length=(uint16_t)n,.maximum_length=(uint16_t)(n+1),.pointer=p};}
static void unmount(XboxLibrary *l) {
    if(l->mounted){uint32_t r=XamContentClose("\\??\\bladespkg",NULL);xsdk_log_write("library: close package mount result=%08X",r);l->mounted=false;}
    if(l->linked){ANSI_STRING link=ansi(SOURCE_LINK);(void)ObDeleteSymbolicLink(&link);l->linked=false;}
}
static bool title(void *context,FILE *file,uint32_t *id,char *name) {(void)context;return xbox_disc_xex_title(file,id,name);}
static void start_worker(void *context) {(void)context;(void)xsdk_thread_move(4);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-1,NULL);}
static bool prepare(void *context,const LibraryEntry *e,char *launch,char *error,size_t bytes) {
    XboxLibrary *l=context;unmount(l);
    xsdk_log_write("library: manual launch title=%08X kind=%u type=%08X path=%s",e->title_id,e->kind,e->content_type,e->path);
    if(e->kind==LIB_MEDIA) {
        if(e->category!=LIB_VIDEO){snprintf(error,bytes,"Music and pictures playback is not implemented yet.");return false;}
        snprintf(launch,LIBRARY_PATH,"%s",e->path);return true;
    }
    if(e->kind==LIB_XEX){snprintf(launch,LIBRARY_PATH,"%s",e->path);return true;}
    if(!l->open_file){snprintf(error,bytes,"This kernel does not expose the content mounting service.");return false;}
    char folder[LIBRARY_PATH];
    if(!native_path(folder,e->path)){snprintf(error,bytes,"The package storage path could not be resolved.");return false;}
    char *last=strrchr(folder,'\\');
    if(!last){snprintf(error,bytes,"The package filename is unavailable.");return false;}
    char filename[LIBRARY_PATH];snprintf(filename,sizeof filename,"%s",last+1);*last=0;
    /* A file directly on a device root requires a trailing separator. */
    if(!strchr(folder+8,'\\'))strcat(folder,"\\");
    ANSI_STRING link=ansi(SOURCE_LINK),target=ansi(folder);
    NTSTATUS status=ObCreateSymbolicLink(&link,&target);
    if(!NT_SUCCESS(status)){xsdk_log_write("library: source link failed %08X",(unsigned)status);snprintf(error,bytes,"The package storage device is unavailable.");return false;}
    l->linked=true;
    char package[LIBRARY_PATH];
    if(snprintf(package,sizeof package,"\\??\\bladescontent:\\%s",filename)>=(int)sizeof package){unmount(l);snprintf(error,bytes,"The package filename is too long.");return false;}
    /* Same execute mount contract as FreeStyle's mountCon: device attribute,
       no digest writes, keep open across title termination, execute. Existing
       packages only; no create, conversion, license editing or signature flag. */
    uint32_t result=l->open_file(0xfe,PACKAGE_ROOT,package,0x04000043,0,NULL,NULL);
    xsdk_log_write("library: XamContentOpenFile type=%08X result=%08X",e->content_type,result);
    if(result){unmount(l);snprintf(error,bytes,"The downloaded title could not be mounted (code %08X).",result);return false;}
    l->mounted=true;
    FILE *f=fopen("bladespkg:/default.xex","rb");uint32_t id=0;
    bool valid=f&&disc_xex_title(f,NULL,&id,NULL)&&(!e->title_id||e->title_id==id);
    if(f)fclose(f);
    if(!valid){unmount(l);snprintf(error,bytes,"The package has no readable matching default.xex.");return false;}
    snprintf(launch,LIBRARY_PATH,"bladespkg:/default.xex");return true;
}
XboxLibrary *xbox_library_open(void) {
    XboxLibrary *l=calloc(1,sizeof *l);if(!l)return NULL;HANDLE module;void *p=NULL;
    if(XexGetModuleHandle("xam.xex",&module)==0&&NT_SUCCESS(XexGetProcedureAddress(module,0x269,&p)))l->open_file=p;
    xsdk_log_write("library: content mount API=%u; user scan paths; all subfolders",l->open_file!=NULL);
    ANSI_STRING mu=ansi(MU_LINK),mu_target=ansi("\\Device\\BuiltInMuSfc");l->mu_linked=NT_SUCCESS(ObCreateSymbolicLink(&mu,&mu_target));
    char self[LIBRARY_PATH];snprintf(self,sizeof self,"%s",ExLoadedImageName?ExLoadedImageName:"");
    for(char *q=self;*q;q++)if(*q=='\\')*q='/';
    LibraryHooks hooks={.context=l,.xex_title=title,.prepare=prepare,.worker_start=start_worker};
    l->service=library_open("game:/Config/library-paths.txt",self,&hooks);
    if(!l->service){if(l->mu_linked)(void)ObDeleteSymbolicLink(&mu);free(l);return NULL;}return l;
}
LibraryService *xbox_library_service(XboxLibrary *l) {return l?l->service:NULL;}
void xbox_library_handoff(XboxLibrary *l,bool preserve) {if(l){l->preserve=preserve;if(!preserve)unmount(l);}}
void xbox_library_close(XboxLibrary *l) {
    if(!l)return;
    library_close(l->service);
    if(!l->preserve){unmount(l);if(l->mu_linked){ANSI_STRING mu=ansi(MU_LINK);(void)ObDeleteSymbolicLink(&mu);}}
    free(l);
}
