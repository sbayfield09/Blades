#include "profile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void package(const char *root,const char *id,bool profile){
    char path[1024];snprintf(path,sizeof path,"%s/Content",root);assert(!mkdir(path,0700)||!access(path,F_OK));
    snprintf(path,sizeof path,"%s/Content/%s",root,id);assert(!mkdir(path,0700)||!access(path,F_OK));
    snprintf(path,sizeof path,"%s/Content/%s/FFFE07D1",root,id);assert(!mkdir(path,0700)||!access(path,F_OK));
    snprintf(path,sizeof path,"%s/Content/%s/FFFE07D1/00010000",root,id);assert(!mkdir(path,0700)||!access(path,F_OK));
    snprintf(path,sizeof path,"%s/Content/%s/FFFE07D1/00010000/%s",root,id,id);
    unsigned char bytes[0x348]={0};memcpy(bytes,"CON ",4);bytes[0x345]=profile?1:2;
    FILE *f=fopen(path,"wb");assert(f&&fwrite(bytes,1,sizeof bytes,f)==sizeof bytes&&!fclose(f));
}
int main(void){
    char root[]="/tmp/blades-profile-count-XXXXXX";assert(mkdtemp(root));char hdd[1024],usb[1024];
    snprintf(hdd,sizeof hdd,"%s/HDD",root);snprintf(usb,sizeof usb,"%s/USB0",root);assert(!mkdir(hdd,0700)&&!mkdir(usb,0700));
    package(hdd,"E000000000000001",true);package(hdd,"E000000000000002",true);package(usb,"E000000000000001",true);
    package(usb,"E000000000000003",false);package(usb,"0000000000000000",true);
    const char *devices[]={hdd,usb,"/nonexistent-blades-device"};unsigned count=99;
    assert(profile_count_packages(devices,3,&count)&&count==2);
    assert(!profile_count_packages(devices+2,1,&count)&&!count);
    package(usb,"E000000000000004",true);assert(profile_count_packages(devices,2,&count)&&count==3);
    char command[1200];snprintf(command,sizeof command,"rm -rf '%s'",root);assert(!system(command));
    puts("PASS: real installed profile-package metadata count, cross-device deduplication, non-profile/common exclusion and unavailable-device handling");
}
