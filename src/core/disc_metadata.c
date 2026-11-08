#include "disc_metadata.h"
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static bool span(size_t size,size_t off,size_t len){return off<=size&&len<=size-off;}
static bool read_at(FILE *f,size_t off,void *out,size_t n){return off<=0x7fffffff&&fseek(f,(long)off,SEEK_SET)==0&&fread(out,1,n,f)==n;}
static bool string_table(const uint8_t *p,size_t n,char *title) {
    if(n<14||memcmp(p,"XSTR",4)||be32(p+4)!=1||be32(p+8)>n)return false;
    size_t at=14;
    for(unsigned i=0;i<be16(p+12);i++) {
        if(!span(n,at,4))return false;
        unsigned id=be16(p+at),len=be16(p+at+2);at+=4;
        if(!span(n,at,len))return false;
        if(id==0x8000) {
            char s[DISC_TITLE_BYTES];size_t copy=len<sizeof s-1?len:sizeof s-1;
            memcpy(s,p+at,copy);s[copy]=0;disc_title_copy(title,DISC_TITLE_BYTES,s);return title[0]!=0;
        }
        at+=len;
    }
    return false;
}
bool disc_xdbf_title(const uint8_t *p,size_t n,char title[DISC_TITLE_BYTES]) {
    title[0]=0;if(n<24||memcmp(p,"XDBF",4)||(be32(p+4)!=1&&be32(p+4)!=0x10000))return false;
    size_t count=be32(p+8),used=be32(p+12),free_count=be32(p+16);
    if(used>count||count>(n-24)/18||free_count>(n-24-count*18)/8)return false;
    size_t base=24+count*18+free_count*8;unsigned preferred=1;
    for(size_t i=0;i<used;i++) {
        const uint8_t *e=p+24+i*18;uint32_t off=be32(e+10),len=be32(e+14);
        if(!span(n-base,off,len))return false;
        if(be16(e)==1&&be32(e+2)==0&&be32(e+6)==0x58535443&&len>=16&&
           !memcmp(p+base+off,"XSTC",4))preferred=be32(p+base+off+12);
    }
    for(unsigned pass=0;pass<3;pass++)for(size_t i=0;i<used;i++) {
        const uint8_t *e=p+24+i*18;unsigned language=be32(e+6);
        if(be16(e)!=3||be32(e+2)||(pass==0&&language!=1)||(pass==1&&language!=preferred))continue;
        if(string_table(p+base+be32(e+10),be32(e+14),title))return true;
    }
    return false;
}
static bool utf16_title(const uint8_t *p,size_t count,char *title) {
    size_t at=0;
    for(size_t i=0;i<count;i++) {
        uint32_t cp=be16(p+i*2);if(!cp)break;
        if(cp>=0xd800&&cp<=0xdbff) {
            if(i+1>=count)return false;
            uint32_t low=be16(p+(++i)*2);if(low<0xdc00||low>0xdfff)return false;
            cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);
        } else if(cp>=0xdc00&&cp<=0xdfff)return false;
        unsigned bytes=cp<0x80?1:cp<0x800?2:cp<0x10000?3:4;
        if(at+bytes>=DISC_TITLE_BYTES)break;
        if(bytes==1)title[at++]=(char)(cp<32?' ':cp);
        else {
            title[at++]=(char)((bytes==2?0xc0:bytes==3?0xe0:0xf0)|(cp>>(6*(bytes-1))));
            for(unsigned b=bytes-1;b;b--)title[at++]=(char)(0x80|((cp>>(6*(b-1)))&63));
        }
    }
    title[at]=0;return at>0;
}
bool disc_stfs_title(FILE *f,char title[DISC_TITLE_BYTES]) {
    uint8_t magic[4],name[128];title[0]=0;
    return read_at(f,0,magic,4)&&(!memcmp(magic,"LIVE",4)||!memcmp(magic,"PIRS",4)||!memcmp(magic,"CON ",4))&&
        read_at(f,0x1691,name,sizeof name)&&utf16_title(name,64,title);
}
typedef struct {uint8_t *data;size_t start,size,pos;} Resource;
static bool emit(Resource *r,const uint8_t *p,size_t n) {
    if(n>128u*1024*1024-r->pos)return false;
    size_t lo=r->pos>r->start?r->pos:r->start,hi=r->pos+n<r->start+r->size?r->pos+n:r->start+r->size;
    if(hi>lo) {if(p)memcpy(r->data+lo-r->start,p+lo-r->pos,hi-lo);else memset(r->data+lo-r->start,0,hi-lo);}
    r->pos+=n;return true;
}
static bool decode(FILE *f,const uint8_t *h,size_t hn,size_t format,Resource *r,DiscCodec *c) {
    if(!span(hn,format,8))return false;
    size_t fn=be32(h+format);if(fn<8||!span(hn,format,fn))return false;
    unsigned encrypted=be16(h+format+4),kind=be16(h+format+6);
    if(encrypted>1||kind>2)return false;
    uint32_t window=kind==2&&fn>=36?be32(h+format+8):0;
    if(kind==2&&(!window||(window&(window-1))||window<32768||window>32u*1024*1024))return false;
    size_t sec=be32(h+16),image_size=be32(h+sec+4);bool started=false,ok=false;
    if(encrypted||kind==2) {
        if(!c||!c->start||!c->finish||!c->start(c->context,h+sec+0x150,encrypted!=0,window))return false;
        started=true;
        if((encrypted&&!c->decrypt)||(kind==2&&!c->expand))goto done;
    }
    uint8_t *chunk=malloc(32768);if(!chunk)goto done;
    size_t input=hn;
    if(kind<2) {
        size_t blocks=kind==1?(fn-8)/8:1;
        if(kind==1&&(fn-8)%8)goto free_chunk;
        for(size_t i=0;i<blocks&&r->pos<r->start+r->size;i++) {
            size_t data=kind==1?be32(h+format+8+i*8):be32(h+sec+4);
            size_t zeros=kind==1?be32(h+format+12+i*8):0;
            if(!span(image_size,r->pos,data)||!span(image_size,r->pos+data,zeros)||(encrypted&&(data&15)))goto free_chunk;
            while(data&&r->pos<r->start+r->size) {
                size_t n=data<32768?data:32768;
                if(!read_at(f,input,chunk,n)||(encrypted&&!c->decrypt(c->context,chunk,n))||!emit(r,chunk,n))goto free_chunk;
                input+=n;data-=n;
            }
            if(r->pos<r->start+r->size&&!emit(r,NULL,zeros))goto free_chunk;
        }
    } else {
        size_t block=be32(h+format+12);unsigned blocks=0;
        while(r->pos<r->start+r->size&&block) {
            if(++blocks>4096||block<26||block>2u*1024*1024||(encrypted&&(block&15)))goto free_chunk;
            uint8_t *b=malloc(block);if(!b)goto free_chunk;
            if(!read_at(f,input,b,block)||(encrypted&&!c->decrypt(c->context,b,block))){free(b);goto free_chunk;}
            input+=block;size_t next=be32(b),at=24;
            while(r->pos<r->start+r->size) {
                if(!span(block,at,2)){free(b);goto free_chunk;}
                size_t n=be16(b+at);at+=2;if(!n)break;
                if(!span(block,at,n)){free(b);goto free_chunk;}
                if(r->pos>=image_size){free(b);goto free_chunk;}
                size_t expanded=image_size-r->pos;if(expanded>32768)expanded=32768;
                if(!c->expand(c->context,b+at,n,chunk,&expanded)||!expanded||expanded>32768||!emit(r,chunk,expanded)){free(b);goto free_chunk;}
                at+=n;
            }
            free(b);block=next;
        }
    }
    ok=r->pos>=r->start+r->size;
free_chunk:free(chunk);
done:if(started)c->finish(c->context);return ok;
}
bool disc_xex_title(FILE *f,DiscCodec *c,uint32_t *id,char title[DISC_TITLE_BYTES]) {
    bool want_title=title!=NULL;char ignored[DISC_TITLE_BYTES];if(!title)title=ignored;
    uint8_t first[24];title[0]=0;*id=0;
    if(!read_at(f,0,first,24)||memcmp(first,"XEX2",4))return false;
    size_t hn=be32(first+8),count=be32(first+20),sec=be32(first+16);
    if(hn<24||hn>1024*1024||count>(hn-24)/8||!span(hn,sec,0x180))return false;
    uint8_t *h=malloc(hn);if(!h)return false;
    if(!read_at(f,0,h,hn)){free(h);return false;}
    size_t resources=0,format=0;bool execution=false;
    for(size_t i=0;i<count;i++) {
        uint32_t key=be32(h+24+i*8),v=be32(h+28+i*8);
        if(key==0x40006&&span(hn,v,24)){*id=be32(h+v+12);execution=true;}
        if(key==0x2ff)resources=v;
        if(key==0x3ff)format=v;
    }
    if(!execution){free(h);return false;}
    if(want_title&&resources&&span(hn,resources,4)) {
        size_t rn=be32(h+resources);char name[9];snprintf(name,sizeof name,"%08X",*id);
        if(rn>=4&&(rn-4)%16==0&&span(hn,resources,rn))for(size_t at=resources+4;at<resources+rn;at+=16) {
            if(memcmp(h+at,name,8))continue;
            uint32_t base=be32(h+sec+0x110),addr=be32(h+at+8),size=be32(h+at+12),image=be32(h+sec+4);
            if(addr<base||size>8u*1024*1024||image>128u*1024*1024||!span(image,addr-base,size))break;
            Resource r={.data=calloc(size?size:1,1),.start=addr-base,.size=size};
            if(r.data&&format&&decode(f,h,hn,format,&r,c))disc_xdbf_title(r.data,r.size,title);
            free(r.data);break;
        }
    }
    free(h);return true;
}
