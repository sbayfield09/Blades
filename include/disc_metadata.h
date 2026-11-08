#ifndef BLADES_DISC_METADATA_H
#define BLADES_DISC_METADATA_H
#include "disc.h"
#include <stdio.h>
/* Stateful CBC/LZX services supplied by the console, with no image loading or
   execution. The reader itself is shared with desktop malformed-file tests. */
typedef struct {
    void *context;
    bool (*start)(void *,const uint8_t key[16],bool encrypted,uint32_t window);
    bool (*decrypt)(void *,uint8_t *,size_t);
    bool (*expand)(void *,const uint8_t *,size_t,uint8_t *,size_t *);
    void (*finish)(void *);
} DiscCodec;
bool disc_xdbf_title(const uint8_t *data,size_t bytes,char title[DISC_TITLE_BYTES]);
bool disc_stfs_title(FILE *file,char title[DISC_TITLE_BYTES]);
/* True means a structurally readable XEX header, even if title metadata is
   absent. In that case title is empty and the UI uses Play Xbox 360 Game. */
bool disc_xex_title(FILE *file,DiscCodec *codec,uint32_t *title_id,char title[DISC_TITLE_BYTES]);
#endif
