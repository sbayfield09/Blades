#include "profile.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool profile_id(const char *name,uint64_t *id){
    if(strlen(name)!=16)return false;
    *id=0;
    for(unsigned i=0;i<16;i++){unsigned c=(unsigned char)name[i],n=c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:16;if(n>15)return false;*id=(*id<<4)|n;}
    return *id!=0;
}
/* Fixed profile-package location and public STFS header only. No account/GPD
   payload is opened or decoded; copied profiles on two devices count once. */
bool profile_count_packages(const char *const *devices,size_t count,unsigned *total){
    uint64_t *ids=calloc(4096,sizeof *ids);if(!ids)return false;unsigned used=0;bool available=false,ok=true;
    for(size_t n=0;n<count;n++){
        DIR *device=opendir(devices[n]);if(!device)continue;closedir(device);available=true;
        char path[1024];int length=snprintf(path,sizeof path,"%s%sContent",devices[n],devices[n][strlen(devices[n])-1]=='/'?"":"/");
        if(length<0||(size_t)length>=sizeof path){ok=false;continue;}
        DIR *content=opendir(path);if(!content){if(errno!=ENOENT)ok=false;continue;}
        for(;;){errno=0;struct dirent *e=readdir(content);if(!e){if(errno)ok=false;break;}uint64_t id;
            if(!profile_id(e->d_name,&id))continue;
            char file[1024];length=snprintf(file,sizeof file,"%s/%s/FFFE07D1/00010000/%s",path,e->d_name,e->d_name);
            if(length<0||(size_t)length>=sizeof file){ok=false;continue;}
            FILE *f=fopen(file,"rb");if(!f){if(errno!=ENOENT&&errno!=ENOTDIR)ok=false;continue;}
            unsigned char magic[4],type[4];bool valid=fread(magic,1,4,f)==4&&(!memcmp(magic,"CON ",4)||!memcmp(magic,"LIVE",4)||!memcmp(magic,"PIRS",4))&&!fseek(f,0x344,SEEK_SET)&&fread(type,1,4,f)==4&&!memcmp(type,"\0\1\0\0",4);fclose(f);
            if(!valid)continue;
            bool duplicate=false;for(unsigned i=0;i<used;i++)if(ids[i]==id)duplicate=true;
            if(!duplicate){if(used==4096){ok=false;break;}ids[used++]=id;}
        }
        closedir(content);
    }
    free(ids);*total=available&&ok?used:0;return available&&ok;
}
