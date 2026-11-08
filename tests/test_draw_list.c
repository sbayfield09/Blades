#include "draw_list.h"
#include "gpu_plan.h"
#include "theme.h"
#include "resources.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static void check(DrawList *list,Dashboard *d,Canvas *direct,Canvas *replayed,bool exact) {
    dashboard_render(direct,d);draw_list_update(list,d);assert(list->valid);
    assert(draw_list_software(list,replayed,d->elapsed));
    unsigned max=0;uint64_t error=0;
    for(int i=0;i<WIDTH*HEIGHT;i++)for(unsigned s=0;s<=16;s+=8) {
        int e=(int)((direct->pixels[i]>>s)&255)-(int)((replayed->pixels[i]>>s)&255);unsigned v=(unsigned)abs(e);
        if(v>max)max=v;error+=v;
    }
    if(exact&&max)fprintf(stderr,"command mismatch blade=%d depth=%d max=%u mean=%.6f\n",d->blade,d->depth,max,(double)error/(WIDTH*HEIGHT*3));
    assert(!exact||max==0);
    GpuPlan plan={0};assert(gpu_plan_build(&plan,list));
    assert(plan.vertex_count&&plan.batch_count&&plan.image_count);
    for(size_t i=0;i<plan.batch_count;i++) {
        const GpuBatch *b=plan.batches+i;
        assert(b->first+b->count<=plan.vertex_count&&b->count<=65532&&b->count%6==0);
        assert(b->image<plan.image_count);
    }
    gpu_plan_close(&plan);
}
/* The fill reaches the divider location. Its shadow replaces the selected
   groove, rather than a separate light rule cutting across the penumbra. */
