/* Execute the pinned SDK's real path resolver and launch preflight on the host.
   Only the NT filesystem/object calls and terminal handoff are simulated. */
#include "disc.h"
#include "media.h"
#include "xsdk/kernel.h"
#include "xsdk/xam.h"
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static jmp_buf handoff;
static unsigned queried_flash_alias,queried_device,linked,data_sets;
static bool reject_device,reject_link,reject_data;
static char image[260];static uint8_t launch_data[1020];
const char *const ExLoadedImageName="\\Device\\Mass0\\Blades\\default.xex";
void xsdk_log_write(const char *format,...) {(void)format;}
int xsdk_file_directory_path(int fd,char path[260]) {(void)fd;(void)path;return EBADF;}
int xsdk_file_errno(NTSTATUS status) {return status==STATUS_OBJECT_NAME_NOT_FOUND?ENOENT:EIO;}
void xsdk_file_stat(const FILE_NETWORK_OPEN_INFORMATION *info,const char *path,struct stat *st) {
    (void)info;(void)path;memset(st,0,sizeof *st);st->st_mode=S_IFREG|0444;
}
NTSTATUS NtQueryFullAttributesFile(const OBJECT_ATTRIBUTES *attr,FILE_NETWORK_OPEN_INFORMATION *info) {
    const char *path=attr->name->pointer;
    if(!strcmp(path,"flash:\\dash.xex")){queried_flash_alias++;return STATUS_OBJECT_NAME_NOT_FOUND;}
    assert(!strcmp(path,"\\Device\\Flash\\dash.xex"));assert(!attr->root_directory);queried_device++;
    memset(info,0,sizeof *info);return reject_device?STATUS_OBJECT_NAME_NOT_FOUND:STATUS_SUCCESS;
}
NTSTATUS ObDeleteSymbolicLink(const ANSI_STRING *name) {assert(!strcmp(name->pointer,"\\??\\launch:"));return STATUS_SUCCESS;}
NTSTATUS ObCreateSymbolicLink(const ANSI_STRING *name,const ANSI_STRING *target) {
    assert(!strcmp(name->pointer,"\\??\\launch:"));assert(!strcmp(target->pointer,"\\Device\\Flash"));
    if(reject_link)return STATUS_OBJECT_NAME_NOT_FOUND;
    linked++;return STATUS_SUCCESS;
}
uint32_t XamLoaderSetLaunchData(const void *data,uint32_t size) {
    assert(size==sizeof launch_data);data_sets++;memcpy(launch_data,data,size);return reject_data?5:0;
}
_Noreturn void xsdk_exit_launching(const char *path) {snprintf(image,sizeof image,"%s",path);longjmp(handoff,1);}
static void reset(void) {linked=data_sets=0;image[0]=0;memset(launch_data,0,sizeof launch_data);}
int main(void) {
    uint8_t data[1020];assert(disc_dashboard_data(DISC_MEDIA_DVD_VIDEO,0,data));
    /* v0.22: Flash: has no DOS-device mapping, so launch fails before setting
       launch data or performing the SDK's cleanup/handoff. */
    reset();assert(xsdk_launch("flash:/dash.xex",data,sizeof data)==-1&&errno==ENOENT);
    assert(queried_flash_alias==1&&!linked&&!data_sets&&!image[0]);
    const DiscMedia kinds[]={DISC_MEDIA_DVD_VIDEO,DISC_MEDIA_AUDIO_CD,DISC_MEDIA_DATA};
    for(unsigned i=0;i<3;i++) {
        reset();assert(disc_dashboard_data(kinds[i],0,data));
        if(!setjmp(handoff)){xsdk_launch(disc_launch_path(kinds[i]),data,sizeof data);assert(!"SDK handoff did not occur");}
        assert(linked==1&&data_sets==1&&!strcmp(image,"launch:\\dash.xex"));
        assert(!memcmp(data,launch_data,sizeof data));assert(launch_data[7]==(i==0?7:i==1?6:0));
        assert(launch_data[11]==(i==0?255:0));
    }
    assert(queried_device==3&&!strcmp(disc_launch_path(DISC_MEDIA_GAME),"/Device/CdRom0/default.xex"));
    reset();reject_device=true;assert(xsdk_launch(disc_launch_path(DISC_MEDIA_DVD_VIDEO),data,sizeof data)==-1&&errno==ENOENT);assert(!linked&&!data_sets&&!image[0]);reject_device=false;
    reset();reject_link=true;assert(xsdk_launch(disc_launch_path(DISC_MEDIA_DVD_VIDEO),data,sizeof data)==-1);assert(!data_sets&&!image[0]);reject_link=false;
    reset();reject_data=true;assert(xsdk_launch(disc_launch_path(DISC_MEDIA_DVD_VIDEO),data,sizeof data)==-1&&errno==EIO);assert(!image[0]);
    reject_data=false;
    const LibraryCategory categories[]={LIB_MUSIC,LIB_PICTURES,LIB_VIDEO};const unsigned commands[]={6,44,39};
    for(unsigned i=0;i<3;i++) {
        reset();assert(media_dashboard_data(categories[i],2,data));assert(data[7]==commands[i]&&data[11]==2);
        for(unsigned j=12;j<sizeof data;j++)assert(data[j]==0); /* No unsupported filename ABI. */
        if(!setjmp(handoff)){xsdk_launch("/Device/Flash/dash.xex",data,sizeof data);assert(!"SDK handoff did not occur");}
        assert(linked==1&&data_sets==1&&!memcmp(data,launch_data,sizeof data));
    }
    assert(media_dashboard_data(LIB_VIDEO,UINT32_MAX,data)&&data[11]==255);
    assert(!media_dashboard_data(LIB_GAMES,0,data));
    puts("PASS: real SDK native disc/media-library opening prepares XAM alias and exact data; failures guarded; selected-file playback is not asserted");
}
