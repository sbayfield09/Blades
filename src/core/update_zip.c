/* Bounded, strict ZIP reader for our USB payload. Raw DEFLATE uses pinned stb. */
#include "update_install.h"
#include "stb_image.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static unsigned le16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t le32(const unsigned char *p){return le16(p)|(uint32_t)le16(p+2)<<16;}
static uint32_t crc32_bytes(const unsigned char *data,size_t bytes){uint32_t table[256];for(unsigned i=0;i<256;i++){uint32_t c=i;for(unsigned k=0;k<8;k++)c=(c>>1)^((0u-(c&1))&0xedb88320u);table[i]=c;}uint32_t c=~0u;for(size_t i=0;i<bytes;i++)c=table[(c^data[i])&255]^(c>>8);return ~c;}
static bool read_at(FILE *f,uint64_t offset,void *data,size_t bytes){return offset<=64u*1024u*1024u&&!fseek(f,(long)offset,SEEK_SET)&&fread(data,1,bytes,f)==bytes;}
bool update_install_stage(const char *root,const char *zip,InstallCancelled cancel,InstallProgress progress,void *context,char *error,size_t bytes){
    snprintf(error,bytes,"The update ZIP is invalid or unsupported. Existing files were retained.");
    if(!update_install_reset(root)){snprintf(error,bytes,"Could not prepare update storage. An interrupted update may need recovery.");return false;}
    UpdateInstallPlan *plan=calloc(1,sizeof *plan);FILE *f=fopen(zip,"rb");unsigned char *tail=NULL,*compressed=NULL,*output=NULL;bool ok=false;
    if(!plan||!f)goto done;
    if(fseek(f,0,SEEK_END))goto done;
    long end=ftell(f);if(end<22||end>64*1024*1024)goto done;
    size_t tail_bytes=(size_t)end<65557?(size_t)end:65557;tail=malloc(tail_bytes);if(!tail||!read_at(f,(uint64_t)end-tail_bytes,tail,tail_bytes))goto done;
    size_t eocd=tail_bytes-22;
    for(;;){if(le32(tail+eocd)==0x06054b50u&&eocd+22+le16(tail+eocd+20)==tail_bytes)break;if(!eocd)goto done;eocd--;}
    if(eocd+22>tail_bytes||le32(tail+eocd)!=0x06054b50u)goto done;
    const unsigned char *e=tail+eocd;unsigned count=le16(e+10);uint64_t cd=le32(e+16),cd_end=cd+le32(e+12);
    if(le16(e+4)||le16(e+6)||le16(e+8)!=count||count<3||count>UPDATE_INSTALL_FILES||cd_end!=(uint64_t)end-tail_bytes+eocd)goto done;
    uint64_t cursor=cd,last_end=0,total=0;unsigned required=0;
    for(unsigned i=0;i<count;i++){
        if(cancel&&cancel(context)){snprintf(error,bytes,"Update cancelled. Existing files were retained.");goto done;}
        unsigned char central[46],local[30];char name[UPDATE_INSTALL_NAME+8],local_name[sizeof name];
        if(cursor+46>cd_end||!read_at(f,cursor,central,sizeof central)||le32(central)!=0x02014b50u)goto done;
        unsigned flags=le16(central+8),method=le16(central+10),length=le16(central+28);uint32_t packed=le32(central+20),unpacked=le32(central+24);uint64_t offset=le32(central+42);
        unsigned mode=le32(central+38)>>16;
        if(flags&~0x800u||(method!=0&&method!=8)||!length||length>=sizeof name||le16(central+34)||packed>32u*1024u*1024u||unpacked>32u*1024u*1024u||(mode&0170000u)==0120000u||offset<last_end||offset+30>cd)goto done;
        cursor+=46;if(cursor+length+le16(central+30)+le16(central+32)>cd_end||!read_at(f,cursor,name,length))goto done;name[length]=0;
        if(strlen(name)!=length||strncmp(name,"Blades/",7)||!update_install_allowed(name+7))goto done;
        UpdateInstallFile *entry=plan->files+plan->count;strcpy(entry->path,name+7);entry->size=unpacked;
        for(unsigned j=0;j<plan->count;j++){
            /* FATX is case insensitive, even when the test host isn't. */
            const char *a=entry->path,*b=plan->files[j].path;while(*a&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}if(!*a&&!*b)goto done;
        }
        if(!read_at(f,offset,local,sizeof local)||le32(local)!=0x04034b50u||le16(local+6)!=flags||le16(local+8)!=method||le32(local+14)!=le32(central+16)||le32(local+18)!=packed||le32(local+22)!=unpacked||le16(local+26)!=length)goto done;
        uint64_t data=offset+30+length+le16(local+28);last_end=data+packed;if(last_end>cd||!read_at(f,offset+30,local_name,length)||memcmp(name,local_name,length))goto done;
        total+=unpacked;if(total>128u*1024u*1024u)goto done;
        compressed=malloc(packed?packed:1);output=malloc(unpacked?unpacked:1);if(!compressed||!output||!read_at(f,data,compressed,packed))goto done;
        if(method==0){if(packed!=unpacked)goto done;memcpy(output,compressed,unpacked);}
        else if(stbi_zlib_decode_noheader_buffer((char *)output,(int)unpacked,(const char *)compressed,(int)packed)!=(int)unpacked)goto done;
        if(crc32_bytes(output,unpacked)!=le32(central+16))goto done;
        if(!strcmp(entry->path,"default.xex")||!strcmp(entry->path,"update-installer.xex")){
            if(unpacked<4096||memcmp(output,"XEX2",4))goto done;
            required|=!strcmp(entry->path,"default.xex")?1:2;
        }
        if(!strcmp(entry->path,"Assets/blades.xzp"))required|=4;
        char target[768];if(!update_install_path(target,sizeof target,root,UPDATE_INSTALL_WORK "/stage",entry->path)||!update_install_parents(target))goto done;
        FILE *out=fopen(target,"wb");if(!out)goto done;bool written=fwrite(output,1,unpacked,out)==unpacked;if(fflush(out)||fsync(fileno(out)))written=false;if(fclose(out))written=false;
        free(compressed);free(output);compressed=output=NULL;
        uint64_t size;if(!written||!update_install_hash(target,&size,entry->hash)||size!=unpacked)goto done;
        plan->count++;cursor+=length+le16(central+30)+le16(central+32);if(progress)progress(context,i+1,count);
    }
    if(required!=7||cursor!=cd_end||(cancel&&cancel(context)))goto done;
    /* Run the currently installed, trusted helper, never an uninstalled candidate. */
    char helper[768],source[768];
    if(!update_install_path(source,sizeof source,root,"","update-installer.xex")||!update_install_path(helper,sizeof helper,root,"",UPDATE_INSTALL_HELPER)||!update_install_copy(source,helper)){
        snprintf(error,bytes,"The installer helper is missing. Reinstall the complete Blades USB package.");goto done;
    }
    ok=update_install_save_plan(root,plan,"prepared");
done:
    if(f)fclose(f);
    free(tail);free(compressed);free(output);free(plan);
    if(!ok)(void)update_install_reset(root);else if(bytes)error[0]=0;
    return ok;
}
