#include "updater.h"
#include "update_net.h"
#include "update_install.h"
#include "protocol.h"
#include "mbedtls/sha256.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
struct Updater {
    pthread_t thread;pthread_mutex_t lock;pthread_cond_t wake;
    unsigned stop,cancel,command;bool busy;UpdateSnapshot view;UpdateManifest manifest;
    char directory[256],install_root[256];void (*worker_start)(void *);void *context;
};
static bool version(const char *s,unsigned part[3]){
    memset(part,0,3*sizeof *part);if(!s||!*s)return false;
    for(unsigned i=0;i<3;i++){if(*s<'0'||*s>'9')return false;unsigned n=0;
        while(*s>='0'&&*s<='9'){if(n>100000)return false;n=n*10+(unsigned)(*s++-'0');}part[i]=n;
        if(!*s)return i>=1;
        if(*s++!='.')return false;
    }return !*s;
}
int update_version_compare(const char *a,const char *b){unsigned x[3],y[3];if(!version(a,x)||!version(b,y))return 0;for(unsigned i=0;i<3;i++)if(x[i]!=y[i])return x[i]>y[i]?1:-1;return 0;}
bool update_manifest_parse(const char *text,size_t bytes,UpdateManifest *out){
    UpdateManifest m={0};xf_json_ref root={text,bytes},ref;uint64_t schema=0;unsigned parts[3];
    if(xf_json_get(root,"schema",&ref)!=1||!xf_json_uint(ref,&schema)||schema!=1||xf_json_string(text,bytes,"version",m.version,sizeof m.version)!=1||!version(m.version,parts)||
       xf_json_string(text,bytes,"asset",m.filename,sizeof m.filename)!=1||xf_json_string(text,bytes,"sha256",m.sha256,sizeof m.sha256)!=1||strlen(m.sha256)!=64||
       xf_json_get(root,"size",&ref)!=1||!xf_json_uint(ref,&m.bytes)||m.bytes<4096||m.bytes>64*1024*1024)return false;
    for(unsigned i=0;i<64;i++){char c=m.sha256[i];if(c>='A'&&c<='F')m.sha256[i]=(char)(c+'a'-'A');else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;}
    size_t n=strlen(m.filename);if(n<17||strncmp(m.filename,"Blades-v",8)||strcmp(m.filename+n-8,"-USB.zip"))return false;
    char asset_version[24];size_t count=n-16;if(count>=sizeof asset_version)return false;memcpy(asset_version,m.filename+8,count);asset_version[count]=0;
    if(!version(asset_version,parts)||update_version_compare(asset_version,m.version))return false;
    *out=m;return true;
}
static bool cancelled(void *context){Updater *u=context;return __atomic_load_n(&u->stop,__ATOMIC_ACQUIRE)||__atomic_load_n(&u->cancel,__ATOMIC_ACQUIRE);}
static void state(Updater *u,UpdateState value,const char *message){pthread_mutex_lock(&u->lock);u->view.state=value;snprintf(u->view.message,sizeof u->view.message,"%s",message);u->view.revision++;pthread_mutex_unlock(&u->lock);}
typedef struct {char data[16385];size_t bytes;} Metadata;
static bool metadata(void *context,const unsigned char *data,size_t bytes){Metadata *m=context;if(bytes>sizeof m->data-1-m->bytes)return false;memcpy(m->data+m->bytes,data,bytes);m->bytes+=bytes;m->data[m->bytes]=0;return true;}
static bool update_address(char *out,size_t bytes,const char *filename){
    const char *origin="https://raw.githubusercontent.com";
#ifndef BLADES_XBOX
    const char *test=getenv("BLADES_UPDATE_TEST_ORIGIN");if(test)origin=test;
#endif
    int n=snprintf(out,bytes,"%s" BLADES_UPDATE_PATH "%s",origin,filename);
    return n>0&&(size_t)n<bytes;
}
static void check(Updater *u){
    Metadata *m=calloc(1,sizeof *m);char error[192]={0};if(!m){state(u,UPDATE_ERROR,"Not enough memory to check updates.");return;}
    state(u,UPDATE_CHECKING,"Checking GitHub for updates...");
    char url[2048];bool ok=update_address(url,sizeof url,"update.json");
    if(ok)ok=update_https(url,16384,metadata,m,cancelled,u,error,sizeof error);
    else snprintf(error,sizeof error,"Invalid update source address.");
    UpdateManifest manifest;
    if(ok&&!update_manifest_parse(m->data,m->bytes,&manifest)){ok=false;snprintf(error,sizeof error,"GitHub returned invalid update information.");}
    free(m);
    if(cancelled(u)){state(u,UPDATE_CANCELLED,"Update check cancelled.");return;}
    if(!ok){state(u,UPDATE_ERROR,error);return;}
    u->manifest=manifest;
    pthread_mutex_lock(&u->lock);snprintf(u->view.version,sizeof u->view.version,"%s",manifest.version);snprintf(u->view.filename,sizeof u->view.filename,"%s",manifest.filename);u->view.total=manifest.bytes;u->view.received=0;pthread_mutex_unlock(&u->lock);
    state(u,update_version_compare(manifest.version,BLADES_VERSION)>0?UPDATE_AVAILABLE:UPDATE_CURRENT,update_version_compare(manifest.version,BLADES_VERSION)>0?"A newer version is available.":"You already have the latest version.");
}
typedef struct {Updater *updater;int fd;uint64_t bytes;mbedtls_sha256_context hash;} Download;
static bool download_bytes(void *context,const unsigned char *data,size_t bytes){
    Download *d=context;if(d->bytes+bytes>d->updater->manifest.bytes||cancelled(d->updater))return false;
    const unsigned char *p=data;size_t left=bytes;
    while(left&&!cancelled(d->updater)){ssize_t n=write(d->fd,p,left);if(n>0){left-=(size_t)n;p+=n;}else if(n<0&&errno==EINTR)continue;else return false;}
    if(left||mbedtls_sha256_update(&d->hash,data,bytes))return false;
    d->bytes+=bytes;
    pthread_mutex_lock(&d->updater->lock);d->updater->view.received=d->bytes;d->updater->view.revision++;pthread_mutex_unlock(&d->updater->lock);return true;
}
static bool verified_zip(Updater *u,const char *path,const UpdateManifest *manifest){
    FILE *f=fopen(path,"rb");if(!f)return false;mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);bool ok=mbedtls_sha256_starts(&ctx,0)==0;unsigned char data[32768],digest[32]={0};uint64_t bytes=0;
    while(ok&&!cancelled(u)){size_t n=fread(data,1,sizeof data,f);if(!n)break;bytes+=n;if(bytes>manifest->bytes||mbedtls_sha256_update(&ctx,data,n))ok=false;}
    ok=ok&&!ferror(f)&&bytes==manifest->bytes;if(mbedtls_sha256_finish(&ctx,digest))ok=false;mbedtls_sha256_free(&ctx);fclose(f);
    char hash[65];for(unsigned i=0;i<32;i++)snprintf(hash+i*2,3,"%02x",digest[i]);return ok&&!cancelled(u)&&!strcmp(hash,manifest->sha256);
}
static void install_progress(void *context,unsigned done,unsigned total){
    Updater *u=context;pthread_mutex_lock(&u->lock);u->view.received=done;u->view.total=total;u->view.revision++;pthread_mutex_unlock(&u->lock);
}
static void ready(Updater *u,const char *path){
    pthread_mutex_lock(&u->lock);snprintf(u->view.saved,sizeof u->view.saved,"%s",path);pthread_mutex_unlock(&u->lock);
    if(u->install_root[0]){
        state(u,UPDATE_PREPARING,"Preparing the update...");char error[192];
        if(!update_install_stage(u->install_root,path,cancelled,install_progress,u,error,sizeof error)){state(u,cancelled(u)?UPDATE_CANCELLED:UPDATE_ERROR,error);return;}
        state(u,UPDATE_READY,"Installing and restarting Blades...");
    }else state(u,UPDATE_READY,"Desktop preview: download verified. Automatic restart runs on Xbox.");
}
static void download(Updater *u){
    char final[384],temporary[392],url[2048],error[192]={0};struct stat st;
    int count=snprintf(final,sizeof final,"%s/%s",u->directory,u->manifest.filename);
    if(count<0||(size_t)count>=sizeof final||strlen(final)>=sizeof u->view.saved){state(u,UPDATE_ERROR,"The update save path is too long.");return;}
    if((mkdir(u->directory,0755)&&errno!=EEXIST)||stat(u->directory,&st)||!S_ISDIR(st.st_mode)){state(u,UPDATE_ERROR,"Could not create the Updates folder.");return;}
    if(!stat(final,&st)&&verified_zip(u,final,&u->manifest)){ready(u,final);return;}
    snprintf(temporary,sizeof temporary,"%s.tmp",final);int fd=open(temporary,O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0){state(u,UPDATE_ERROR,"Could not write to the Updates folder.");return;}
    Download d={.updater=u,.fd=fd};mbedtls_sha256_init(&d.hash);bool ok=mbedtls_sha256_starts(&d.hash,0)==0;
    if(!update_address(url,sizeof url,u->manifest.filename))ok=false;
    state(u,UPDATE_DOWNLOADING,"Downloading the USB ZIP...");
    if(ok)ok=update_https(url,u->manifest.bytes,download_bytes,&d,cancelled,u,error,sizeof error);
    unsigned char digest[32]={0};char actual[65];if(mbedtls_sha256_finish(&d.hash,digest))ok=false;mbedtls_sha256_free(&d.hash);
    for(unsigned i=0;i<32;i++)snprintf(actual+i*2,3,"%02x",digest[i]);
    if(ok&&fsync(fd))ok=false;
    if(close(fd))ok=false;
    if(ok&&(d.bytes!=u->manifest.bytes||strcmp(actual,u->manifest.sha256))){ok=false;snprintf(error,sizeof error,"The update checksum or size did not match. Download discarded.");}
    if(cancelled(u)){ok=false;snprintf(error,sizeof error,"Update download cancelled.");}
    if(ok&&rename(temporary,final)){ok=false;snprintf(error,sizeof error,"Could not save the completed update ZIP.");}
    if(!ok){remove(temporary);state(u,cancelled(u)?UPDATE_CANCELLED:UPDATE_ERROR,error[0]?error:"Update download failed. Check free storage space.");return;}
    ready(u,final);
}
static void *worker(void *context){
    Updater *u=context;if(u->worker_start)u->worker_start(u->context);
    for(;;){pthread_mutex_lock(&u->lock);while(!u->command&&!u->stop)pthread_cond_wait(&u->wake,&u->lock);if(u->stop){pthread_mutex_unlock(&u->lock);break;}
        unsigned command=u->command;u->command=0;pthread_mutex_unlock(&u->lock);if(command==1)check(u);else download(u);
        pthread_mutex_lock(&u->lock);u->busy=false;pthread_mutex_unlock(&u->lock);
    }return NULL;
}
Updater *updater_open(const char *directory,void (*start)(void *),void *context){
    if(!directory||!*directory||strlen(directory)>=256)return NULL;
    Updater *u=calloc(1,sizeof *u);if(!u)return NULL;snprintf(u->directory,sizeof u->directory,"%s",directory);u->worker_start=start;u->context=context;u->view.revision=1;
    snprintf(u->view.message,sizeof u->view.message,"Check GitHub for a newer version of Blades.");pthread_mutex_init(&u->lock,NULL);pthread_cond_init(&u->wake,NULL);
    pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,512*1024);int error=pthread_create(&u->thread,&attr,worker,u);pthread_attr_destroy(&attr);
    if(error){pthread_cond_destroy(&u->wake);pthread_mutex_destroy(&u->lock);free(u);return NULL;}return u;
}
void updater_poll(Updater *u,UpdateSnapshot *out){if(!out)return;if(!u){*out=(UpdateSnapshot){.state=UPDATE_ERROR};snprintf(out->message,sizeof out->message,"Updater unavailable.");return;}pthread_mutex_lock(&u->lock);*out=u->view;pthread_mutex_unlock(&u->lock);}
static bool request(Updater *u,unsigned command){if(!u)return false;pthread_mutex_lock(&u->lock);bool ok=!u->busy&&!u->stop&&(command==1||((u->view.state==UPDATE_AVAILABLE||u->view.state==UPDATE_ERROR||u->view.state==UPDATE_CANCELLED)&&u->manifest.filename[0]&&update_version_compare(u->manifest.version,BLADES_VERSION)>0));if(ok){__atomic_store_n(&u->cancel,0,__ATOMIC_RELEASE);u->view.received=0;u->busy=true;u->command=command;u->view.state=command==1?UPDATE_CHECKING:UPDATE_DOWNLOADING;u->view.revision++;pthread_cond_broadcast(&u->wake);}pthread_mutex_unlock(&u->lock);return ok;}
bool updater_check(Updater *u){return request(u,1);}
bool updater_download(Updater *u){return request(u,2);}
bool updater_install_to(Updater *u,const char *root){
    if(!u||!root||!*root||strlen(root)>=sizeof u->install_root)return false;
    pthread_mutex_lock(&u->lock);bool ok=!u->busy;if(ok)strcpy(u->install_root,root);pthread_mutex_unlock(&u->lock);return ok;
}
void updater_install_failed(Updater *u,const char *message){if(u)state(u,UPDATE_ERROR,message);}
void updater_cancel(Updater *u){if(u)__atomic_store_n(&u->cancel,1,__ATOMIC_RELEASE);}
void updater_close(Updater *u){if(!u)return;pthread_mutex_lock(&u->lock);__atomic_store_n(&u->stop,1,__ATOMIC_RELEASE);__atomic_store_n(&u->cancel,1,__ATOMIC_RELEASE);pthread_cond_broadcast(&u->wake);pthread_mutex_unlock(&u->lock);pthread_join(u->thread,NULL);pthread_cond_destroy(&u->wake);pthread_mutex_destroy(&u->lock);free(u);}
