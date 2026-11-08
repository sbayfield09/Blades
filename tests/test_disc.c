#include "blades.h"
#include "disc_metadata.h"
#include "draw_list.h"
#include "resources.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static void put16(uint8_t *p,unsigned n){p[0]=n>>8;p[1]=n;}
static void put32(uint8_t *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static FILE *file(const uint8_t *p,size_t n){FILE *f=tmpfile();assert(f&&fwrite(p,1,n,f)==n);rewind(f);return f;}
static size_t xdbf(uint8_t p[256]) {
    memset(p,0,256);memcpy(p,"XDBF",4);put32(p+4,0x10000);put32(p+8,2);put32(p+12,1);put32(p+16,1);
    put16(p+24,3);put32(p+30,1);put32(p+34,0);put32(p+38,24);
    uint8_t *s=p+24+36+8;memcpy(s,"XSTR",4);put32(s+4,1);put32(s+8,24);put16(s+12,1);
    put16(s+14,0x8000);put16(s+16,6);memcpy(s+18,"Halo 3",6);return 92;
}
typedef struct {unsigned starts,decrypts,expands,finishes;bool encrypted,fail;} FakeCodec;
static bool start(void *p,const uint8_t key[16],bool encrypted,uint32_t window) {
    FakeCodec *c=p;c->starts++;c->encrypted=encrypted;assert(key[0]==0x71);assert(!window||window==32768);return true;
}
static bool decrypt(void *p,uint8_t *data,size_t n) {
    FakeCodec *c=p;c->decrypts++;assert(c->encrypted&&!(n&15));
    for(size_t i=0;i<n;i++)data[i]^=0xa5;return !c->fail;
}
static bool expand(void *p,const uint8_t *data,size_t n,uint8_t *out,size_t *count) {
    FakeCodec *c=p;c->expands++;assert(n<=*count);memcpy(out,data,n);*count=n;return !c->fail;
}
static void finish(void *p){((FakeCodec *)p)->finishes++;}
static size_t xex(uint8_t p[2048],unsigned kind,bool encrypted) {
    memset(p,0,2048);uint8_t db[256];size_t n=xdbf(db);
    memcpy(p,"XEX2",4);put32(p+8,0x400);put32(p+16,0x100);put32(p+20,3);
    put32(p+24,0x40006);put32(p+28,0x80);put32(p+32,0x2ff);put32(p+36,0xa0);
    put32(p+40,0x3ff);put32(p+44,0x40);put32(p+0x8c,0x4d5307e6);
    put32(p+0xa0,20);memcpy(p+0xa4,"4D5307E6",8);put32(p+0xac,0x82000040);put32(p+0xb0,n);
    put32(p+0x104,256);put32(p+0x210,0x82000000);p[0x250]=0x71;
    put16(p+0x44,encrypted);put16(p+0x46,kind);
    uint8_t image[256]={0};memcpy(image+64,db,n);size_t bytes=0x400;
    if(kind==0){put32(p+0x40,8);memcpy(p+bytes,image,256);bytes+=256;}
    else if(kind==1) {
        put32(p+0x40,24);put32(p+0x48,32);put32(p+0x4c,32);put32(p+0x50,192);
        memcpy(p+bytes,image,32);memcpy(p+bytes+32,image+64,192);bytes+=224;
    } else {
        put32(p+0x40,36);put32(p+0x48,32768);put32(p+0x4c,288);
        put16(p+bytes+24,256);memcpy(p+bytes+26,image,256);bytes+=288;
    }
    if(encrypted)for(size_t i=0x400;i<bytes;i++)p[i]^=0xa5;
    return bytes;
}
static void metadata_tests(void) {
    uint8_t db[256],p[2048];char title[DISC_TITLE_BYTES];size_t n=xdbf(db);
    assert(disc_xdbf_title(db,n,title)&&!strcmp(title,"Halo 3"));
    for(size_t i=0;i<n;i++)assert(!disc_xdbf_title(db,i,title));
    put32(db+34,0xffffffff);assert(!disc_xdbf_title(db,n,title));
    xdbf(db);put32(db+8,UINT32_MAX);assert(!disc_xdbf_title(db,n,title));
    xdbf(db);put16(db+68+16,0xffff);assert(!disc_xdbf_title(db,n,title));
    for(unsigned kind=0;kind<=2;kind++)for(unsigned encrypted=0;encrypted<=1;encrypted++) {
        size_t bytes=xex(p,kind,encrypted);FakeCodec fake={0};DiscCodec codec={&fake,start,decrypt,expand,finish};
        FILE *f=file(p,bytes);uint32_t id;
        assert(disc_xex_title(f,&codec,&id,title)&&id==0x4d5307e6&&!strcmp(title,"Halo 3"));
        assert(fake.starts==(encrypted||kind==2)&&fake.finishes==fake.starts);
        assert((fake.decrypts>0)==(encrypted!=0));assert((fake.expands>0)==(kind==2));fclose(f);
        f=file(p,bytes);assert(disc_xex_title(f,NULL,&id,NULL)&&id==0x4d5307e6);fclose(f);
        f=file(p,bytes-128);assert(disc_xex_title(f,&codec,&id,title)&&!title[0]);fclose(f);
    }
    n=xex(p,2,true);FakeCodec bad={.fail=true};DiscCodec codec={&bad,start,decrypt,expand,finish};
    FILE *f=file(p,n);uint32_t id;assert(disc_xex_title(f,&codec,&id,title)&&!title[0]);
    assert(bad.starts==1&&bad.finishes==1);fclose(f);
    n=xex(p,0,false);put32(p+0xb0,UINT32_MAX);f=file(p,n);assert(disc_xex_title(f,NULL,&id,title)&&!title[0]);fclose(f);
    n=xex(p,0,false);put32(p+8,UINT32_MAX);f=file(p,n);assert(!disc_xex_title(f,NULL,&id,title));fclose(f);
    /* Mutate each byte independently, including counts and offsets, so the
       sanitizer exercises structurally plausible corruptions as well. */
    n=xex(p,0,false);
    for(size_t at=0;at<n;at++) {
        uint8_t old=p[at];p[at]^=0xff;f=file(p,n);
        (void)disc_xex_title(f,NULL,&id,title);assert(memchr(title,0,sizeof title));fclose(f);p[at]=old;
    }
    uint8_t art[0x1711]={0};memcpy(art,"LIVE",4);
    const char *name="Forza Motorsport 2";for(size_t i=0;i<strlen(name);i++)put16(art+0x1691+i*2,name[i]);
    f=file(art,sizeof art);assert(disc_stfs_title(f,title)&&!strcmp(title,name));fclose(f);
    f=file(art,sizeof art-1);assert(!disc_stfs_title(f,title));fclose(f);
    put16(art+0x1691,0xd800);put16(art+0x1693,'x');f=file(art,sizeof art);assert(!disc_stfs_title(f,title));fclose(f);
    disc_title_copy(title,sizeof title,"Halo\n3\t");assert(!strcmp(title,"Halo 3"));
    char short_title[5];disc_title_copy(short_title,sizeof short_title,"abc\xe2\x84\xa2");assert(!strcmp(short_title,"abc"));
    disc_title_copy(title,sizeof title,"x\xed\xa0\x80y");assert(!strcmp(title,"xy"));
}
static void expect(DiscDrive *d,DiscState state,const char *text) {
    char label[DISC_TITLE_BYTES+16];disc_label(&d->status,label,sizeof label);
    assert(d->status.state==state&&!strcmp(label,text));
}
static void drive_tests(void) {
    DiscDrive d;disc_init(&d);expect(&d,DISC_UNAVAILABLE,"Drive Unavailable");
    disc_observe(&d,TRAY_EMPTY,.1f);expect(&d,DISC_EMPTY,"Open Tray");
    assert(!disc_request(&d,DISC_LAUNCH,d.status.generation));
    assert(disc_request(&d,DISC_EJECT,d.status.generation));expect(&d,DISC_OPENING,"Opening");
    disc_observe(&d,TRAY_EMPTY,.1f);expect(&d,DISC_OPENING,"Opening");
    disc_observe(&d,TRAY_OPENING,.1f);expect(&d,DISC_OPENING,"Opening");
    disc_observe(&d,TRAY_OPEN,.1f);expect(&d,DISC_OPEN,"Close Tray");
    assert(disc_request(&d,DISC_CLOSE,d.status.generation));expect(&d,DISC_CLOSING,"Closing");
    disc_observe(&d,TRAY_OPEN,.1f);expect(&d,DISC_CLOSING,"Closing");
    disc_observe(&d,TRAY_CLOSING,.1f);disc_observe(&d,TRAY_CLOSED,.1f);expect(&d,DISC_READING,"Reading");
    uint32_t generation=d.status.generation;assert(disc_identified(&d,generation,0x4d5307e6,"Halo 3",true));
    expect(&d,DISC_READY,"Play Halo 3");
    assert(!disc_request(&d,DISC_LAUNCH,generation-1));assert(disc_request(&d,DISC_LAUNCH,generation));
    /* A physical eject/close cycle has no controller command. Old title reads
       cannot restore Play, including when the replacement has the same ID. */
    disc_observe(&d,TRAY_OPENING,.1f);expect(&d,DISC_OPENING,"Opening");assert(!d.status.title[0]&&!d.status.title_id);
    assert(!disc_identified(&d,generation,0x4d5307e6,"Stale title",true));
    disc_observe(&d,TRAY_OPEN,.1f);disc_observe(&d,TRAY_CLOSING,.1f);disc_observe(&d,TRAY_CLOSED,.1f);
    assert(!disc_identified(&d,generation,0x4d5307e6,"Stale title",true));
    assert(disc_identified(&d,d.status.generation,0x4d5307e6,"Halo 3",true));
    disc_observe(&d,TRAY_EMPTY,.1f);expect(&d,DISC_READING,"Reading");
    disc_media_update(&d,d.status.generation,0,0);
    disc_observe(&d,TRAY_EMPTY,3);disc_media_update(&d,d.status.generation,0,0);expect(&d,DISC_EMPTY,"Open Tray");
    disc_observe(&d,TRAY_CLOSED,.1f);disc_observe(&d,TRAY_CLOSED,31);expect(&d,DISC_ERROR,"Can't Read Disc");
    assert(disc_identified(&d,d.status.generation,0,NULL,false));expect(&d,DISC_UNSUPPORTED,"Unrecognized Disc");
    assert(disc_select(&d.status)==DISC_EJECT);
    disc_observe(&d,TRAY_OPEN,.1f);assert(disc_request(&d,DISC_CLOSE,d.status.generation));
    disc_observe(&d,TRAY_OPEN,4);expect(&d,DISC_ERROR,"Can't Read Disc");
    disc_observe(&d,TRAY_OPEN,.1f);expect(&d,DISC_OPEN,"Close Tray");
    disc_observe(&d,(DiscTray)99,NAN);expect(&d,DISC_ERROR,"Can't Read Disc");
    /* XAM's startup media query is needed for both CLOSED and EMPTY cached
       tray values (as in FreeStyle's force-trigger path). Media confirmation
       can establish a boot-inserted game without inventing an eject cycle. */
    disc_init(&d);disc_observe(&d,TRAY_EMPTY,0);generation=d.status.generation;
    assert(disc_begin_reading(&d,generation));disc_observe(&d,TRAY_EMPTY,.2f);
    expect(&d,DISC_READING,"Reading");assert(disc_identified(&d,generation,7,"Boot disc",true));
    disc_observe(&d,TRAY_EMPTY,.2f);expect(&d,DISC_READY,"Play Boot disc");
    disc_observe(&d,TRAY_OPENING,0);assert(!disc_begin_reading(&d,generation));
}
static void media_replay_tests(void) {
    assert(disc_media_kind(0,0xfffffffe)==DISC_MEDIA_PENDING);
    assert(disc_media_kind(0,0xffffffff)==DISC_MEDIA_PENDING);
    assert(disc_media_kind(0,254)==DISC_MEDIA_PENDING);
    assert(disc_media_kind(0,0x10001)==DISC_MEDIA_PENDING);
    assert(disc_media_kind(0x5553083b,1)==DISC_MEDIA_GAME);
    DiscDrive d;disc_init(&d);disc_observe(&d,TRAY_CLOSING,0);disc_observe(&d,TRAY_CLOSED,0);
    uint32_t generation=d.status.generation;
    /* Exact media words/return pair from the user's hardware log. A cached
       CLOSED -> EMPTY change must not end authentication or lose its token. */
    for(unsigned i=0;i<8;i++) {
        disc_observe(&d,i<4?TRAY_CLOSED:TRAY_EMPTY,1);
        assert(disc_media_update(&d,generation,0,0xfffffffe)==DISC_MEDIA_PENDING);
        expect(&d,DISC_READING,"Reading");assert(d.status.generation==generation);
    }
    assert(disc_media_update(&d,generation,0x5553083b,1)==DISC_MEDIA_GAME);
    assert(disc_identified(&d,generation,0x5553083b,"Detected Game",true));expect(&d,DISC_READY,"Play Detected Game");
    Dashboard ui;dashboard_init(&ui);ui.disc=d.status;ui.selected[BLADE_GAMES]=4;
    for(unsigned i=0;i<120;i++){dashboard_update(&ui,0,1.f/60);assert(ui.disc_command==DISC_NONE);}
    dashboard_update(&ui,SELECT,0);assert(ui.disc_command==DISC_LAUNCH);
    /* Both CLOSED and EMPTY are seen during an empty close. In either case,
       settle on Open Tray rather than interpreting no-media as unsupported. */
    for(unsigned cached=0;cached<2;cached++) {
        disc_init(&d);disc_observe(&d,TRAY_CLOSING,0);disc_observe(&d,TRAY_CLOSED,0);generation=d.status.generation;
        disc_media_update(&d,generation,0,0xfffffffe);
        for(unsigned second=1;second<=4;second++) {
            disc_observe(&d,cached?TRAY_EMPTY:TRAY_CLOSED,1);
            disc_media_update(&d,generation,0,second<3?0xfffffffe:0);
            if(second<4)expect(&d,DISC_READING,"Reading");
        }
        expect(&d,DISC_EMPTY,"Open Tray");assert(!d.status.title[0]&&!d.status.title_id);
    }
    /* A transient real media type also gets a settling window; only stable
       documented types become unsupported. Polling can recover afterwards. */
    disc_init(&d);disc_observe(&d,TRAY_CLOSED,0);generation=d.status.generation;
    disc_media_update(&d,generation,0,4);expect(&d,DISC_READING,"Reading");
    disc_observe(&d,TRAY_CLOSED,1);disc_media_update(&d,generation,0,0xfffffffe);expect(&d,DISC_READING,"Reading");
    for(unsigned i=0;i<3;i++){disc_observe(&d,TRAY_CLOSED,1);disc_media_update(&d,generation,0,4);}
    expect(&d,DISC_UNSUPPORTED,"Unrecognized Disc");
    disc_media_update(&d,generation,0,0xfffffffe);expect(&d,DISC_READING,"Reading");
    assert(disc_media_update(&d,generation,0x5553083b,1)==DISC_MEDIA_GAME);
    assert(disc_identified(&d,generation,0x5553083b,"Detected Game",true));
    disc_observe(&d,TRAY_OPENING,0);
    assert(disc_media_update(&d,generation,0,0)==DISC_MEDIA_PENDING);expect(&d,DISC_OPENING,"Opening");
    disc_init(&d);disc_observe(&d,TRAY_CLOSED,0);generation=d.status.generation;
    for(unsigned i=0;i<29;i++){disc_observe(&d,TRAY_CLOSED,1);disc_media_update(&d,generation,0,3);expect(&d,DISC_READING,"Reading");}
    disc_observe(&d,TRAY_CLOSED,1);disc_media_update(&d,generation,0,3);expect(&d,DISC_UNSUPPORTED,"Unrecognized Disc");
    for(unsigned i=0;i<3;i++){disc_observe(&d,TRAY_CLOSED,1);disc_media_update(&d,generation,0,3);expect(&d,DISC_UNSUPPORTED,"Unrecognized Disc");}
}
static void dashboard_tests(void) {
    Dashboard d;
    for(int blade=BLADE_LIVE;blade<=BLADE_MEDIA;blade++) {
        dashboard_init(&d);d.blade=blade;d.selected[blade]=menu_count(blade,0,0)-1;
        dashboard_update(&d,SELECT,0);assert(!d.depth&&d.disc_command==DISC_EJECT);
        d.disc.state=DISC_OPEN;dashboard_update(&d,SELECT,0);assert(!d.depth&&d.disc_command==DISC_CLOSE);
        d.disc.state=DISC_READING;dashboard_update(&d,SELECT,0);assert(!d.disc_command&&!d.depth);
        d.disc.state=DISC_READY;strcpy(d.disc.title,"Halo 3");
        for(unsigned i=0;i<120;i++){dashboard_update(&d,0,1.f/60);assert(!d.disc_command);}
        dashboard_update(&d,SELECT,0);assert(d.disc_command==DISC_LAUNCH&&!d.depth);
        dashboard_update(&d,0,0);assert(!d.disc_command);
        dashboard_update(&d,EJECT|SELECT,0);assert(d.disc_command==DISC_EJECT&&!d.depth);
        d.depth=1;dashboard_update(&d,EJECT,0);assert(d.disc_command==DISC_EJECT&&d.depth==1);
        d.dialog=true;dashboard_update(&d,EJECT,0);assert(!d.disc_command);
    }
    dashboard_begin_startup(&d);d.disc.state=DISC_READY;
    dashboard_update(&d,EJECT|SELECT,.01f);assert(!d.disc_command);
    Input in={0};input_update(&in,0,0,true);assert(input_update(&in,EJECT,0,true)==EJECT);
    for(unsigned i=0;i<120;i++)assert(!input_update(&in,EJECT,1.f/60,true));
    DrawList list={0};dashboard_init(&d);assert(draw_list_update(&list,&d));
    d.disc.state=DISC_READY;strcpy(d.disc.title,"Halo 3");assert(draw_list_update(&list,&d));
    assert(list.valid&&!draw_list_update(&list,&d));strcpy(d.disc.title,"Forza Motorsport 2");assert(draw_list_update(&list,&d));
    d.disc.generation++;assert(!draw_list_update(&list,&d));draw_list_close(&list);
    assert(disc_reading_frame(NAN)==0&&disc_reading_frame(INFINITY)==0&&disc_reading_frame(-1)==0);
    dashboard_init(&d);d.disc.state=DISC_READING;d.selected[BLADE_GAMES]=4;
    assert(draw_list_update(&list,&d));d.disc_elapsed=.04f;assert(draw_list_update(&list,&d));
    d.selected[BLADE_GAMES]=0;assert(draw_list_update(&list,&d));d.disc_elapsed=.08f;assert(!draw_list_update(&list,&d));
    draw_list_close(&list);
}
static void media_disc_tests(void) {
    const unsigned raw[]={5,7,8};const DiscMedia kinds[]={DISC_MEDIA_DVD_VIDEO,DISC_MEDIA_AUDIO_CD,DISC_MEDIA_DATA};
    const char *labels[]={"Play DVD","Play CD","Open Data Disc"};
    for(unsigned m=0;m<3;m++) {
        DiscDrive drive;disc_init(&drive);disc_observe(&drive,TRAY_CLOSED,0);uint32_t generation=drive.status.generation;
        for(unsigned n=0;n<3;n++){disc_observe(&drive,TRAY_CLOSED,1);assert(disc_media_update(&drive,generation,0,raw[m])==kinds[m]);if(n<2)assert(drive.status.state==DISC_READING);}
        expect(&drive,DISC_READY,labels[m]);assert(drive.status.media==kinds[m]&&!drive.status.title_id&&disc_select(&drive.status)==DISC_LAUNCH&&disc_can_eject(&drive.status));
        Dashboard d;dashboard_init(&d);d.disc=drive.status;d.selected[BLADE_GAMES]=4;
        for(unsigned i=0;i<120;i++){dashboard_update(&d,0,1.f/60);assert(!d.disc_command);}dashboard_update(&d,SELECT,0);assert(d.disc_command==DISC_LAUNCH);
        uint8_t data[1020];assert(disc_dashboard_data(kinds[m],0,data));assert(data[7]==(m==0?7:m==1?6:0)&&!data[3]&&data[11]==(m==0?0xff:0));
        for(size_t i=12;i<sizeof data;i++)assert(data[i]==0);
        disc_observe(&drive,TRAY_OPENING,0);assert(drive.status.media==DISC_MEDIA_PENDING);assert(!disc_request(&drive,DISC_LAUNCH,generation));
    }
    uint8_t data[1020];assert(!disc_dashboard_data(DISC_MEDIA_GAME,0,data));
}
int main(void) {media_disc_tests();
    uint32_t *pixels=calloc(WIDTH*HEIGHT,4);assert(pixels);Canvas canvas={.pixels=pixels};
    for(int r=0;r<=42;r++) {
        memset(pixels,0,WIDTH*HEIGHT*4);circle(&canvas,45,45,r,0xff123456);
        for(int y=0;y<90;y++)for(int x=0;x<90;x++)
            assert((pixels[y*WIDTH+x]!=0)==((x-45)*(x-45)+(y-45)*(y-45)<=r*r));
    }
    free(pixels);
    drive_tests();media_replay_tests();metadata_tests();dashboard_tests();
    const char *pack=getenv("BLADES_RESOURCE_PACK");
    if(pack){assert(resources_open(pack));dashboard_tests();resources_close();}
    puts("PASS: tray cycle, physical eject, no autoplay, manual launch/Y, stale reads, timeouts, metadata bounds and cache invalidation");
}
