/* PNG-only configuration of the pinned, MIT/public-domain stb decoder. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 1024
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma GCC diagnostic pop
#include "resources.h"
unsigned char *resource_png_decode(const unsigned char *bytes,int size,int *w,int *h) {
    int channels;
    return stbi_load_from_memory(bytes,size,w,h,&channels,4);
}
void resource_png_free(void *pixels) {stbi_image_free(pixels);}
unsigned char *resource_png_decode_bounded(const unsigned char *bytes,int size,int *w,int *h,int max_w,int max_h) {
    int channels;
    if(!stbi_info_from_memory(bytes,size,w,h,&channels)||*w<1||*h<1||*w>max_w||*h>max_h)return NULL;
    return resource_png_decode(bytes,size,w,h);
}
