#define _DEFAULT_SOURCE
#include "update_install.h"
#include "mbedtls/sha256.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef BLADES_XBOX
#define safe_stat stat /* FATX has no filesystem symlinks. */
#else
#define safe_stat lstat
#endif
#define FILE_LIMIT (32u*1024u*1024u)
static bool problem(char *error,size_t bytes,const char *text){snprintf(error,bytes,"%s",text);return false;}
bool update_install_allowed(const char *path){
    if(!path||!*path||strlen(path)>=UPDATE_INSTALL_NAME)return false;
    const char *part=path;unsigned components=0;
    for(const char *p=path;;p++){
        if(*p=='/'||!*p){size_t n=(size_t)(p-part);if(!n||n>42||part[0]=='.'||part[n-1]=='.')return false;components++;part=p+1;if(!*p)break;}
        else if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'||*p=='.'))return false;
    }
    if(components==2&&(!strncmp(path,"Assets/",7)||!strncmp(path,"licenses/",9)))return true;
    return !strcmp(path,"default.xex")||!strcmp(path,"update-installer.xex")||!strcmp(path,"README.txt")||!strcmp(path,"build-info.txt")||!strcmp(path,"SHA256SUMS")||!strcmp(path,"LICENSE")||!strcmp(path,"XSDK-LICENSE.txt")||!strcmp(path,"THIRD_PARTY_NOTICES.md")||!strcmp(path,"Config/README.txt");
}
bool update_install_path(char *out,size_t bytes,const char *root,const char *area,const char *relative){
    int n=snprintf(out,bytes,"%s/%s%s%s",root,area,*area&&*relative?"/":"",relative);return n>0&&(size_t)n<bytes;
}
static bool directory(const char *path){struct stat st;return (!mkdir(path,0755)||errno==EEXIST)&&!safe_stat(path,&st)&&S_ISDIR(st.st_mode);}
bool update_install_parents(const char *path){
    char copy[768];if(strlen(path)>=sizeof copy)return false;strcpy(copy,path);
    for(char *p=copy+1;*p;p++)if(*p=='/'){
        *p=0;bool drive=p>copy&&p[-1]==':';bool ok=drive||directory(copy);*p='/';if(!ok)return false;
    }return true;
}
bool update_install_hash(const char *path,uint64_t *bytes,char hash[65]){
    struct stat st;if(safe_stat(path,&st)||!S_ISREG(st.st_mode)||st.st_size<0||(uint64_t)st.st_size>FILE_LIMIT)return false;
    FILE *f=fopen(path,"rb");if(!f)return false;mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);bool ok=mbedtls_sha256_starts(&ctx,0)==0;unsigned char data[32768],digest[32];uint64_t total=0;
    while(ok){size_t n=fread(data,1,sizeof data,f);if(!n)break;total+=n;if(total>FILE_LIMIT||mbedtls_sha256_update(&ctx,data,n))ok=false;}
    ok=ok&&!ferror(f)&&total==(uint64_t)st.st_size;ok=mbedtls_sha256_finish(&ctx,digest)==0&&ok;mbedtls_sha256_free(&ctx);if(fclose(f))ok=false;
    if(ok){*bytes=total;for(unsigned i=0;i<32;i++)snprintf(hash+2*i,3,"%02x",digest[i]);}return ok;
}
bool update_install_copy(const char *source,const char *destination){
    struct stat st;if(safe_stat(source,&st)||!S_ISREG(st.st_mode)||!update_install_parents(destination))return false;
    if(!safe_stat(destination,&st)&&!S_ISREG(st.st_mode))return false;
    FILE *in=fopen(source,"rb"),*out=NULL;if(in)out=fopen(destination,"wb");if(!in||!out){if(in)fclose(in);return false;}
    unsigned char data[32768];bool ok=true;while(ok){size_t n=fread(data,1,sizeof data,in);if(!n)break;if(fwrite(data,1,n,out)!=n)ok=false;}
    ok=ok&&!ferror(in);if(fflush(out)||fsync(fileno(out)))ok=false;if(fclose(in))ok=false;if(fclose(out))ok=false;return ok;
}
static bool present(const char *root,const char *name){char path[768];struct stat st;if(!update_install_path(path,sizeof path,root,UPDATE_INSTALL_WORK,name))return true;return !safe_stat(path,&st)||errno!=ENOENT;}
bool update_install_pending(const char *root){return present(root,"pending")||present(root,"prepared");}
static bool clear_directory(const char *path,unsigned depth){
    if(depth>4)return false;
    struct stat st;if(safe_stat(path,&st))return errno==ENOENT;
    if(!S_ISDIR(st.st_mode))return !unlink(path);
    DIR *d=opendir(path);if(!d)return false;bool ok=true;struct dirent *e;
    while((e=readdir(d))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;char child[768];if(!update_install_path(child,sizeof child,path,"",e->d_name)||!clear_directory(child,depth+1)){ok=false;break;}}
    closedir(d);return ok&&!rmdir(path);
}
bool update_install_reset(const char *root){
    char path[768];if(update_install_pending(root)||!update_install_path(path,sizeof path,root,"",UPDATE_INSTALL_WORK))return false;
    return clear_directory(path,0)&&directory(path);
}
static bool hex_hash(const char *s){if(strlen(s)!=64)return false;for(unsigned i=0;i<64;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;return true;}
static bool checked_file(const char *root,const char *area,const UpdateInstallFile *f,bool old){
    char path[768],hash[65];uint64_t size=0;return update_install_path(path,sizeof path,root,area,f->path)&&update_install_hash(path,&size,hash)&&size==(old?f->old_size:f->size)&&!strcmp(hash,old?f->old_hash:f->hash);
}
bool update_install_save_plan(const char *root,const UpdateInstallPlan *p,const char *name){
    char path[768],temporary[768],relative[80];snprintf(relative,sizeof relative,"%s.tmp",name);
    if(!update_install_path(path,sizeof path,root,UPDATE_INSTALL_WORK,name)||!update_install_path(temporary,sizeof temporary,root,UPDATE_INSTALL_WORK,relative))return false;
    FILE *f=fopen(temporary,"wb");if(!f)return false;bool ok=fprintf(f,"BLADES-UPDATE-1\n%u\n",p->count)>0;
    for(unsigned i=0;ok&&i<p->count;i++){const UpdateInstallFile *e=p->files+i;ok=fprintf(f,"%llu %s %u %llu %s %s\n",(unsigned long long)e->size,e->hash,e->existed?1:0,(unsigned long long)e->old_size,e->existed?e->old_hash:"-",e->path)>0;}
    if(fflush(f)||fsync(fileno(f)))ok=false;
    if(fclose(f))ok=false;
    if(ok&&rename(temporary,path))ok=false;
    if(!ok)unlink(temporary);
    return ok;
}
static bool read_plan(const char *root,UpdateInstallPlan *p,const char *name){
    char path[768],line[512];if(!update_install_path(path,sizeof path,root,UPDATE_INSTALL_WORK,name))return false;
    FILE *f=fopen(path,"rb");if(!f)return false;bool ok=fgets(line,sizeof line,f)&&!strcmp(line,"BLADES-UPDATE-1\n")&&fgets(line,sizeof line,f)&&sscanf(line,"%u",&p->count)==1&&p->count>=3&&p->count<=UPDATE_INSTALL_FILES;
    unsigned required=0;uint64_t total=0;
    for(unsigned i=0;ok&&i<p->count;i++){
        UpdateInstallFile *e=p->files+i;unsigned exists;unsigned long long size,old;char extra;
        ok=fgets(line,sizeof line,f)&&sscanf(line,"%llu %64s %u %llu %64s %159s %c",&size,e->hash,&exists,&old,e->old_hash,e->path,&extra)==6;
        if(!ok)break;
        e->size=size;e->old_size=old;e->existed=exists!=0;
        ok=exists<=1&&size<=FILE_LIMIT&&old<=FILE_LIMIT&&hex_hash(e->hash)&&(!exists||hex_hash(e->old_hash))&&update_install_allowed(e->path);
        for(unsigned j=0;ok&&j<i;j++)if(!strcmp(e->path,p->files[j].path))ok=false;
        total+=size;if(total>128u*1024u*1024u)ok=false;
        if(!strcmp(e->path,"default.xex"))required|=1;
        if(!strcmp(e->path,"update-installer.xex"))required|=2;
        if(!strcmp(e->path,"Assets/blades.xzp"))required|=4;
    }
    if(fgetc(f)!=EOF||ferror(f))ok=false;
    fclose(f);return ok&&required==7;
}
static void remove_marker(const char *root,const char *name){char path[768];if(update_install_path(path,sizeof path,root,UPDATE_INSTALL_WORK,name))unlink(path);}
static bool rollback(const char *root,const UpdateInstallPlan *p){
    /* Backups are never consumed: a second recovery can repeat every restore. */
    for(unsigned i=0;i<p->count;i++)if(p->files[i].existed&&!checked_file(root,UPDATE_INSTALL_WORK "/backup",p->files+i,true))return false;
    for(unsigned i=0;i<p->count;i++){
        const UpdateInstallFile *e=p->files+i;char target[768],backup[768],temporary[768];
        if(!update_install_path(target,sizeof target,root,"",e->path))return false;
        if(e->existed){
            if(checked_file(root,"",e,true))continue;
            if(!update_install_path(backup,sizeof backup,root,UPDATE_INSTALL_WORK "/backup",e->path)||!update_install_path(temporary,sizeof temporary,root,UPDATE_INSTALL_WORK "/restore",e->path)||!update_install_copy(backup,temporary)||rename(temporary,target)||!checked_file(root,"",e,true))return false;
        }else{struct stat st;if(!safe_stat(target,&st)){if(!checked_file(root,"",e,false)||unlink(target))return false;}else if(errno!=ENOENT)return false;}
#ifdef BLADES_INSTALL_TEST
        extern bool update_install_test_point(unsigned);if(!update_install_test_point(UPDATE_INSTALL_FILES+i+1))return false;
#endif
    }
    remove_marker(root,"prepared");remove_marker(root,"pending");return !update_install_pending(root);
}
int update_install_apply(const char *root,char *error,size_t bytes){
    UpdateInstallPlan *p=calloc(1,sizeof *p);if(!p){problem(error,bytes,"Not enough memory for installation.");return -1;}
    bool recovering=present(root,"pending");
    if(!read_plan(root,p,recovering?"pending":"prepared")){if(!recovering)remove_marker(root,"prepared");free(p);problem(error,bytes,"The update journal is missing or invalid. Existing files were retained.");return -1;}
    if(recovering){bool ok=rollback(root,p);free(p);problem(error,bytes,ok?"Interrupted update recovered. Previous version restored.":"Recovery could not finish. Keep the update backup and retry the installer.");return ok?0:-1;}
    /* Validate every staged file and complete every backup before changing anything. */
    bool ok=true;
    for(unsigned i=0;ok&&i<p->count;i++){
        UpdateInstallFile *e=p->files+i;char target[768],backup[768];struct stat st;
        ok=checked_file(root,UPDATE_INSTALL_WORK "/stage",e,false)&&update_install_path(target,sizeof target,root,"",e->path)&&update_install_parents(target)&&update_install_path(backup,sizeof backup,root,UPDATE_INSTALL_WORK "/backup",e->path);
        if(!ok)break;
        if(!safe_stat(target,&st)){e->existed=true;ok=update_install_hash(target,&e->old_size,e->old_hash)&&update_install_copy(target,backup)&&checked_file(root,UPDATE_INSTALL_WORK "/backup",e,true);}
        else if(errno!=ENOENT)ok=false;
    }
    if(!ok||!update_install_save_plan(root,p,"pending")){remove_marker(root,"prepared");free(p);problem(error,bytes,"Could not validate or back up the update. Existing files were retained.");return -1;}
    remove_marker(root,"prepared");
    /* Atomic replacement keeps default.xex present; install it last. */
    for(unsigned pass=0;ok&&pass<2;pass++)for(unsigned i=0;ok&&i<p->count;i++){
        const UpdateInstallFile *e=p->files+i;if((!strcmp(e->path,"default.xex"))!=(pass==1))continue;
        char source[768],target[768];
        ok=update_install_path(source,sizeof source,root,UPDATE_INSTALL_WORK "/stage",e->path)&&update_install_path(target,sizeof target,root,"",e->path)&&!rename(source,target)&&checked_file(root,"",e,false);
#ifdef BLADES_INSTALL_TEST
        extern bool update_install_test_point(unsigned);if(ok)ok=update_install_test_point(i+1);
#endif
    }
    if(ok){remove_marker(root,"pending");ok=!present(root,"pending");}
    if(!ok){bool restored=rollback(root,p);free(p);problem(error,bytes,restored?"Installation failed. Previous version restored.":"Installation failed; recovery is pending. Keep the update backup.");return -1;}
    free(p);if(bytes)error[0]=0;return 1;
}
