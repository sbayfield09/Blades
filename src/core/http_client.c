/* BSD socket adapter for XenonFin's bounded HTTP parser. */
#include "http_client.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
enum { HTTP_RING=65536,HTTP_RECEIVE=16384,HTTP_CONTROL_IDLE=8,HTTP_MEDIA_IDLE=30 };
struct HttpStream {
    int socket;xf_response response;
    HttpCancelled cancel;void *context;bool media;
    unsigned char ring[HTTP_RING];size_t head,count;
    double deadline,total_deadline;char error[192];
};
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static double idle_limit(const HttpStream *s){return s->media?HTTP_MEDIA_IDLE:HTTP_CONTROL_IDLE;}
static bool cancelled(HttpStream *s){return s->cancel&&s->cancel(s->context);}
static bool fail(HttpStream *s,const char *message){snprintf(s->error,sizeof s->error,"%s",message);return false;}
static bool wait_for(HttpStream *s,short events){
    while(!cancelled(s)){
        if(now()>s->deadline||(!s->media&&now()>s->total_deadline))return fail(s,s->media&&s->response.headers_done?"The server stopped sending media for 30 seconds.":"The server timed out.");
        struct pollfd p={.fd=s->socket,.events=events};int n=poll(&p,1,100);
        if(n>0){if(p.revents&(events|POLLHUP))return true;if(p.revents&(POLLERR|POLLNVAL))return fail(s,"The server connection failed.");}
        if(n<0&&errno!=EINTR)return fail(s,"Could not wait for the server.");
    }
    return fail(s,"Request cancelled.");
}
static bool send_request(HttpStream *s,const char *request){
    size_t left=strlen(request);if(!left||left>=XF_REQUEST_LIMIT)return fail(s,"The request is too large.");
    while(left&&!cancelled(s)){
        ssize_t n=send(s->socket,request,left,MSG_NOSIGNAL);
        if(n>0){left-=(size_t)n;request+=n;s->deadline=now()+HTTP_CONTROL_IDLE;}
        else if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)){if(!wait_for(s,POLLOUT))return false;}
        else return fail(s,"Could not send the request.");
    }
    return !left;
}
static int sink(void *context,unsigned char byte){
    HttpStream *s=context;if(s->count==HTTP_RING)return 0;
    s->ring[(s->head+s->count)%HTTP_RING]=byte;s->count++;return 1;
}
static bool receive(HttpStream *s){
    unsigned char data[HTTP_RECEIVE];
    while(!cancelled(s)){
        if(!s->media&&now()>s->total_deadline)return fail(s,"The server timed out.");
        ssize_t n=recv(s->socket,data,sizeof data,0);
        if(n>0){s->deadline=now()+idle_limit(s);xf_response_feed(&s->response,data,(size_t)n);
            if(s->response.failed)return fail(s,s->response.error);
            return true;}
        if(!n){xf_response_eof(&s->response);return !s->response.failed||fail(s,s->response.error);}
        if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR){if(!wait_for(s,POLLIN))return false;}
        else return fail(s,"The server connection was lost.");
    }
    return fail(s,"Request cancelled.");
}
static HttpStream *stream_open(const xf_endpoint *ep,const char *request,bool media,bool image,HttpCancelled cancel,void *context,char *error,size_t cap,int *status){
    HttpStream *s=calloc(1,sizeof *s);if(status)*status=0;
    if(!s){snprintf(error,cap,"Not enough memory for HTTP.");return NULL;}
    s->socket=-1;s->cancel=cancel;s->context=context;s->media=media;s->deadline=now()+HTTP_CONTROL_IDLE;s->total_deadline=now()+20;
    if(media||image)xf_response_stream(&s->response,sink,s,image?262144:UINT64_MAX);else xf_response_init(&s->response);
    s->socket=socket(AF_INET,SOCK_STREAM,0);if(s->socket<0){fail(s,"The network is unavailable.");goto failure;}
    int flags=fcntl(s->socket,F_GETFL,0);if(flags<0||fcntl(s->socket,F_SETFL,flags|O_NONBLOCK)<0){fail(s,"Could not configure the network socket.");goto failure;}
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(ep->port)};
    if(inet_pton(AF_INET,ep->host,&address.sin_addr)!=1){fail(s,"Enter a valid server IPv4 address.");goto failure;}
    if(connect(s->socket,(struct sockaddr *)&address,sizeof address)<0){
        if(errno!=EINPROGRESS&&errno!=EWOULDBLOCK&&errno!=EAGAIN){fail(s,"Could not connect to the server.");goto failure;}
        if(!wait_for(s,POLLOUT))goto failure;
        int result=0;socklen_t bytes=sizeof result;
        if(getsockopt(s->socket,SOL_SOCKET,SO_ERROR,&result,&bytes)||result){fail(s,"Could not connect to the server.");goto failure;}
    }
    if(!send_request(s,request))goto failure;
    s->deadline=now()+idle_limit(s);
    while(!s->response.headers_done&&!s->response.complete)if(!receive(s))goto failure;
    if(status)*status=s->response.status;
    if(media&&s->response.status!=200){snprintf(s->error,sizeof s->error,"Jellyfin stream returned HTTP %d.",s->response.status);goto failure;}
    if(media&&s->response.content_type[0]&&strncmp(s->response.content_type,"video/",6)&&strncmp(s->response.content_type,"application/octet-stream",24)){
        fail(s,"Jellyfin returned an unsupported video response.");goto failure;
    }
    if(image&&(s->response.status!=200||strcmp(s->response.content_type,"image/png"))){fail(s,"Artwork unavailable.");goto failure;}
    return s;
