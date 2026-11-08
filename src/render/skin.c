#include "skin.h"
#include "resources.h"
#include "animation.h"
#include "theme.h"
#include "startup.h"
#include <string.h>
#include <stdio.h>
enum { SLOTS=12, RECT_BYTES=8, HEADER_BYTES=12, RECT_END=HEADER_BYTES+BLADE_COUNT*SLOTS*RECT_BYTES,
    ROOT_END=RECT_END+BLADE_COUNT*HEIGHT*4, MANIFEST_BYTES=ROOT_END+BLADE_COUNT*HEIGHT*4 };
typedef struct {int x,y,w,h;} Rect;
static struct {unsigned generation;const unsigned char *data;bool disabled,expanded_bounds;} skin;
static unsigned u16(const unsigned char *p) {return (unsigned)p[0]<<8|p[1];}
static void name(char *out,size_t size,int state,int slot) {snprintf(out,size,"skin/%d/%d.png",state,slot);}
static bool ready(void) {
    unsigned generation=resources_generation();
    if(skin.generation==generation)return skin.data&&!skin.disabled;
    skin.generation=generation;skin.data=NULL;skin.disabled=false;skin.expanded_bounds=false;
    size_t size;const unsigned char *p=resources_blob("skin/layout.bin",&size);
    if(!p)return false;
    if(size<HEADER_BYTES||p[0]!='B'||p[1]!='S'||p[2]!='K')return false;
    bool extended=p[3]=='2';
    if((extended?size!=MANIFEST_BYTES:p[3]!='1'||size!=ROOT_END)||u16(p+4)!=WIDTH||u16(p+6)!=HEIGHT||u16(p+8)!=BLADE_COUNT||u16(p+10)!=SLOTS)return false;
    for(unsigned n=0;n<BLADE_COUNT*SLOTS;n++) {
        const unsigned char *r=p+HEADER_BYTES+n*RECT_BYTES;
        unsigned x=u16(r),y=u16(r+2),w=u16(r+4),h=u16(r+6);
        if(!w||!h||w>1024||h>1024||x+w>WIDTH||y+h>HEIGHT)return false;
        char key[64];name(key,sizeof key,n/SLOTS,n%SLOTS);size_t bytes;
        const unsigned char *png=resources_blob(key,&bytes);
        if(!png||bytes<24||png[0]!=137||png[1]!='P'||png[2]!='N'||png[3]!='G')return false;
    }
    for(unsigned n=0;n<BLADE_COUNT*HEIGHT*(extended?2u:1u);n++) {
        const unsigned char *r=p+RECT_END+n*4;
        if(u16(r)>=u16(r+2)||u16(r+2)>WIDTH)return false;
    }
    skin.data=p;skin.expanded_bounds=extended;return true;
}
static Rect rect(int state,int slot) {
    const unsigned char *p=skin.data+HEADER_BYTES+(state*SLOTS+slot)*RECT_BYTES;
    return (Rect){(int)u16(p),(int)u16(p+2),(int)u16(p+4),(int)u16(p+6)};
}
static bool draw(Canvas *c,int state,int slot,int dx,int dy,int w,int h) {
    char key[64];name(key,sizeof key,state,slot);
    if(!resources_draw(c,key,dx,dy,w,h)){skin.disabled=true;return false;}
    return true;
}
static bool draw_rect(Canvas *c,int state,int slot,int dx,int dy) {
    Rect r=rect(state,slot);return draw(c,state,slot,r.x+dx,r.y+dy,r.w,r.h);
}
static void bounds(int state,int y,int *left,int *right) {
    const unsigned char *p=skin.data+RECT_END+(state*HEIGHT+y)*4;
    *left=(int)u16(p);*right=(int)u16(p+2);
}
static void expanded_bounds(int state,int y,int *left,int *right) {
    *left=SUBMENU_LEFT;*right=SUBMENU_RIGHT;
    if(skin.expanded_bounds) {
        const unsigned char *p=skin.data+ROOT_END+(state*HEIGHT+y)*4;
        *left=(int)u16(p);*right=(int)u16(p+2);
    }
}
static void clip(Canvas *c,int state,float depth) {
    for(int y=0;y<HEIGHT;y++) {
        int left,right;bounds(state,y,&left,&right);
        int expanded_left,expanded_right;expanded_bounds(state,y,&expanded_left,&expanded_right);
        float t=anim_clamp(depth);
        left=(int)(left*(1-t)+expanded_left*t);right=(int)(right*(1-t)+expanded_right*t);
        fill_rect(c,0,y,left,1,0xffb9bec4);fill_rect(c,right,y,WIDTH-right,1,0xffb9bec4);
    }
}
/* Keep the supplied cap silhouettes. Cache only each column's edge; the
   shadow uses a few coalesced spans and needs no per-frame image allocation. */
