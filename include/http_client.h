#ifndef BLADES_HTTP_CLIENT_H
#define BLADES_HTTP_CLIENT_H
#include "protocol.h"
#include <stdbool.h>
#include <stddef.h>
typedef bool (*HttpCancelled)(void *);
typedef struct HttpStream HttpStream;
/* Numeric IPv4 HTTP, matching XenonFin's native transport. Requests and
 * responses are bounded. No redirects, credential forwarding or URL logging. */
HttpStream *http_stream_open(const xf_endpoint *,const char *request,bool media,
    HttpCancelled,void *,char *error,size_t error_bytes,int *status);
int http_stream_read(HttpStream *,unsigned char *,int bytes);
void http_stream_close(HttpStream *);
bool http_request(const xf_endpoint *,const char *request,xf_response *,
    HttpCancelled,void *,char *error,size_t error_bytes);
const char *http_stream_error(const HttpStream *);
/* Authenticated PNG response, bounded to 256 KiB and control deadlines. */
unsigned char *http_image_request(const xf_endpoint *,const char *,size_t *,HttpCancelled,void *);
#endif
