#include "ftp.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
enum { FTP_TRANSFER_BYTES=256*1024,FTP_SOCKET_BYTES=63*1024 };
static void data_buffers(int fd){
    int bytes=FTP_SOCKET_BYTES;
    (void)setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&bytes,sizeof bytes);
    (void)setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&bytes,sizeof bytes);
}
typedef struct {
    struct FtpServer *server;pthread_t thread;bool started,running,authenticated;
    int control,passive;uint32_t peer;unsigned epoch;
    char cwd[FTP_PATH],rename_from[FTP_PATH],input[2048];size_t buffered;
    uint64_t restart;bool user_ok,rename_ready;
} Client;
struct FtpServer {
    pthread_mutex_t lock;pthread_t thread;
    unsigned stop,enabled,epoch;
    FtpSnapshot snapshot;FtpDrive drives[FTP_DRIVES];unsigned count,port;
    char bind_address[16],config[FTP_PATH];FtpOptions hooks;
    Client clients[FTP_CLIENTS];
};
static uint64_t milliseconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+(unsigned)t.tv_nsec/1000000;}
static bool stopped(FtpServer *s){return __atomic_load_n(&s->stop,__ATOMIC_ACQUIRE)!=0;}
static bool enabled(FtpServer *s){return __atomic_load_n(&s->enabled,__ATOMIC_ACQUIRE)!=0;}
static unsigned epoch(FtpServer *s){return __atomic_load_n(&s->epoch,__ATOMIC_ACQUIRE);}
static bool alive(Client *c){return !stopped(c->server)&&enabled(c->server)&&epoch(c->server)==c->epoch;}
static void counts(FtpServer *s){
    unsigned devices=0;
    for(unsigned i=0;i<FTP_CLIENTS;i++)if(s->clients[i].running&&s->clients[i].authenticated){
        bool seen=false;for(unsigned j=0;j<i;j++)if(s->clients[j].running&&s->clients[j].authenticated&&s->clients[j].peer==s->clients[i].peer)seen=true;
        if(!seen)devices++;
    }
    if(s->snapshot.devices!=devices){s->snapshot.devices=devices;s->snapshot.revision++;}
}
static void authenticate(Client *c,bool value){pthread_mutex_lock(&c->server->lock);c->authenticated=value;counts(c->server);pthread_mutex_unlock(&c->server->lock);}
static void state(FtpServer *s,FtpState value,const char *ip,unsigned port,int error){
    pthread_mutex_lock(&s->lock);
    if(s->snapshot.state!=value||strcmp(s->snapshot.ip,ip)||s->snapshot.port!=port||s->snapshot.error!=error){
        s->snapshot.state=value;s->snapshot.port=port;s->snapshot.error=error;snprintf(s->snapshot.ip,16,"%s",ip);s->snapshot.revision++;
    }
    pthread_mutex_unlock(&s->lock);
}
static int nonblocking(int fd){int flags=fcntl(fd,F_GETFL,0);return flags<0?-1:fcntl(fd,F_SETFL,flags|O_NONBLOCK);}
static bool wait_socket(Client *c,int fd,short events,uint64_t deadline){
    while(alive(c)&&milliseconds()<deadline){struct pollfd p={.fd=fd,.events=events};int n=poll(&p,1,100);if(n>0)return (p.revents&(events|POLLHUP))!=0;if(n<0&&errno!=EINTR)return false;}
    return false;
}
static bool send_all(Client *c,int fd,const void *data,size_t size){
    const char *p=data;uint64_t deadline=milliseconds()+30000;
    while(size&&alive(c)){
        ssize_t n=send(fd,p,size,MSG_NOSIGNAL);
        if(n>0){p+=n;size-=(size_t)n;deadline=milliseconds()+30000;}
        else if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)){if(!wait_socket(c,fd,POLLOUT,deadline))return false;}
        else return false;
    }
    return !size;
}
static bool reply(Client *c,const char *format,...){
    char line[2304];va_list ap;va_start(ap,format);int n=vsnprintf(line,sizeof line,format,ap);va_end(ap);
    return n>=0&&(size_t)n<sizeof line&&send_all(c,c->control,line,(size_t)n);
}
static bool line(Client *c,char out[2048]){
    uint64_t deadline=milliseconds()+300000;
    while(alive(c)){
        char *end=memchr(c->input,'\n',c->buffered);
        if(end){size_t n=(size_t)(end-c->input),used=n+1;if(n&&c->input[n-1]=='\r')n--;if(memchr(c->input,0,n))return false;
            memcpy(out,c->input,n);out[n]=0;memmove(c->input,c->input+used,c->buffered-used);c->buffered-=used;return true;}
        if(c->buffered==sizeof c->input-1){(void)reply(c,"500 Command too long.\r\n");return false;}
        if(!wait_socket(c,c->control,POLLIN,deadline))return false;
        ssize_t n=recv(c->control,c->input+c->buffered,sizeof c->input-1-c->buffered,0);
        if(n>0)c->buffered+=(size_t)n;else if(!n||(errno!=EINTR&&errno!=EAGAIN&&errno!=EWOULDBLOCK))return false;
    }
    return false;
}
static bool virtual_path(const char *cwd,const char *arg,char out[FTP_PATH]){
    char combined[FTP_PATH*2];int n=snprintf(combined,sizeof combined,arg[0]=='/'?"%s":"%s/%s",arg[0]=='/'?arg:cwd,arg);
    if(n<0||(size_t)n>=sizeof combined)return false;
    size_t used=0;out[0]=0;char *save=NULL;
    for(char *part=strtok_r(combined,"/",&save);part;part=strtok_r(NULL,"/",&save)){
        if(!strcmp(part,"."))continue;
        if(!strcmp(part,"..")){while(used&&out[used-1]!='/')used--;if(used)used--;out[used]=0;continue;}
        if(strpbrk(part,"\\:\r\n")||used+strlen(part)+1>=FTP_PATH)return false;
        out[used++]='/';memcpy(out+used,part,strlen(part)+1);used+=strlen(part);
    }
    if(!used)strcpy(out,"/");
    return true;
}
static bool path_stat(const char *p,struct stat *st){
#ifdef BLADES_XBOX
    return !stat(p,st);
#else
    return !lstat(p,st)&&!S_ISLNK(st->st_mode);
#endif
}
/* Resolve only exported drives; never expose the host or Xbox device namespace. */
static bool local_path(FtpServer *s,const char *virtual,char out[FTP_PATH],bool *drive){
    if(!strcmp(virtual,"/"))return false;
    const char *slash=strchr(virtual+1,'/');size_t n=slash?(size_t)(slash-virtual-1):strlen(virtual+1);
    const FtpDrive *d=NULL;for(unsigned i=0;i<s->count;i++)if(strlen(s->drives[i].name)==n&&!strncasecmp(s->drives[i].name,virtual+1,n))d=s->drives+i;
    if(!d)return false;
    *drive=!slash;
    int length=snprintf(out,FTP_PATH,"%s%s%s",d->path,slash&&d->path[strlen(d->path)-1]!='/'?"/":"",slash?slash+1:"");
    if(length<0||length>=FTP_PATH)return false;
    struct stat st;if(!path_stat(d->path,&st)||!S_ISDIR(st.st_mode))return false;
#ifndef BLADES_XBOX
    /* Do not follow symlinks in any exported component, including upload targets. */
    size_t base=strlen(d->path);for(size_t i=base;i<strlen(out);i++)if(out[i]=='/'){char ch=out[i];out[i]=0;bool ok=path_stat(out,&st)&&S_ISDIR(st.st_mode);out[i]=ch;if(!ok)return false;}
    if(!lstat(out,&st)&&S_ISLNK(st.st_mode))return false;
#endif
    return true;
}
static bool resolve(Client *c,const char *arg,char virt[FTP_PATH],char local[FTP_PATH],bool *drive){return virtual_path(c->cwd,arg,virt)&&local_path(c->server,virt,local,drive);}
static void passive_close(Client *c){if(c->passive>=0){close(c->passive);c->passive=-1;}}
static bool passive(Client *c,bool extended){
    passive_close(c);struct sockaddr_in local;socklen_t size=sizeof local;
    if(getsockname(c->control,(struct sockaddr *)&local,&size))return reply(c,"425 No local address.\r\n");
    int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return reply(c,"425 No data socket.\r\n");
    data_buffers(fd);
    local.sin_port=0;
    if(nonblocking(fd)||bind(fd,(struct sockaddr *)&local,sizeof local)||listen(fd,4)||getsockname(fd,(struct sockaddr *)&local,&size)){close(fd);return reply(c,"425 Cannot listen for data.\r\n");}
    c->passive=fd;unsigned port=ntohs(local.sin_port);uint32_t ip=ntohl(local.sin_addr.s_addr);
    if(extended)return reply(c,"229 Entering Extended Passive Mode (|||%u|).\r\n",port);
    return reply(c,"227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).\r\n",ip>>24,(ip>>16)&255,(ip>>8)&255,ip&255,port>>8,port&255);
}
static int data_open(Client *c){
    if(c->passive<0){(void)reply(c,"425 Use PASV or EPSV first.\r\n");return -1;}
    if(!reply(c,"150 Opening data connection.\r\n"))return -1;
    uint64_t deadline=milliseconds()+15000;int result=-1;
    while(wait_socket(c,c->passive,POLLIN,deadline)){
        struct sockaddr_in peer;socklen_t size=sizeof peer;int fd=accept(c->passive,(struct sockaddr *)&peer,&size);
        if(fd<0){if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)continue;break;}
        if(peer.sin_addr.s_addr!=c->peer||nonblocking(fd)){close(fd);continue;}data_buffers(fd);result=fd;break;
    }
    passive_close(c);if(result<0)(void)reply(c,"425 Data connection failed.\r\n");return result;
}
static uint64_t file_size(const struct stat *st){
#ifdef BLADES_XBOX
    /* FATX can contain a file above signed 32-bit off_t's limit. */
    return (uint32_t)st->st_size;
#else
    return (uint64_t)st->st_size;
#endif
}
static void timestamp(time_t value,char out[32],bool machine){struct tm t;gmtime_r(&value,&t);strftime(out,32,machine?"%Y%m%d%H%M%S":"%b %d %H:%M",&t);}
static bool entry(Client *c,int fd,const char *name,const struct stat *st,int mode){
    if(strpbrk(name,"\r\n"))return true;
    char buffer[1500],date[32];timestamp(st->st_mtime,date,mode!=0);bool dir=S_ISDIR(st->st_mode);int n;
    if(mode==2)n=snprintf(buffer,sizeof buffer,"%s\r\n",name);
    else if(mode==1)n=snprintf(buffer,sizeof buffer,"type=%s;size=%llu;modify=%s; %s\r\n",dir?"dir":"file",(unsigned long long)(dir?0:file_size(st)),date,name);
    else n=snprintf(buffer,sizeof buffer,"%crwxrwxrwx 1 xbox xbox %llu %s %s\r\n",dir?'d':'-',(unsigned long long)(dir?0:file_size(st)),date,name);
    return n>=0&&(size_t)n<sizeof buffer&&send_all(c,fd,buffer,(size_t)n);
}
static bool listing(Client *c,const char *arg,int mode){
    if(arg[0]=='-'&&mode!=1){const char *p=strchr(arg,' ');arg=p?p+1:"";}
    char virt[FTP_PATH],local[FTP_PATH];bool drive=false;struct stat st;
    if(!virtual_path(c->cwd,arg,virt))return reply(c,"550 Invalid path.\r\n");
    bool root=!strcmp(virt,"/");DIR *dir=NULL;
    if(!root){if(!local_path(c->server,virt,local,&drive)||!path_stat(local,&st))return reply(c,"550 Not found.\r\n");if(S_ISDIR(st.st_mode)){dir=opendir(local);if(!dir)return reply(c,"550 Cannot read directory.\r\n");}}
    int fd=data_open(c);if(fd<0){if(dir)closedir(dir);return true;}bool ok=true;
    if(root){for(unsigned i=0;i<c->server->count&&ok&&alive(c);i++){const FtpDrive *d=c->server->drives+i;if(path_stat(d->path,&st)&&S_ISDIR(st.st_mode))ok=entry(c,fd,d->name,&st,mode);}}
    else if(!dir){const char *name=strrchr(virt,'/');ok=entry(c,fd,name?name+1:virt,&st,mode);}
    else{
        while(ok&&alive(c)){errno=0;struct dirent *e=readdir(dir);if(!e){if(errno)ok=false;break;}if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
            char child[FTP_PATH];int n=snprintf(child,sizeof child,"%s/%s",local,e->d_name);if(n<0||n>=FTP_PATH){ok=false;break;}
            if(path_stat(child,&st)&&(S_ISREG(st.st_mode)||S_ISDIR(st.st_mode)))ok=entry(c,fd,e->d_name,&st,mode);
        }
        closedir(dir);
    }
    close(fd);return reply(c,ok?"226 Directory sent.\r\n":"426 Directory transfer failed.\r\n");
}
static bool write_file(Client *c,int file,const char *buffer,size_t bytes,unsigned *calls){
    while(bytes&&alive(c)){(*calls)++;ssize_t n=write(file,buffer,bytes);if(n>0){buffer+=n;bytes-=(size_t)n;}else if(n<0&&errno==EINTR)continue;else return false;}
    return bytes==0;
}
static bool transfer(Client *c,const char *arg,bool upload,bool append){
    char virt[FTP_PATH],local[FTP_PATH];bool drive=false;uint64_t restart=c->restart;c->restart=0;
    if(!resolve(c,arg,virt,local,&drive)||drive)return reply(c,"550 Invalid file path.\r\n");
    struct stat st;bool exists=path_stat(local,&st);
    if((exists&&!S_ISREG(st.st_mode))||(!upload&&!exists)||(restart&&!exists))return reply(c,"550 Not a regular file.\r\n");
    if(restart>(uint64_t)LONG_MAX)return reply(c,"551 Resume offset unsupported.\r\n");
    if(restart&&restart>file_size(&st))return reply(c,"551 Resume offset beyond file.\r\n");
    int file=-1;if(!upload){file=open(local,O_RDONLY);if(file<0)return reply(c,"550 Cannot open file.\r\n");}
    int fd=data_open(c);if(fd<0){if(file>=0)close(file);return true;}
    if(upload)file=open(local,O_WRONLY|O_CREAT|(append?O_APPEND:restart?0:O_TRUNC),0666);
    if(file<0){close(fd);return reply(c,"550 Cannot open file.\r\n");}
    bool ok=!restart||lseek(file,(off_t)restart,SEEK_SET)>=0;char *buffer=malloc(FTP_TRANSFER_BYTES);if(!buffer)ok=false;
    size_t used=0;
    uint64_t began=milliseconds();FtpTransfer stats={.upload=upload};socklen_t option_size=sizeof(int);
    (void)getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&stats.receive_buffer,&option_size);option_size=sizeof(int);
    (void)getsockopt(fd,SOL_SOCKET,SO_SNDBUF,&stats.send_buffer,&option_size);
    uint64_t deadline=milliseconds()+30000;
    while(ok&&alive(c)){
        /* Drain ready network data directly; poll only when recv would block.
           Aggregate short packets into one large FATX write, bypassing newlib's
           1 KiB stdio buffer. No per-packet disk calls or ready polls. */
        if(upload){ssize_t n=recv(fd,buffer+used,FTP_TRANSFER_BYTES-used,0);
            if(!n){if(used){ok=write_file(c,file,buffer,used,&stats.disk_calls);if(ok)stats.bytes+=used;}break;}
            if(n<0){if(errno==EINTR)continue;if(errno==EAGAIN||errno==EWOULDBLOCK){if(!wait_socket(c,fd,POLLIN,deadline)){ok=false;break;}continue;}ok=false;break;}
            used+=(size_t)n;deadline=milliseconds()+30000;
            if(used==FTP_TRANSFER_BYTES){ok=write_file(c,file,buffer,used,&stats.disk_calls);if(ok)stats.bytes+=used;used=0;}
        }else{stats.disk_calls++;ssize_t n=read(file,buffer,FTP_TRANSFER_BYTES);if(n<0){if(errno==EINTR)continue;ok=false;break;}if(!n)break;if(!send_all(c,fd,buffer,(size_t)n)){ok=false;break;}stats.bytes+=(unsigned)n;}
    }
    if(!alive(c))ok=false;
    free(buffer);if(close(file))ok=false;close(fd);
    stats.success=ok;stats.elapsed_ms=(unsigned)(milliseconds()-began);
    if(c->server->hooks.transfer_done)c->server->hooks.transfer_done(c->server->hooks.context,&stats);
    return reply(c,ok?"226 Transfer complete.\r\n":"426 Transfer interrupted or storage error.\r\n");
}
static bool command(Client *c,char *text){
    char *arg=strchr(text,' ');if(arg)*arg++=0;else arg=text+strlen(text);
    for(char *p=text;*p;p++)*p=(char)toupper((unsigned char)*p);
    bool rename_ready=c->rename_ready;c->rename_ready=false;
    if(!strcmp(text,"QUIT")){(void)reply(c,"221 Goodbye.\r\n");return false;}
    if(!strcmp(text,"USER")){authenticate(c,false);pthread_mutex_lock(&c->server->lock);c->user_ok=!strcmp(arg,c->server->snapshot.username);pthread_mutex_unlock(&c->server->lock);return reply(c,"331 Password required.\r\n");}
    if(!strcmp(text,"PASS")){pthread_mutex_lock(&c->server->lock);bool ok=c->user_ok&&!strcmp(arg,c->server->snapshot.password);pthread_mutex_unlock(&c->server->lock);authenticate(c,ok);return reply(c,ok?"230 Logged in.\r\n":"530 Login incorrect.\r\n");}
    if(!strcmp(text,"AUTH"))return reply(c,"502 Plain FTP only.\r\n");
    if(!strcmp(text,"FEAT"))return reply(c,"211-Features\r\n EPSV\r\n PASV\r\n UTF8\r\n SIZE\r\n MDTM\r\n REST STREAM\r\n MLST type*;size*;modify*;\r\n211 End\r\n");
    if(!strcmp(text,"SYST"))return reply(c,"215 UNIX Type: L8\r\n");
    if(!strcmp(text,"NOOP"))return reply(c,"200 OK.\r\n");
    if(!c->authenticated)return reply(c,"530 Log in first.\r\n");
    if(!strcmp(text,"PWD")||!strcmp(text,"XPWD")){char quoted[FTP_PATH*2];size_t n=0;for(const char *p=c->cwd;*p;p++){quoted[n++]=*p;if(*p=='"')quoted[n++]='"';}quoted[n]=0;return reply(c,"257 \"%s\"\r\n",quoted);}
    if(!strcmp(text,"TYPE"))return reply(c,!strcmp(arg,"I")||!strcmp(arg,"A")?"200 Type accepted.\r\n":"504 Type unsupported.\r\n");
    if(!strcmp(text,"OPTS"))return reply(c,!strcasecmp(arg,"UTF8 ON")||!strncasecmp(arg,"MLST ",5)?"200 Option accepted.\r\n":"501 Option unsupported.\r\n");
    if(!strcmp(text,"PASV"))return passive(c,false);
    if(!strcmp(text,"EPSV"))return !arg[0]||!strcmp(arg,"1")?passive(c,true):reply(c,"522 IPv4 only.\r\n");
    if(!strcmp(text,"LIST")||!strcmp(text,"MLSD")||!strcmp(text,"NLST"))return listing(c,arg,!strcmp(text,"MLSD")?1:!strcmp(text,"NLST")?2:0);
    if(!strcmp(text,"RETR"))return transfer(c,arg,false,false);
    if(!strcmp(text,"STOR")||!strcmp(text,"APPE"))return transfer(c,arg,true,!strcmp(text,"APPE"));
    if(!strcmp(text,"REST")){char *end=NULL;errno=0;unsigned long long n=strtoull(arg,&end,10);if(!isdigit((unsigned char)*arg)||errno||!end||*end||n>(uint64_t)LONG_MAX)return reply(c,"501 Invalid resume offset.\r\n");c->restart=n;return reply(c,"350 Resume accepted.\r\n");}
    if(!strcmp(text,"ALLO"))return reply(c,"202 Allocation unnecessary.\r\n");
    if(!strcmp(text,"ABOR")){passive_close(c);c->restart=0;return reply(c,"225 No transfer active.\r\n");}
    if(!strcmp(text,"CWD")||!strcmp(text,"XCWD")||!strcmp(text,"CDUP")){
        char virt[FTP_PATH],local[FTP_PATH];bool drive;struct stat st;
        if(!virtual_path(c->cwd,!strcmp(text,"CDUP")?"..":arg,virt))return reply(c,"550 Invalid directory.\r\n");
        if(strcmp(virt,"/")&&(!local_path(c->server,virt,local,&drive)||!path_stat(local,&st)||!S_ISDIR(st.st_mode)))return reply(c,"550 Directory unavailable.\r\n");
        snprintf(c->cwd,sizeof c->cwd,"%s",virt);return reply(c,"250 Directory changed.\r\n");
    }
    char virt[FTP_PATH],local[FTP_PATH];bool drive=false;struct stat st;
    if(!strcmp(text,"RNTO")){
        if(!rename_ready)return reply(c,"503 RNFR required.\r\n");
        if(!resolve(c,arg,virt,local,&drive)||drive)return reply(c,"550 Invalid destination.\r\n");
        /* Renames stay on one exported storage device. */
        if(strchr(c->rename_from,':')&&strchr(local,':')&&strncasecmp(c->rename_from,local,(size_t)(strchr(c->rename_from,':')-c->rename_from)+1))return reply(c,"550 Different storage device.\r\n");
        return reply(c,!rename(c->rename_from,local)?"250 Renamed.\r\n":"550 Rename failed.\r\n");
    }
    if(!strcmp(text,"MLST")&&!strcmp(arg[0]?arg:c->cwd,"/"))return reply(c,"250-Entry\r\n type=dir; /\r\n250 End\r\n");
    bool known=!strcmp(text,"SIZE")||!strcmp(text,"MDTM")||!strcmp(text,"MLST")||!strcmp(text,"MKD")||!strcmp(text,"XMKD")||!strcmp(text,"RMD")||!strcmp(text,"XRMD")||!strcmp(text,"DELE")||!strcmp(text,"RNFR");
    if(!known)return reply(c,"502 Command unsupported; use passive mode.\r\n");
    if(!resolve(c,arg,virt,local,&drive))return reply(c,"550 Invalid path.\r\n");
    if(!strcmp(text,"MKD")||!strcmp(text,"XMKD"))return reply(c,!drive&&!mkdir(local,0777)?"257 Directory created.\r\n":"550 Cannot create directory.\r\n");
    if(!path_stat(local,&st))return reply(c,"550 Not found.\r\n");
    if(!strcmp(text,"SIZE"))return S_ISREG(st.st_mode)?reply(c,"213 %llu\r\n",(unsigned long long)file_size(&st)):reply(c,"550 Not a file.\r\n");
    if(!strcmp(text,"MDTM")){char date[32];timestamp(st.st_mtime,date,true);return reply(c,"213 %s\r\n",date);}
    if(!strcmp(text,"MLST")){if(!reply(c,"250-Entry\r\n "))return false;if(!entry(c,c->control,virt,&st,1))return false;return reply(c,"250 End\r\n");}
    if(drive)return reply(c,"550 Cannot modify a storage root.\r\n");
    if(!strcmp(text,"RNFR")){snprintf(c->rename_from,sizeof c->rename_from,"%s",local);c->rename_ready=true;return reply(c,"350 Destination required.\r\n");}
    bool ok=!strcmp(text,"DELE")?(S_ISREG(st.st_mode)&&!unlink(local)):(S_ISDIR(st.st_mode)&&!rmdir(local));return reply(c,ok?"250 Removed.\r\n":"550 Cannot remove entry.\r\n");
}
static void *client_main(void *arg){
    Client *c=arg;FtpServer *s=c->server;if(s->hooks.worker_start)s->hooks.worker_start(s->hooks.context);
    char buffer[2048];bool ok=reply(c,"220 Blades plain FTP ready.\r\n");
    while(ok&&line(c,buffer))ok=command(c,buffer);
    passive_close(c);close(c->control);pthread_mutex_lock(&s->lock);c->authenticated=false;c->running=false;counts(s);pthread_mutex_unlock(&s->lock);return NULL;
}
static bool login_valid(const char *text){
    size_t n=strlen(text);if(!n||n>=FTP_LOGIN_BYTES)return false;
    for(size_t i=0;i<n;i++)if((unsigned char)text[i]<32||(unsigned char)text[i]>126)return false;
    return true;
}
static int load_setting(FtpServer *s,char username[FTP_LOGIN_BYTES],char password[FTP_LOGIN_BYTES]){
    if(!s->config[0])return 1;
    FILE *f=fopen(s->config,"rb");if(!f)return errno==ENOENT?1:-1;
    char text[256];size_t n=fread(text,1,sizeof text-1,f);bool ok=!ferror(f)&&feof(f);fclose(f);text[n]=0;
    if(!ok||n<10||memcmp(text,"enabled=",8)||(text[8]!='0'&&text[8]!='1')||text[9]!='\n')return -1;
    int value=text[8]=='1';if(n==10)return value; /* Upgrade v0.30's boolean-only file. */
    char *user=text+10;if(strncmp(user,"username=",9))return -1;user+=9;
    char *end=strchr(user,'\n');if(!end)return -1;*end++=0;
    if(strncmp(end,"password=",9))return -1;
    char *pass=end+9;end=strchr(pass,'\n');if(!end||end[1])return -1;*end=0;
    if(!login_valid(user)||!login_valid(pass))return -1;
    snprintf(username,FTP_LOGIN_BYTES,"%s",user);snprintf(password,FTP_LOGIN_BYTES,"%s",pass);return value;
}
static bool save_setting(FtpServer *s){
    if(!s->config[0])return true;
    pthread_mutex_lock(&s->lock);FtpSnapshot value=s->snapshot;pthread_mutex_unlock(&s->lock);
    char parent[FTP_PATH],temp[FTP_PATH];snprintf(parent,sizeof parent,"%s",s->config);char *slash=strrchr(parent,'/');if(slash){*slash=0;if(mkdir(parent,0777)&&errno!=EEXIST)return false;}
    int n=snprintf(temp,sizeof temp,"%s.tmp",s->config);if(n<0||n>=FTP_PATH)return false;
    FILE *f=fopen(temp,"wb");if(!f)return false;
    bool ok=fprintf(f,"enabled=%u\nusername=%s\npassword=%s\n",value.enabled?1:0,value.username,value.password)>0;
    if(fflush(f))ok=false;
    if(ok&&fsync(fileno(f)))ok=false;
    if(fclose(f))ok=false;
    if(ok&&rename(temp,s->config))ok=false;
    if(!ok)remove(temp);
    return ok;
}
static void saved(FtpServer *s,bool ok){pthread_mutex_lock(&s->lock);if(s->snapshot.save_error==ok){s->snapshot.save_error=!ok;s->snapshot.revision++;}pthread_mutex_unlock(&s->lock);}
static int listener(FtpServer *s,unsigned *port){
    int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return -1;int yes=1;(void)setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);
    struct sockaddr_in local={.sin_family=AF_INET,.sin_port=htons((uint16_t)s->port)};socklen_t size=sizeof local;
    if(inet_pton(AF_INET,s->bind_address,&local.sin_addr)!=1||nonblocking(fd)||bind(fd,(struct sockaddr *)&local,sizeof local)||listen(fd,FTP_CLIENTS)||getsockname(fd,(struct sockaddr *)&local,&size)){int error=errno;close(fd);errno=error;return -1;}
    *port=ntohs(local.sin_port);return fd;
}
static void *server_main(void *arg){
    FtpServer *s=arg;if(s->hooks.worker_start)s->hooks.worker_start(s->hooks.context);
    char username[FTP_LOGIN_BYTES]=FTP_USERNAME,password[FTP_LOGIN_BYTES]=FTP_PASSWORD;
    int setting=load_setting(s,username,password);unsigned saved_epoch=1;
    pthread_mutex_lock(&s->lock);if(epoch(s)==1){__atomic_store_n(&s->enabled,setting!=0,__ATOMIC_RELEASE);s->snapshot.enabled=setting!=0;snprintf(s->snapshot.username,FTP_LOGIN_BYTES,"%s",username);snprintf(s->snapshot.password,FTP_LOGIN_BYTES,"%s",password);s->snapshot.revision++;}s->snapshot.save_error=setting<0;pthread_mutex_unlock(&s->lock);
    int fd=-1;unsigned port=s->port;uint64_t retry=0,address_at=0;char ip[16]="";
    while(!stopped(s)){
        unsigned current=epoch(s);if(current!=saved_epoch){bool ok=save_setting(s);saved(s,ok);saved_epoch=current;}
        if(milliseconds()>=address_at){bool ready=true;address_at=milliseconds()+1000;if(s->hooks.address)ready=s->hooks.address(s->hooks.context,ip);else snprintf(ip,sizeof ip,"%s",s->bind_address);if(!ready)ip[0]=0;}
        if(!enabled(s)){if(fd>=0){close(fd);fd=-1;}state(s,FTP_OFF,ip,port,0);}
        else{
            if(!ip[0]){if(fd>=0){close(fd);fd=-1;}state(s,FTP_WAITING,"",port,0);}
            else if(fd<0&&milliseconds()>=retry){fd=listener(s,&port);retry=milliseconds()+2000;state(s,fd<0?FTP_ERROR:FTP_RUNNING,ip,port,fd<0?errno:0);}
            else if(fd>=0)state(s,FTP_RUNNING,ip,port,0);
        }
        if(fd>=0&&enabled(s)){
            struct pollfd p={.fd=fd,.events=POLLIN};int n=poll(&p,1,100);
            if(n>0&&(p.revents&POLLIN)){
                struct sockaddr_in peer;socklen_t size=sizeof peer;int control=accept(fd,(struct sockaddr *)&peer,&size);if(control<0)continue;
                Client *c=NULL;pthread_mutex_lock(&s->lock);for(unsigned i=0;i<FTP_CLIENTS;i++)if(!s->clients[i].running){c=s->clients+i;break;}pthread_mutex_unlock(&s->lock);
                if(!c||nonblocking(control)){close(control);continue;}if(c->started)pthread_join(c->thread,NULL);
                int yes=1;(void)setsockopt(control,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof yes);
                pthread_mutex_lock(&s->lock);memset(c,0,sizeof *c);c->server=s;c->control=control;c->passive=-1;c->peer=peer.sin_addr.s_addr;c->epoch=epoch(s);strcpy(c->cwd,"/");pthread_mutex_unlock(&s->lock);
                pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,128*1024);
                pthread_mutex_lock(&s->lock);c->running=true;int error=pthread_create(&c->thread,&attr,client_main,c);c->started=!error;if(error)c->running=false;pthread_mutex_unlock(&s->lock);pthread_attr_destroy(&attr);if(error)close(control);
            }
        }else{struct timespec delay={0,100000000};nanosleep(&delay,NULL);}
    }
    if(fd>=0)close(fd);
    for(unsigned i=0;i<FTP_CLIENTS;i++)if(s->clients[i].started)pthread_join(s->clients[i].thread,NULL);
    if(epoch(s)!=saved_epoch)saved(s,save_setting(s));
    return NULL;
}
FtpServer *ftp_open(const FtpOptions *o){
    if(!o||o->count>FTP_DRIVES||!o->count||!o->drives||o->port>65535)return NULL;
    FtpServer *s=calloc(1,sizeof *s);if(!s)return NULL;s->hooks=*o;s->count=o->count;s->port=o->port;
    for(unsigned i=0;i<s->count;i++){if(!o->drives[i].name[0]||strpbrk(o->drives[i].name,"/\\:\r\n")||!o->drives[i].path[0]){free(s);return NULL;}s->drives[i]=o->drives[i];}
    snprintf(s->bind_address,sizeof s->bind_address,"%s",o->bind_address?o->bind_address:"0.0.0.0");snprintf(s->config,sizeof s->config,"%s",o->config?o->config:"");
    s->enabled=1;s->epoch=1;s->snapshot=(FtpSnapshot){.state=FTP_STARTING,.enabled=true,.port=o->port,.revision=1,.username=FTP_USERNAME,.password=FTP_PASSWORD};pthread_mutex_init(&s->lock,NULL);
    pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,128*1024);int error=pthread_create(&s->thread,&attr,server_main,s);pthread_attr_destroy(&attr);
    if(error){pthread_mutex_destroy(&s->lock);free(s);return NULL;}return s;
}
void ftp_poll(FtpServer *s,FtpSnapshot *out){if(!out)return;if(!s){*out=(FtpSnapshot){.state=FTP_ERROR,.error=ENOMEM};return;}pthread_mutex_lock(&s->lock);*out=s->snapshot;pthread_mutex_unlock(&s->lock);}
void ftp_enable(FtpServer *s,bool value){if(!s)return;pthread_mutex_lock(&s->lock);if(enabled(s)!=value){__atomic_store_n(&s->enabled,value,__ATOMIC_RELEASE);__atomic_add_fetch(&s->epoch,1,__ATOMIC_ACQ_REL);s->snapshot.enabled=value;s->snapshot.state=value?FTP_STARTING:FTP_OFF;s->snapshot.revision++;}pthread_mutex_unlock(&s->lock);}
bool ftp_credentials(FtpServer *s,const char *username,const char *password){
    if(!s||!username||!password||!login_valid(username)||!login_valid(password))return false;
    pthread_mutex_lock(&s->lock);
    if(strcmp(s->snapshot.username,username)||strcmp(s->snapshot.password,password)){
        snprintf(s->snapshot.username,FTP_LOGIN_BYTES,"%s",username);snprintf(s->snapshot.password,FTP_LOGIN_BYTES,"%s",password);
        __atomic_add_fetch(&s->epoch,1,__ATOMIC_ACQ_REL);s->snapshot.revision++;
    }
    pthread_mutex_unlock(&s->lock);return true;
}
void ftp_close(FtpServer *s){if(!s)return;__atomic_store_n(&s->stop,1,__ATOMIC_RELEASE);pthread_join(s->thread,NULL);pthread_mutex_destroy(&s->lock);free(s);}
const char *ftp_status(const FtpSnapshot *s){if(s->save_error)return s->enabled?"On - setting not saved":"Off - setting not saved";switch(s->state){case FTP_OFF:return "Off";case FTP_STARTING:return "Starting";case FTP_RUNNING:return "Running";case FTP_WAITING:return "Waiting for network";case FTP_ERROR:return "Unavailable";}return "Unavailable";}
