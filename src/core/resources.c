#include "resources.h"
#include "draw_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { PACK_MAX_BYTES=16*1024*1024, PACK_MAX_ENTRIES=512, NAME_BYTES=160 };
typedef struct {
    char name[NAME_BYTES];
    uint32_t offset,size;
    unsigned char *rgba;
    int width,height;
    bool tried;
    unsigned image_id;
    struct {unsigned char *rgba;int width,height;unsigned image_id;} scaled[3];
    unsigned next_scaled;
} Entry;
static unsigned generation=1;
unsigned resources_generation(void) {return generation;}
static struct {unsigned char *data;size_t size;Entry *entries;unsigned count;} pack;
static uint32_t be32(const unsigned char *p) {
    return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
void resources_close(void) {
    generation++;
    for(unsigned i=0;i<pack.count;i++) {
        resource_png_free(pack.entries[i].rgba);
        for(unsigned j=0;j<3;j++)free(pack.entries[i].scaled[j].rgba);
    }
    free(pack.entries);free(pack.data);memset(&pack,0,sizeof pack);
}
/* Ownership transfers only on success. Every table/payload range is checked
   before a resource can reach the PNG decoder. Nothing is extracted to disk. */
static bool adopt(unsigned char *bytes,size_t size) {
    if(size<22||size>PACK_MAX_BYTES||memcmp(bytes,"XUIZ",4)||be32(bytes+4)!=1||be32(bytes+8)!=size||be32(bytes+12)!=0)return false;
    size_t table=be32(bytes+16),base=22+table,cursor=22;
    unsigned count=(unsigned)bytes[20]<<8|bytes[21];
    if(table>size-22||count==0||count>PACK_MAX_ENTRIES)return false;
    Entry *entries=calloc(count,sizeof *entries);
    if(!entries)return false;
    bool valid=true;
    for(unsigned i=0;i<count&&valid;i++) {
        if(cursor>base||base-cursor<9){valid=false;break;}
        uint32_t length=be32(bytes+cursor),relative=be32(bytes+cursor+4);
        unsigned chars=bytes[cursor+8];cursor+=9;
        if(chars==0||chars>=NAME_BYTES||chars*2>base-cursor||relative>size-base||length>size-base-relative){valid=false;break;}
        Entry *entry=&entries[i];
        for(unsigned j=0;j<chars;j++) {
            /* This v1 pack uses ASCII names stored as UTF-16BE. */
            unsigned high=bytes[cursor+j*2],low=bytes[cursor+j*2+1];
            if(high||(!low&&j+1<chars)||low>=128){valid=false;break;}
            entry->name[j]=(char)low;
        }
        cursor+=chars*2;
        entry->offset=(uint32_t)(base+relative);entry->size=length;
        if(!entry->name[0])valid=false;
        for(unsigned j=0;j<i;j++)if(!strcmp(entry->name,entries[j].name))valid=false;
    }
    if(cursor!=base)valid=false;
    if(!valid){free(entries);return false;}
    resources_close();pack.data=bytes;pack.size=size;pack.entries=entries;pack.count=count;
    fprintf(stderr,"resources: loaded local XUIZ archive (%u entries)\n",count);
    return true;
}
bool resources_open_memory(const unsigned char *bytes,size_t size) {
    if(!bytes||size<22||size>PACK_MAX_BYTES)return false;
    unsigned char *owned=malloc(size);if(!owned)return false;
    memcpy(owned,bytes,size);
    if(adopt(owned,size))return true;
    free(owned);return false;
}
bool resources_open(const char *path) {
    FILE *file=fopen(path,"rb");
    if(!file){fprintf(stderr,"resources: optional local archive unavailable; embedded artwork active\n");return false;}
    if(fseek(file,0,SEEK_END)!=0){fclose(file);return false;}
    long length=ftell(file);
    if(length<22||length>PACK_MAX_BYTES||fseek(file,0,SEEK_SET)!=0){fclose(file);return false;}
    unsigned char *bytes=malloc((size_t)length);
    if(!bytes){fclose(file);return false;}
    size_t read=fread(bytes,1,(size_t)length,file);fclose(file);
    if(read==(size_t)length&&adopt(bytes,read))return true;
    free(bytes);fprintf(stderr,"resources: invalid local archive; embedded artwork active\n");return false;
}
const unsigned char *resources_blob(const char *name,size_t *size) {
    if(size)*size=0;
    for(unsigned i=0;i<pack.count;i++)if(!strcmp(pack.entries[i].name,name)) {
        if(size)*size=pack.entries[i].size;
        return pack.data+pack.entries[i].offset;
    }
    return NULL;
}
/* Alpha-aware box filtering for downscaled logos/icons, cached after first use.
   RGB is accumulated premultiplied to keep transparent texels from making halos. */
static unsigned char *scaled_image(const unsigned char *rgba,int iw,int ih,int w,int h) {
    unsigned char *out=malloc((size_t)w*h*4);if(!out)return NULL;
    for(int y=0;y<h;y++)for(int x=0;x<w;x++) {
        int x0=x*iw/w,x1=((x+1)*iw+w-1)/w;
        int y0=y*ih/h,y1=((y+1)*ih+h-1)/h;
        uint64_t sum[4]={0};unsigned count=0;
        for(int sy=y0;sy<y1;sy++)for(int sx=x0;sx<x1;sx++) {
            const unsigned char *p=rgba+(sy*iw+sx)*4;
            for(int k=0;k<3;k++)sum[k]+=(unsigned)p[k]*p[3];
            sum[3]+=p[3];count++;
        }
        unsigned char *dst=out+(y*w+x)*4;
        for(int k=0;k<3;k++)dst[k]=sum[3]?(unsigned char)(sum[k]/sum[3]):0;
        dst[3]=count?(unsigned char)(sum[3]/count):0;
    }
    return out;
}
static Entry *image_entry(const char *name) {
    for(unsigned i=0;i<pack.count;i++) {
        Entry *entry=&pack.entries[i];
        if(strcmp(entry->name,name))continue;
        if(!entry->tried) {
            entry->tried=true;
            entry->rgba=resource_png_decode(pack.data+entry->offset,(int)entry->size,&entry->width,&entry->height);
            if(entry->rgba)entry->image_id=canvas_image_id();
            if(!entry->rgba)fprintf(stderr,"resources: %s is not a supported PNG; using fallback\n",entry->name);
        }
        return entry->rgba?entry:NULL;
    }
    return NULL;
}
bool resources_draw_fit(Canvas *c,const char *name,int x,int y,int w,int h) {
    if(w<=0||h<=0||w>WIDTH*2||h>HEIGHT*2)return false;
    Entry *entry=image_entry(name);if(!entry)return false;
    int dw=w,dh=h;
    if(w*entry->height>h*entry->width)dw=h*entry->width/entry->height;
    else dh=w*entry->height/entry->width;
    if(dw<1)dw=1;
    if(dh<1)dh=1;
    return resources_draw(c,name,x+(w-dw)/2,y+(h-dh)/2,dw,dh);
}
bool resources_draw_opacity(Canvas *c,const char *name,int x,int y,int w,int h,unsigned opacity) {
    if(opacity>=255)return resources_draw(c,name,x,y,w,h);
    if(w<=0||h<=0||w>WIDTH*2||h>HEIGHT*2)return false;
    Entry *entry=image_entry(name);if(!entry)return false;
    canvas_image_tagged_opacity(c,(DrawImage){entry->rgba,entry->image_id,0,entry->width,entry->height,IMAGE_RGBA},x,y,w,h,opacity);
    return true;
}
bool resources_draw(Canvas *c,const char *name,int x,int y,int w,int h) {
    if(w<=0||h<=0||w>WIDTH*2||h>HEIGHT*2)return false;
    Entry *entry=image_entry(name);
    if(entry) {
        /* Static artwork retains its alpha-aware box filter. Only animated
           skin caps bypass CPU resampling when their height changes. */
        const char *slot=strrchr(name,'/');
        bool moving_cap=c->ops&&!strncmp(name,"skin/",5)&&slot&&(!strcmp(slot,"/7.png")||!strcmp(slot,"/8.png"));
        if(!moving_cap&&w<=entry->width&&h<=entry->height&&(w!=entry->width||h!=entry->height)) {
            for(unsigned j=0;j<3;j++)if(entry->scaled[j].rgba&&entry->scaled[j].width==w&&entry->scaled[j].height==h) {
                canvas_image_tagged(c,(DrawImage){entry->scaled[j].rgba,entry->scaled[j].image_id,0,w,h,IMAGE_RGBA},x,y,w,h);return true;
            }
            unsigned j=entry->next_scaled++%3;
            free(entry->scaled[j].rgba);
            entry->scaled[j].rgba=scaled_image(entry->rgba,entry->width,entry->height,w,h);
            entry->scaled[j].image_id=canvas_image_id();
            entry->scaled[j].width=w;entry->scaled[j].height=h;
            if(entry->scaled[j].rgba){canvas_image_tagged(c,(DrawImage){entry->scaled[j].rgba,entry->scaled[j].image_id,0,w,h,IMAGE_RGBA},x,y,w,h);return true;}
        }
        canvas_image_tagged(c,(DrawImage){entry->rgba,entry->image_id,0,entry->width,entry->height,IMAGE_RGBA},x,y,w,h);
        return true;
    }
    return false;
}
