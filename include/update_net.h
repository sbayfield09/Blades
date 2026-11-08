#ifndef BLADES_UPDATE_NET_H
#define BLADES_UPDATE_NET_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef bool (*UpdateCancelled)(void *);
typedef bool (*UpdateSink)(void *,const unsigned char *,size_t);
/* HTTPS only, peer/hostname verification, bounded redirects and body.
   No credentials. Allowed release/CDN hosts are fixed in the implementation. */
bool update_https(const char *url,uint64_t limit,UpdateSink,void *,UpdateCancelled,void *,char *error,size_t);
void update_sha256(const unsigned char *,size_t,unsigned char digest[32]);
#endif