failure:
    if(status)*status=s->response.status;
    snprintf(error,cap,"%s",s->error[0]?s->error:"The HTTP request failed.");http_stream_close(s);return NULL;
}
HttpStream *http_stream_open(const xf_endpoint *ep,const char *request,bool media,HttpCancelled cancel,void *context,char *error,size_t cap,int *status){
    return stream_open(ep,request,media,false,cancel,context,error,cap,status);
}
unsigned char *http_image_request(const xf_endpoint *ep,const char *request,size_t *size,HttpCancelled cancel,void *context){
    char error[192];*size=0;HttpStream *s=stream_open(ep,request,false,true,cancel,context,error,sizeof error,NULL);if(!s)return NULL;
    unsigned char *data=malloc(262144);bool ok=data!=NULL;
    while(ok){unsigned char buffer[8192];int n=http_stream_read(s,buffer,sizeof buffer);if(n<0){ok=false;break;}if(!n)break;
        if(*size+(size_t)n>262144){ok=false;break;}memcpy(data+*size,buffer,(size_t)n);*size+=(size_t)n;}
    http_stream_close(s);if(!ok||!*size){free(data);*size=0;return NULL;}return data;
}
int http_stream_read(HttpStream *s,unsigned char *out,int bytes){
    if(!s||bytes<=0)return 0;
    if(cancelled(s))return -1;
    s->deadline=now()+idle_limit(s); /* Paused/backpressured playback is not a network timeout. */
    while(!s->count&&!s->response.complete)if(!receive(s))return -1;
    size_t n=s->count;if(n>(unsigned)bytes)n=(unsigned)bytes;
    size_t first=n;if(first>HTTP_RING-s->head)first=HTTP_RING-s->head;
    memcpy(out,s->ring+s->head,first);memcpy(out+first,s->ring,n-first);
    s->head=(s->head+n)%HTTP_RING;s->count-=n;return (int)n;
}
const char *http_stream_error(const HttpStream *s){return s?s->error:"The HTTP stream is unavailable.";}
void http_stream_close(HttpStream *s){if(s){if(s->socket>=0)close(s->socket);xf_wipe(s,sizeof *s);free(s);}}
bool http_request(const xf_endpoint *ep,const char *request,xf_response *response,HttpCancelled cancel,void *context,char *error,size_t cap){
    HttpStream *s=http_stream_open(ep,request,false,cancel,context,error,cap,NULL);if(!s){xf_response_init(response);return false;}
    bool ok=true;while(!s->response.complete)if(!receive(s)){ok=false;break;}
    *response=s->response;if(!ok)snprintf(error,cap,"%s",s->error);else if(cap)*error=0;
    http_stream_close(s);return ok;
}