static unsigned brightness(uint32_t p){return ((p>>16)&255)+((p>>8)&255)+(p&255);}
static void row_edge(Canvas *focused,Canvas *neutral,Dashboard *d,int x,int y,bool ftp) {
    Dashboard base=*d;
    if(ftp){base.child=d->child?0:2;base.highlight=(float)base.child;}
    else base.profile_card_selected=true;
    dashboard_render(neutral,&base);dashboard_render(focused,d);
    for(int offset=-2;offset<=50;offset++) {
        if(offset>0&&offset<48)continue;
        unsigned a=brightness(focused->pixels[(y+offset)*WIDTH+x]),b=brightness(neutral->pixels[(y+offset)*WIDTH+x]);
        if(a<=b)fprintf(stderr,"edge failed blade=%d depth=%d selected=%d child=%d ftp=%d x=%d y=%d offset=%d focused=%u neutral=%u\n",d->blade,d->depth,d->selected[d->blade],d->child,ftp,x,y,offset,a,b);
        assert(a>b);
    }
    for(int offset=51;offset<=54;offset++)
        assert(brightness(focused->pixels[(y+offset)*WIDTH+x])<brightness(neutral->pixels[(y+offset)*WIDTH+x]));
    assert(brightness(focused->pixels[(y+53)*WIDTH+x])<=brightness(focused->pixels[(y+54)*WIDTH+x]));
    assert(brightness(focused->pixels[(y+54)*WIDTH+x])<brightness(neutral->pixels[(y+54)*WIDTH+x]));
}
static void menu_edges(Canvas *focused,Canvas *neutral) {
    Dashboard d;
    for(int blade=0;blade<BLADE_COUNT;blade++)for(int row=0;row<2;row++) {
        dashboard_init(&d);d.blade=d.previous_blade=blade;d.selected[blade]=row;d.highlight=(float)row;d.elapsed=3.25f;
        int y=(blade_has_profile(blade)?238:96)+row*53;
        row_edge(focused,neutral,&d,theme_content_left(blade)+206,y,false);
        if(row<menu_count(blade,1,d.selected[blade])) {
            d.depth=d.previous_depth=1;d.child=row;d.highlight=(float)row;
            row_edge(focused,neutral,&d,400,87+row*53,false);
        }
    }
    for(int row=0;row<3;row++) {
        dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=4;d.depth=d.previous_depth=1;d.child=row;d.highlight=(float)row;d.elapsed=3.25f;
        row_edge(focused,neutral,&d,400,87+row*53,true);
    }
    /* Card sheen reaches the inner lower border. Both lower frame lines
       remain unchanged and the existing shadow begins outside the frame. */
    dashboard_init(&d);d.elapsed=3.25f;dashboard_render(neutral,&d);
    d.profile_card_selected=true;dashboard_render(focused,&d);
    int x=theme_content_left(BLADE_GAMES)+200;
    assert(brightness(focused->pixels[220*WIDTH+x])>brightness(neutral->pixels[220*WIDTH+x]));
    assert(focused->pixels[221*WIDTH+x]==neutral->pixels[221*WIDTH+x]);
    assert(focused->pixels[223*WIDTH+x]==neutral->pixels[223*WIDTH+x]);
    /* The shadow begins outside the rounded corner higher than the centre,
       then finishes sooner there. A rectangular under-strip fails both. */
    theme_background(neutral,BLADE_GAMES,d.elapsed);
    int left=theme_content_left(BLADE_GAMES);
    for(int side=0;side<2;side++) {
        int corner=left+(side?394:5);
        assert(brightness(focused->pixels[222*WIDTH+corner])<brightness(neutral->pixels[222*WIDTH+corner]));
        assert(focused->pixels[232*WIDTH+corner]==neutral->pixels[232*WIDTH+corner]);
    }
    assert(brightness(focused->pixels[224*WIDTH+x])<brightness(neutral->pixels[224*WIDTH+x]));
    assert(focused->pixels[237*WIDTH+x]==neutral->pixels[237*WIDTH+x]);
    /* Include the rounded bevel: the penumbra must start at the former
       divider, never above the last highlighted pixel, in either style. */
    for(int rounded=0;rounded<2;rounded++) {
        for(int i=0;i<WIDTH*HEIGHT;i++)neutral->pixels[i]=0xff808080;
        theme_menu_shadow(neutral,250,289,400,526,rounded!=0);
        for(int y=286;y<=288;y++)for(int px=249;px<=651;px++)
            assert(neutral->pixels[y*WIDTH+px]==0xff808080);
        assert(brightness(neutral->pixels[289*WIDTH+450])<384);
    }
    puts("PASS: 20 row-edge checks, selected divider covered by shadow, including rounded bevel; card fill/frame join and both curved shadow ends");
}
static void upper_groups(Canvas *upper,Canvas *lower) {
    for(int rounded=0;rounded<2;rounded++) {
        for(int i=0;i<WIDTH*HEIGHT;i++)upper->pixels[i]=lower->pixels[i]=0xff808080;
        theme_menu_upper_shadow(upper,250,100,400,100,rounded!=0);
        assert(!memcmp(upper->pixels,lower->pixels,WIDTH*HEIGHT*4));
        theme_menu_upper_shadow(upper,250,100,400,200,rounded!=0);
        theme_menu_shadow(lower,250,0,400,100,rounded!=0);
        for(int y=0;y<100;y++)for(int x=0;x<WIDTH;x++)
            assert(upper->pixels[(199-y)*WIDTH+x]==lower->pixels[y*WIDTH+x]);
        for(int x=0;x<WIDTH;x++) {
            assert(upper->pixels[99*WIDTH+x]==0xff808080);
            assert(upper->pixels[200*WIDTH+x]==0xff808080);
        }
        if(rounded) {
            assert(brightness(upper->pixels[188*WIDTH+648])<384);
            assert(upper->pixels[199*WIDTH+648]==0xff808080);
        }
    }
    Dashboard d;
    for(int blade=0;blade<BLADE_COUNT;blade++) {
        dashboard_init(&d);d.blade=d.previous_blade=blade;d.elapsed=3.25f;
        dashboard_render(upper,&d);theme_background(lower,blade,d.elapsed);
        int first=blade_has_profile(blade)?238:96,x=theme_content_left(blade)+206;
        assert(brightness(upper->pixels[(first-4)*WIDTH+x])<brightness(lower->pixels[(first-4)*WIDTH+x]));
        assert(brightness(upper->pixels[(first-1)*WIDTH+x])>brightness(lower->pixels[(first-1)*WIDTH+x]));
    }
    dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=4;
    d.depth=d.previous_depth=1;dashboard_render(upper,&d);theme_background(lower,BLADE_SYSTEM,d.elapsed);
    assert(brightness(upper->pixels[83*WIDTH+400])<brightness(lower->pixels[83*WIDTH+400]));
    assert(brightness(upper->pixels[86*WIDTH+400])>brightness(lower->pixels[86*WIDTH+400]));
    puts("PASS: upper shadow exactly mirrors lower pixels in both styles; no upper on first row, clipped upper group and top dividers on five roots/FTP");
}
static int ftp_cursor(const DrawList *list) {
    for(size_t i=0;i<list->count;i++) {
        const DrawOp *op=list->ops+i;
        if(op->kind==DRAW_QUAD&&!op->image.pixels&&op->x==400&&op->w==1&&op->h>20&&
           (op->color&0xffffff)==0xe2eadf)return op->y;
    }
    assert(!"missing FTP selection body");return -1;
}
static void ftp_motion_spacing(void) {
    Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=4;
    dashboard_update(&d,SELECT,0);d.menu_transition=1;d.previous_depth=1;
    d.ftp=(FtpSnapshot){.state=FTP_RUNNING,.enabled=true,.devices=1};
    strcpy(d.ftp.ip,"Running");strcpy(d.ftp.username,"Running");strcpy(d.ftp.password,"Running");
    DrawList list={0};assert(draw_list_update(&list,&d));int first=ftp_cursor(&list);
    /* Equal value strings isolate actual glyph placement/font size from
       differing words. All four first letters must share an86px pitch. */
    DrawOp ref={0};bool have=false;int ys[4]={0};unsigned count=0;
    for(size_t i=0;i<list.count;i++) {
        DrawOp op=list.ops[i];
        if(op.kind!=DRAW_QUAD||op.image.format!=IMAGE_MASK||!op.image.pixels||op.color!=0xff161f24||op.x<708||op.x>712)continue;
        if(!have&&op.y>=130&&op.y<150){ref=op;have=true;}
        if(have&&op.image.id==ref.image.id&&op.sx==ref.sx&&op.sy==ref.sy&&op.w==ref.w&&op.h==ref.h&&op.y>=130&&op.y<420) {
            assert(count<4);ys[count++]=op.y;
        }
    }
    assert(count==4);for(unsigned i=1;i<count;i++)assert(ys[i]-ys[i-1]==86);
    dashboard_update(&d,DOWN,0);assert(d.child==1&&d.highlight==0);
    assert(draw_list_update(&list,&d));assert(ftp_cursor(&list)==first);
    dashboard_update(&d,0,1.f/60);assert(d.highlight>0&&d.highlight<1);
    assert(draw_list_update(&list,&d));int middle=ftp_cursor(&list);assert(middle>first&&middle<first+53);
    for(int frame=0;frame<30;frame++)dashboard_update(&d,0,1.f/60);
    assert(draw_list_update(&list,&d));assert(ftp_cursor(&list)==first+53);
    dashboard_update(&d,SELECT,0);assert(d.ftp_edit==1&&!d.ftp_toggle);
    draw_list_close(&list);
    puts("PASS: FTP equal86px value pitch/common font, interpolated cursor/cache updates and exact final row alignment");
}
static void live_group_edges(Canvas *focused,Canvas *neutral) {
    Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_LIVE;d.elapsed=3.25f;
    for(int row=0;row<3;row++) {
        d.selected[BLADE_LIVE]=row;d.highlight=(float)row;d.profile_card_selected=true;
        dashboard_render(neutral,&d);d.profile_card_selected=false;dashboard_render(focused,&d);
        if(row<2) {
            int below=row?378:370;
            assert(brightness(focused->pixels[below*WIDTH+602])<brightness(neutral->pixels[below*WIDTH+602]));
        }
        if(row>0) {
            int above=row==1?260:300;
            assert(brightness(focused->pixels[above*WIDTH+602])<brightness(neutral->pixels[above*WIDTH+602]));
        }
        /* The entire bottom divider remains, including its right end.
           No shadow extends through or beneath it, even with Chat selected. */
        for(int y=395;y<=399;y++)for(int x=208;x<=608;x++)
            assert(focused->pixels[y*WIDTH+x]==neutral->pixels[y*WIDTH+x]);
        /* Preserve the raised logo tile's own face and bevel layering. */
        assert(focused->pixels[450*WIDTH+600]==neutral->pixels[450*WIDTH+600]);
    }
    puts("PASS: Xbox Live side rails, full bottom divider and no downward curve under Chat");
}
static void score_and_brand(Canvas *with_shadow,Canvas *plain) {
    Dashboard d;dashboard_init(&d);
    ProfileSnapshot p={.identity={.state=PROFILE_LOCAL,.xuid=42,.name="Test"},.known=7,.score=12345,.revision=1};
    dashboard_set_profile(&d,&p);DrawList list={0};assert(draw_list_update(&list,&d));
    int icon_bottom=-1,text_bottom=-1,origin=theme_content_left(BLADE_GAMES);
    for(size_t n=0;n<list.count;n++) {
        const DrawOp *op=list.ops+n;if(op->kind!=DRAW_QUAD||!op->image.pixels)continue;
        if(op->image.format==IMAGE_RGBA&&op->x==origin+104&&op->w==32&&op->h==32) {
            for(int y=0;y<op->image.height;y++)for(int x=0;x<op->image.width;x++)
                if(op->image.pixels[(y*op->image.width+x)*4+3])icon_bottom=op->y+y*op->h/op->image.height;
        }
        if(op->image.format==IMAGE_MASK&&op->color==0xff161f24&&op->y==146&&op->x>=origin+130) {
            for(int y=0;y<op->sh;y++)for(int x=0;x<op->sw;x++)
                if(op->image.pixels[(op->sy+y)*op->image.width+op->sx+x]&&op->y+y>text_bottom)text_bottom=op->y+y;
        }
    }
    assert(icon_bottom>=0&&text_bottom>=0&&icon_bottom==text_bottom+6);draw_list_close(&list);
    for(int i=0;i<WIDTH*HEIGHT;i++)with_shadow->pixels[i]=plain->pixels[i]=0xffffffff;
    assert(resources_draw_fit(plain,"splash_360.png",100,100,190,118));
    assert(resources_draw_opacity(with_shadow,"branding/360-shadow.png",96,97,202,130,120));
    assert(resources_draw_fit(with_shadow,"splash_360.png",100,100,190,118));
    unsigned outside=0;
    for(int y=90;y<235;y++)for(int x=90;x<305;x++) {
        unsigned a=with_shadow->pixels[y*WIDTH+x],b=plain->pixels[y*WIDTH+x];
        for(unsigned shift=0;shift<24;shift+=8)assert(((a>>shift)&255)<=((b>>shift)&255));
        outside+=b==0xffffffff&&a!=b;
    }
    assert(outside>50);
    puts("PASS: Gamerscore icon restored to pre-0.36 position; cached original-logo shadow extends softly outside original silhouette");
}
static void live_cursor_cache(Canvas *direct,Canvas *replayed) {
    Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_LIVE;
    d.selected[BLADE_LIVE]=2;d.elapsed=3.25f;
    DrawList list={0};
    /* Both positions truncate to105, but the second rounds to106. The
       latter is Chat's exact divider edge and must evict the old shadow. */
    d.highlight=105.25f/53;assert(draw_list_update(&list,&d));
    d.highlight=105.75f/53;assert(draw_list_update(&list,&d));
    assert(draw_list_software(&list,replayed,d.elapsed));dashboard_render(direct,&d);
    assert(!memcmp(direct->pixels,replayed->pixels,WIDTH*HEIGHT*4));
    /* Exercise actual Down and Up approaches, then idle reuse, rather than
       testing only manually snapped row positions. The tray is row3. */
    for(unsigned direction=0;direction<2;direction++) {
        dashboard_init(&d);d.blade=d.previous_blade=BLADE_LIVE;d.elapsed=3.25f;
        d.selected[BLADE_LIVE]=direction?3:1;d.highlight=(float)d.selected[BLADE_LIVE];
        assert(draw_list_update(&list,&d));dashboard_update(&d,direction?UP:DOWN,0);
        assert(d.selected[BLADE_LIVE]==2);
        for(unsigned frame=0;frame<60;frame++) {
            dashboard_update(&d,0,1.f/60);draw_list_update(&list,&d);
            assert(draw_list_software(&list,replayed,d.elapsed));dashboard_render(direct,&d);
            assert(!memcmp(direct->pixels,replayed->pixels,WIDTH*HEIGHT*4));
        }
        unsigned builds=list.builds;
        dashboard_update(&d,0,1.f/60);assert(!draw_list_update(&list,&d)&&list.builds==builds);
    }
    draw_list_close(&list);
    puts("PASS: Chat shadow clears through cached Down/Up animation and reuses settled foreground");
}
int main(int argc,char **argv) {
    assert(gpu_ambient_frame(0)==0&&gpu_ambient_frame(1.0f/60)==0&&gpu_ambient_frame(1.0f/30)==1);
    assert(gpu_ambient_frame(NAN)==0&&gpu_ambient_frame(INFINITY)==0&&gpu_ambient_frame(-1)==0);
    const char *pack=argc>1?argv[1]:getenv("BLADES_RESOURCE_PACK");
    bool assets=pack&&resources_open(pack);
    uint32_t *a=malloc((WIDTH*HEIGHT+2)*4),*b=malloc((WIDTH*HEIGHT+2)*4);assert(a&&b);
    a[0]=b[0]=0x12345678;a[WIDTH*HEIGHT+1]=b[WIDTH*HEIGHT+1]=0x87654321;
    Canvas direct={.pixels=a+1},replayed={.pixels=b+1};DrawList list={0};Dashboard d;dashboard_init(&d);
    check(&list,&d,&direct,&replayed,true);
    unsigned builds=list.builds;d.elapsed=7.125f;assert(!draw_list_update(&list,&d)&&list.builds==builds);
    assert(draw_list_software(&list,&replayed,d.elapsed));dashboard_render(&direct,&d);
    assert(!memcmp(direct.pixels,replayed.pixels,WIDTH*HEIGHT*4));
    for(unsigned frame=0;frame<600;frame++){dashboard_update(&d,0,1.0f/60);assert(!draw_list_update(&list,&d));}
    assert(list.builds==builds); /* Ten idle seconds never rebuild foreground. */
    unsigned states=1;
    for(int blade=0;blade<BLADE_COUNT;blade++)for(int row=0;row<menu_count(blade,0,0);row++) {
        if(!assets&&(blade!=BLADE_GAMES||row>0))continue;
        dashboard_init(&d);d.blade=d.previous_blade=blade;d.selected[blade]=row;d.highlight=(float)row;d.elapsed=3.25f;
        check(&list,&d,&direct,&replayed,true);states++;
        d.depth=d.previous_depth=1;d.highlight=0;
        check(&list,&d,&direct,&replayed,true);states++;
    }
    if(assets) {
        menu_edges(&direct,&replayed);
        upper_groups(&direct,&replayed);
        ftp_motion_spacing();
        live_group_edges(&direct,&replayed);
        live_cursor_cache(&direct,&replayed);
        score_and_brand(&direct,&replayed);
        /* Exercise old-content fades, the moving two-background leaf, arriving
           content, submenu opening/closing, clipping and dialog/exit overlays. */
        dashboard_init(&d);dashboard_update(&d,RIGHT,0);
        const float t[]={.01f,.12f,.2f,.3f,.45f,.54f};
        for(unsigned i=0;i<sizeof t/sizeof t[0];i++){d.transition=t[i]/.55f;check(&list,&d,&direct,&replayed,false);states++;}
        dashboard_init(&d);d.selected[BLADE_GAMES]=1;dashboard_update(&d,SELECT,0);
        for(unsigned i=0;i<sizeof t/sizeof t[0];i++){d.menu_transition=t[i]/.94f;check(&list,&d,&direct,&replayed,false);states++;}
        d.depth=0;d.previous_depth=1;
        for(unsigned i=0;i<sizeof t/sizeof t[0];i++){d.menu_transition=t[i]/.94f;check(&list,&d,&direct,&replayed,false);states++;}
        dashboard_init(&d);dashboard_update(&d,PROFILE,0);check(&list,&d,&direct,&replayed,true);states++;
        dashboard_init(&d);dashboard_update(&d,EXIT,0);check(&list,&d,&direct,&replayed,true);states++;
    }
    dashboard_init(&d);ProfileSnapshot profile={.identity={.state=PROFILE_LOCAL,.xuid=42,.name="Example"},.revision=1,.picture_revision=1,.known=7,.score=12345,.has_picture=true};
    memset(profile.picture,255,sizeof profile.picture);dashboard_set_profile(&d,&profile);check(&list,&d,&direct,&replayed,true);states++;
    builds=list.builds;assert(!draw_list_update(&list,&d));profile.score=23456;profile.revision++;dashboard_set_profile(&d,&profile);check(&list,&d,&direct,&replayed,true);assert(list.builds==builds+1);states++;
    memset(profile.picture,100,sizeof profile.picture);profile.picture_revision++;profile.revision++;dashboard_set_profile(&d,&profile);check(&list,&d,&direct,&replayed,true);states++;
    dashboard_update(&d,PROFILE,0);check(&list,&d,&direct,&replayed,true);states++;
    profile.identity.state=PROFILE_SIGNED_OUT;profile.identity.xuid=0;profile.revision++;dashboard_set_profile(&d,&profile);dashboard_update(&d,UP,0);assert(d.profile_card_selected);check(&list,&d,&direct,&replayed,true);states++;
    assert(a[0]==0x12345678&&a[WIDTH*HEIGHT+1]==0x87654321&&b[0]==0x12345678&&b[WIDTH*HEIGHT+1]==0x87654321);
    unsigned generation=list.generation;resources_close();assert(draw_list_update(&list,&d)&&list.generation!=generation);
    draw_list_close(&list);free(a);free(b);
    printf("PASS: %u command states, static pixel equivalence, cache reuse/invalidation, bounded batches and canvas guards\n",states);
}
