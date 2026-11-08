/* Dedicated HTTPS transport for release updates. Jellyfin's HTTP/player path
   is intentionally independent. Mbed TLS verifies both chain and hostname. */
#include "update_net.h"
#include "protocol.h"
#include "update_roots.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#ifdef BLADES_XBOX
#include "xsdk/kernel.h"
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
typedef struct {char host[256],path[1536];unsigned port;} Url;
typedef struct {
    int fd;UpdateCancelled cancel;void *context;double deadline,total;
    mbedtls_ssl_context ssl;mbedtls_ssl_config config;mbedtls_x509_crt roots;
    mbedtls_ctr_drbg_context random;mbedtls_entropy_context entropy;
} Tls;
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
mbedtls_ms_time_t mbedtls_ms_time(void){return (mbedtls_ms_time_t)(seconds()*1000);}
static bool cancelled(Tls *s){return (s->cancel&&s->cancel(s->context))||seconds()>s->deadline||seconds()>s->total;}
static bool wait_socket(Tls *s,short events){
    while(!cancelled(s)){struct pollfd p={.fd=s->fd,.events=events};int n=poll(&p,1,100);if(n>0)return (p.revents&(events|POLLHUP))!=0;if(n<0&&errno!=EINTR)return false;}return false;
}
int mbedtls_hardware_poll(void *context,unsigned char *out,size_t length,size_t *received){
    (void)context;*received=0;
#ifdef BLADES_XBOX
    XeCryptRandom(out,(uint32_t)length);*received=length;return 0;
#else
    FILE *f=fopen("/dev/urandom","rb");if(!f)return -1;*received=fread(out,1,length,f);fclose(f);return *received==length?0:-1;
#endif
}
static bool parse(const char *text,Url *u){
    if(strncmp(text,"https://",8))return false;
    const char *host=text+8,*slash=strchr(host,'/');if(!slash)return false;size_t n=(size_t)(slash-host);if(!n||n>=sizeof u->host||strlen(slash)>=sizeof u->path)return false;
    memcpy(u->host,host,n);u->host[n]=0;snprintf(u->path,sizeof u->path,"%s",slash);u->port=443;
    char *port=strchr(u->host,':');if(port){char *end;unsigned long p=strtoul(port+1,&end,10);if(!p||p>65535||*end)return false;*port=0;u->port=(unsigned)p;}
    for(const char *p=u->host;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'))return false;
    for(const char *p=u->path;*p;p++)if((unsigned char)*p<33||(unsigned char)*p>126||*p=='#'||*p=='\\')return false;
#ifndef BLADES_XBOX
    if(getenv("BLADES_UPDATE_TEST_ORIGIN")&&(!strcmp(u->host,"localhost")||!strcmp(u->host,"127.0.0.1")))return true;
#endif
    return u->port==443&&(!strcmp(u->host,"raw.githubusercontent.com")||!strcmp(u->host,"github.com")||!strcmp(u->host,"release-assets.githubusercontent.com")||!strcmp(u->host,"objects.githubusercontent.com"));
}
static int send_tls(void *context,const unsigned char *data,size_t bytes){
    Tls *s=context;if(cancelled(s))return MBEDTLS_ERR_SSL_TIMEOUT;
    ssize_t n=send(s->fd,data,bytes,MSG_NOSIGNAL);if(n>=0){s->deadline=seconds()+15;return (int)n;}
    if(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK)return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}
static int receive_tls(void *context,unsigned char *data,size_t bytes){
    Tls *s=context;if(cancelled(s))return MBEDTLS_ERR_SSL_TIMEOUT;
    ssize_t n=recv(s->fd,data,bytes,0);if(n>=0){s->deadline=seconds()+15;return (int)n;}
    if(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK)return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}
static bool retry(Tls *s,int code){return code==MBEDTLS_ERR_SSL_WANT_READ?wait_socket(s,POLLIN):code==MBEDTLS_ERR_SSL_WANT_WRITE?wait_socket(s,POLLOUT):false;}
static bool connect_socket(Tls *s,const char *host,unsigned port){
    char service[8];snprintf(service,sizeof service,"%u",port);struct addrinfo hint={.ai_family=AF_INET,.ai_socktype=SOCK_STREAM},*addresses=NULL;
    if(getaddrinfo(host,service,&hint,&addresses))return false;
    bool ok=false;
    for(struct addrinfo *a=addresses;a&&!cancelled(s);a=a->ai_next){
        s->fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);if(s->fd<0)continue;
        int flags=fcntl(s->fd,F_GETFL,0);if(flags<0||fcntl(s->fd,F_SETFL,flags|O_NONBLOCK)<0){close(s->fd);s->fd=-1;continue;}
        int result=connect(s->fd,a->ai_addr,a->ai_addrlen);
        if(!result)ok=true;
        else if((errno==EINPROGRESS||errno==EWOULDBLOCK)&&wait_socket(s,POLLOUT)){int error=0;socklen_t size=sizeof error;ok=!getsockopt(s->fd,SOL_SOCKET,SO_ERROR,&error,&size)&&!error;}
        if(ok)break;
        close(s->fd);s->fd=-1;
    }
    freeaddrinfo(addresses);return ok;
}
static bool connect_tls(Tls *s,const Url *u){
    char proxy_host[256]={0};unsigned proxy_port=0;
#ifndef BLADES_XBOX
    /* Use the managed host's inherited proxy/trust when running desktop tests.
       The console has no proxy or test-origin overrides. */
    const char *proxy=getenv("HTTPS_PROXY");
    bool local=!strcmp(u->host,"localhost")||!strcmp(u->host,"127.0.0.1");
    if(proxy&&*proxy&&!local){if(strncmp(proxy,"http://",7))return false;const char *p=proxy+7,*colon=strrchr(p,':');if(!colon||(size_t)(colon-p)>=sizeof proxy_host)return false;
        memcpy(proxy_host,p,(size_t)(colon-p));char *end;unsigned long port=strtoul(colon+1,&end,10);if(!port||port>65535||(*end&&strcmp(end,"/")))return false;proxy_port=(unsigned)port;}
#endif
    if(!connect_socket(s,proxy_host[0]?proxy_host:u->host,proxy_host[0]?proxy_port:u->port))return false;
    if(proxy_host[0]){
        char request[640],response[8193]={0};int bytes=snprintf(request,sizeof request,"CONNECT %s:%u HTTP/1.1\r\nHost: %s:%u\r\n\r\n",u->host,u->port,u->host,u->port);size_t sent=0,used=0;
        while(sent<(size_t)bytes&&!cancelled(s)){ssize_t n=send(s->fd,request+sent,(size_t)bytes-sent,MSG_NOSIGNAL);if(n>0)sent+=(size_t)n;else if(n<0&&(errno==EAGAIN||errno==EINTR||errno==EWOULDBLOCK)){if(!wait_socket(s,POLLOUT))return false;}else return false;}
        while(used<sizeof response-1&&!cancelled(s)){if(!wait_socket(s,POLLIN))return false;ssize_t n=recv(s->fd,response+used,1,0);if(n<=0)return false;used+=(size_t)n;response[used]=0;if(strstr(response,"\r\n\r\n"))break;}
        if(strncmp(response,"HTTP/1.1 200",12)&&strncmp(response,"HTTP/1.0 200",12))return false;
    }
    if(mbedtls_ctr_drbg_seed(&s->random,mbedtls_entropy_func,&s->entropy,(const unsigned char *)"Blades HTTPS updater",20)||mbedtls_x509_crt_parse(&s->roots,(const unsigned char *)update_ca_roots,sizeof update_ca_roots)<0)return false;
#ifndef BLADES_XBOX
    const char *extra=getenv("SSL_CERT_FILE");if(extra&&*extra&&mbedtls_x509_crt_parse_file(&s->roots,extra)<0)return false;
#endif
    if(mbedtls_ssl_config_defaults(&s->config,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT))return false;
    mbedtls_ssl_conf_authmode(&s->config,MBEDTLS_SSL_VERIFY_REQUIRED);mbedtls_ssl_conf_ca_chain(&s->config,&s->roots,NULL);mbedtls_ssl_conf_rng(&s->config,mbedtls_ctr_drbg_random,&s->random);
    if(mbedtls_ssl_setup(&s->ssl,&s->config)||mbedtls_ssl_set_hostname(&s->ssl,u->host))return false;
    mbedtls_ssl_set_bio(&s->ssl,s,send_tls,receive_tls,NULL);
    int result;while((result=mbedtls_ssl_handshake(&s->ssl))!=0)if(!retry(s,result))return false;
    return mbedtls_ssl_get_verify_result(&s->ssl)==0;
}
static void release(Tls *s){if(s->fd>=0)close(s->fd);mbedtls_ssl_free(&s->ssl);mbedtls_ssl_config_free(&s->config);mbedtls_x509_crt_free(&s->roots);mbedtls_ctr_drbg_free(&s->random);mbedtls_entropy_free(&s->entropy);free(s);}
typedef struct {xf_response *response;UpdateSink sink;void *context;unsigned char data[8192];size_t used;} Sink;
static bool flush(Sink *s){if(!s->used)return true;bool ok=s->sink(s->context,s->data,s->used);s->used=0;return ok;}
static int receive_byte(void *context,unsigned char byte){Sink *s=context;if(s->response->status!=200)return 1;s->data[s->used++]=byte;return s->used==sizeof s->data?flush(s):1;}
static bool location(const char *header,char *out,size_t bytes){
    const char *p=strstr(header,"\r\n");if(!p)return false;p+=2;
    while(*p&&strncmp(p,"\r\n",2)){const char *end=strstr(p,"\r\n");if(!end)return false;
        if(!strncasecmp(p,"Location:",9)){p+=9;while(*p==' '||*p=='\t')p++;size_t n=(size_t)(end-p);if(!n||n>=bytes)return false;memcpy(out,p,n);out[n]=0;return true;}p=end+2;}
    return false;
}
bool update_https(const char *url,uint64_t limit,UpdateSink sink,void *context,UpdateCancelled cancel,void *cancel_context,char *error,size_t error_bytes){
    if(error_bytes)error[0]=0;
    char current[2048];if(strlen(url)>=sizeof current)return false;snprintf(current,sizeof current,"%s",url);bool ok=false;
    for(unsigned redirect=0;redirect<6;redirect++){
        Url u;if(!parse(current,&u)){snprintf(error,error_bytes,"Invalid update HTTPS address.");return false;}
        Tls *s=calloc(1,sizeof *s);xf_response *r=malloc(sizeof *r);if(!s||!r){free(s);free(r);snprintf(error,error_bytes,"Not enough memory for HTTPS.");return false;}
        s->fd=-1;s->cancel=cancel;s->context=cancel_context;s->deadline=seconds()+15;s->total=seconds()+600;
        mbedtls_ssl_init(&s->ssl);mbedtls_ssl_config_init(&s->config);mbedtls_x509_crt_init(&s->roots);mbedtls_ctr_drbg_init(&s->random);mbedtls_entropy_init(&s->entropy);
        Sink output={.response=r,.sink=sink,.context=context};xf_response_stream(r,receive_byte,&output,limit);
        char raw_header[XF_HEADER_LIMIT+1]={0};size_t header_bytes=0;bool raw_complete=false;
        if(!connect_tls(s,&u)){snprintf(error,error_bytes,"HTTPS connection failed. Check network and console date/time.");goto done;}
        char request[2048];int n=snprintf(request,sizeof request,"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Blades/0.50.0\r\nAccept: application/octet-stream,application/json\r\nAccept-Encoding: identity\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n",u.path,u.host);
        if(n<0||(size_t)n>=sizeof request)goto done;
        for(size_t sent=0;sent<(size_t)n;){int count=mbedtls_ssl_write(&s->ssl,(const unsigned char *)request+sent,(size_t)n-sent);if(count>0)sent+=(size_t)count;else if(!retry(s,count))goto done;}
        while(!r->complete&&!r->failed&&!cancelled(s)){
            unsigned char data[8192];int count=mbedtls_ssl_read(&s->ssl,data,sizeof data);
            if(count>0){
                /* The shared framing parser tokenizes its header in place.
                   Keep only a bounded raw copy for redirect Location fields. */
                if(!raw_complete)for(int i=0;i<count;i++){
                    if(header_bytes==XF_HEADER_LIMIT)goto done;
                    raw_header[header_bytes++]=(char)data[i];raw_header[header_bytes]=0;
                    if(header_bytes>=4&&!memcmp(raw_header+header_bytes-4,"\r\n\r\n",4)){raw_complete=true;break;}
                }
                xf_response_feed(r,data,(size_t)count);
            }
            else if(!count||count==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY){xf_response_eof(r);break;}
            else if(!retry(s,count))goto done;
            if(r->headers_done&&r->status>=300&&r->status<400){char target[2048];if(!location(raw_header,target,sizeof target))goto done;
                if(target[0]=='/'&&target[1]!='/'){
                    char authority[272];if(u.port==443)snprintf(authority,sizeof authority,"%s",u.host);else snprintf(authority,sizeof authority,"%s:%u",u.host,u.port);
                    size_t host=strlen(authority),path=strlen(target);if(host+path+9>sizeof current)goto done;
                    memcpy(current,"https://",8);memcpy(current+8,authority,host);memcpy(current+8+host,target,path+1);
                }else snprintf(current,sizeof current,"%s",target);
                release(s);free(r);s=NULL;r=NULL;break;}
        }
        if(!s)continue;
        ok=r->complete&&!r->failed&&r->status==200&&!cancelled(s)&&flush(&output);
        if(!ok)snprintf(error,error_bytes,r->status==404?"No published update was found on GitHub.":"Update download was interrupted or invalid (HTTP %d).",r->status);
done:
        if(!ok&&(!error_bytes||!error[0]))snprintf(error,error_bytes,"HTTPS transfer failed or timed out.");
        release(s);free(r);return ok;
    }
    snprintf(error,error_bytes,"Too many update redirects.");return false;
}
void update_sha256(const unsigned char *data,size_t bytes,unsigned char out[32]){(void)mbedtls_sha256(data,bytes,out,0);}