typedef struct {bool tried,valid;int width,height;short edge[1024];} CapEdge;
static struct {unsigned generation;CapEdge caps[BLADE_COUNT][2];} cap_edges;
static CapEdge *cap_edge(int state,int slot) {
    unsigned generation=resources_generation();
    if(cap_edges.generation!=generation){memset(&cap_edges,0,sizeof cap_edges);cap_edges.generation=generation;}
    CapEdge *cap=&cap_edges.caps[state][slot-7];
    if(cap->tried)return cap->valid?cap:NULL;
    cap->tried=true;char key[64];name(key,sizeof key,state,slot);size_t size;
    const unsigned char *bytes=resources_blob(key,&size);if(!bytes)return NULL;
    unsigned char *rgba=resource_png_decode(bytes,(int)size,&cap->width,&cap->height);
    if(!rgba)return NULL;
    if(cap->width>0&&cap->width<=1024&&cap->height>0&&cap->height<=1024) {
        unsigned peak=0;for(int i=0;i<cap->width*cap->height;i++)if(rgba[i*4+3]>peak)peak=rgba[i*4+3];
        for(int x=0;x<cap->width;x++) {
            cap->edge[x]=-1;
            for(int y=0;y<cap->height;y++)if(rgba[(y*cap->width+x)*4+3]>peak/2) {
                cap->edge[x]=(short)y;if(slot==8)break;
            }
        }
        cap->valid=peak>0;
    }
    resource_png_free(rgba);return cap->valid?cap:NULL;
}
static void cap_shadow(Canvas *c,int state,int slot,int x,int y,int w,int h,unsigned opacity) {
    if(w<=0||h<=0||!opacity)return;
    CapEdge *cap=cap_edge(state,slot);if(!cap)return;
    /* Inset cap shading: the header darkens at its lower edge and fades
       upward; the footer darkens at its upper edge and fades downward. */
    static const unsigned char alpha[]={48,37,27,19,12,7,3,1};
    for(unsigned k=0;k<sizeof alpha;k++) {
        unsigned a=alpha[k]*opacity/255;if(!a)continue;
        int begin=0,last=-1;
        for(int i=0;i<=w;i++) {
            int edge=i<w?cap->edge[i*cap->width/w]:-1;
            int line=edge<0?-1:slot==7?y+(edge+1)*h/cap->height-1-(int)k:
                y+edge*h/cap->height+(int)k;
            if(line!=last) {
                if(last>=0)fill_rect(c,x+begin,last,i-begin,1,a<<24);
                begin=i;last=line;
            }
        }
    }
}
bool skin_startup_base(Canvas *c,float seconds) {
    if(!ready())return false;
    const int state=BLADE_GAMES;StartupFrame frame=startup_sample(seconds);
    unsigned center=(unsigned)(frame.center*255);
    for(int slot=7;slot<=8;slot++) {
        Rect r=rect(state,slot);char key[64];name(key,sizeof key,state,slot);
        if(!resources_draw_opacity(c,key,r.x,r.y,r.w,r.h,center))return false;
        cap_shadow(c,state,slot,r.x,r.y,r.w,r.h,center);
    }
    /* Underpaint follows the emerging full blade silhouettes. Rectangular
       image wipes would slice their curves/vertical lettering during entry. */
    for(int y=0;y<HEIGHT;y++) {
        int left,right;bounds(state,y,&left,&right);
        left+=(int)lroundf(frame.offset[BLADE_GAMES]);right+=(int)lroundf(frame.offset[BLADE_MEDIA]);
        fill_rect(c,0,y,left,1,0xffb9bec4);fill_rect(c,right,y,WIDTH-right,1,0xffb9bec4);
    }
    const int order[]={5,6,0,1,3,4,2};
    for(unsigned i=0;i<sizeof order/sizeof order[0];i++) {
        int slot=order[i];Rect r=rect(state,slot);char key[64];name(key,sizeof key,state,slot);
        int dx=slot<BLADE_COUNT?(int)lroundf(frame.offset[slot]):0;
        unsigned alpha=slot<BLADE_COUNT?255:(unsigned)(frame.runners*255);
        if(!resources_draw_opacity(c,key,r.x+dx,r.y,r.w,r.h,alpha))return false;
    }
    return true;
}
bool skin_startup_shutters(Canvas *c,float seconds) {
    size_t size;const unsigned char *p=resources_blob("startup/layout.bin",&size);
    if(!p||size!=28||memcmp(p,"STP1",4)||u16(p+4)!=WIDTH||u16(p+6)!=HEIGHT||u16(p+8)!=2||u16(p+10))return false;
    StartupFrame frame=startup_sample(seconds);
    int dx[2]={(int)lroundf(frame.left-739*(2.f/3)),(int)lroundf(frame.right-1203*(2.f/3))};
    for(unsigned i=0;i<2;i++) {
        const unsigned char *r=p+12+i*8;
        unsigned x=u16(r),y=u16(r+2),w=u16(r+4),h=u16(r+6);
        if(!w||!h||w>1024||h>1024||x+w>WIDTH||y+h>HEIGHT)return false;
        if(!resources_draw_opacity(c,i?"startup/right.png":"startup/left.png",(int)x+dx[i],(int)y,(int)w,(int)h,
            (unsigned)(frame.shutters*255)))return false;
        unsigned gray=((unsigned)(frame.shutters*255)<<24)|0xb9bec4;
        if(i==0&&dx[i]>0)fill_rect(c,0,0,dx[i],HEIGHT,gray);
        if(i==1&&dx[i]<0)fill_rect(c,WIDTH+dx[i],0,-dx[i],HEIGHT,gray);
    }
    return true;
}
bool skin_frame(Canvas *c,int blade,float depth,float header,float footer) {
    if(!ready())return false;
    Rect top=rect(blade,7),bottom=rect(blade,8);
    int unused,top_right,bottom_right;
    expanded_bounds(blade,0,&unused,&top_right);expanded_bounds(blade,HEIGHT-1,&unused,&bottom_right);
    float t=anim_clamp(depth);
    int tx=(int)(top.x+(104-top.x)*t),tw=(int)(top.w+(top_right-104-top.w)*t);
    int bx=(int)(bottom.x+(99-bottom.x)*t),bw=(int)(bottom.w+(bottom_right-99-bottom.w)*t);
    int top_extra=(int)(header-68),bottom_extra=(int)(610-footer);
    /* Stretch caps to the expanded shell, then clip to its curved silhouette.
       The underpaint must include the active tab and translucent chrome folds. */
    bool ok=draw(c,blade,7,tx,top.y,tw,top.h+top_extra)&&
            draw(c,blade,8,bx,bottom.y-bottom_extra,bw,bottom.h+bottom_extra);
    if(ok) {
        cap_shadow(c,blade,7,tx,top.y,tw,top.h+top_extra,255);
        cap_shadow(c,blade,8,bx,bottom.y-bottom_extra,bw,bottom.h+bottom_extra,255);
        clip(c,blade,depth);
    }
    return ok;
}
bool skin_chrome(Canvas *c,int blade,float depth,bool closing) {
    if(!ready())return false;
    float t=anim_clamp(depth);
    for(int slot=5;slot<=6;slot++) {
        Rect r=rect(blade,slot);
        int target=slot==5?35-r.w:SUBMENU_RIGHT;
        if(!draw_rect(c,blade,slot,(int)((target-r.x)*t),0))return false;
    }
    for(int i=0;i<BLADE_COUNT;i++)if(i!=blade) {
        Rect r=rect(blade,i);
        int target=i<blade?-r.w-(blade-i-1)*30:WIDTH+(i-blade-1)*30;
        /* Outer inactive tabs clear before the active leaf finishes moving.
           The adjacent right blade stays as the moving panel boundary. */
        float retraction=(i<blade||i>blade+1)?anim_clamp(t/(closing?1.0f/3:.20f)):t;
        if(!draw_rect(c,blade,i,(int)((target-r.x)*retraction),0))return false;
    }
    Rect active=rect(blade,blade);
    return draw_rect(c,blade,blade,(int)((SUBMENU_RAIL_LEFT-active.x)*t),0);
}
bool skin_leaf(Canvas *c,int from,int to,float progress) {
    if(!ready())return false;
    float t=anim_clamp(progress);int moving=to>from?to:from;
    for(int y=0;y<HEIGHT;y++) {
        int la,ra,lb,rb;bounds(from,y,&la,&ra);bounds(to,y,&lb,&rb);
        int left=la<lb?la:lb,right=ra>rb?ra:rb;
        fill_rect(c,0,y,left,1,0xffb9bec4);fill_rect(c,right,y,WIDTH-right,1,0xffb9bec4);
    }
    for(int slot=5;slot<=6;slot++) {
        Rect a=rect(from,slot),b=rect(to,slot);
        if(!draw(c,to,slot,(int)(a.x+(b.x-a.x)*t),(int)(a.y+(b.y-a.y)*t),b.w,b.h))return false;
    }
    for(int i=0;i<BLADE_COUNT;i++)if(i!=moving) {
        Rect a=rect(from,i),b=rect(to,i);
        if(!draw(c,to,i,(int)(a.x+(b.x-a.x)*t),(int)(a.y+(b.y-a.y)*t),b.w,b.h))return false;
    }
    Rect a=rect(from,moving),b=rect(to,moving);
    return draw(c,to,moving,(int)(a.x+(b.x-a.x)*t),(int)(a.y+(b.y-a.y)*t),b.w,b.h);
}
bool skin_tray(Canvas *c,int blade,bool selected,const char *label) {
    if(!ready())return false;
    Rect r=rect(blade,9);
    if(selected) {
        theme_round(c,r.x,r.y,r.w,r.h,r.h/2,0x35ffffff);
    }
    for(int i=9;i<12;i++)if(!draw_rect(c,blade,i,0,0))return false;
    theme_label_fit(c,r.x+75,r.y+(r.h-29)/2-5,29,r.w-92,label);
    return true;
}
