#include "theme.h"
#include "draw_list.h"
#include <string.h>
void theme_label_fit(Canvas *c,int x,int y,int size,int width,const char *label) {
    char shortened[DISC_TITLE_BYTES+16];disc_title_copy(shortened,sizeof shortened,label);
    if(text_width(size,shortened)>width) {
        size_t n=strlen(shortened);
        while(n) {
            n--;while(n&&((unsigned char)shortened[n]&0xc0)==0x80)n--;
            memcpy(shortened+n,"...",4);
            if(text_width(size,shortened)<=width)break;
        }
    }
    theme_label(c,x,y,size,shortened,false);
}
#include "ambient.h"
#include "skin.h"
#include "animation.h"
#include <math.h>
/* Console-feedback tints; retain the supplied motion and light equations. */
static const uint32_t palette[BLADE_COUNT]={0xffde6e35,0xffda9f4c,0xff4ab239,0xff3b87cd,0xff906ed4};
int theme_left(int b) {return 100+b*48;}
int theme_right(int b) {return 994+b*48;}
int theme_content_left(int b) {
    static const int origins[BLADE_COUNT]={140,202,250,294,335};
    return origins[b];
}
uint32_t theme_color(int b) {return palette[b];}
uint32_t theme_mix(uint32_t a,uint32_t b,float t) {
    if(t<0)t=0;
    if(t>1)t=1;
    uint32_t out=0xff000000;
    for(int s=0;s<=16;s+=8)out|=(uint32_t)(((a>>s)&255)*(1-t)+((b>>s)&255)*t)<<s;
    return out;
}
void theme_round(Canvas *c,int x,int y,int w,int h,int r,uint32_t color) {
    fill_rect(c,x+r,y,w-2*r,h,color);
    for(int j=0;j<h;j++) {
        int dy=j<r?r-j:j>=h-r?j-(h-r-1):0;
        int inset=dy?r-(int)sqrtf((float)(r*r-dy*dy)):0;
        fill_rect(c,x+inset,y+j,r-inset,1,color);
        fill_rect(c,x+w-r,y+j,r-inset,1,color);
    }
}
void theme_outline(Canvas *c,int x,int y,int w,int h,int r,uint32_t color) {
    for(int j=0;j<h;j++) {
        int dy=j<r?r-j:j>=h-r?j-(h-r-1):0;
        int inset=dy?r-(int)sqrtf((float)(r*r-dy*dy)):0;
        if(j==0||j==h-1)fill_rect(c,x+inset,y+j,w-2*inset,1,color);
        else {
            int idy=j<r?r-j:j>=h-r?j-(h-r-1):0;
            int inner=idy>r-1?r:1+(idy?(r-1)-(int)sqrtf((float)((r-1)*(r-1)-idy*idy)):0);
            int edge=inner-inset;if(edge<1)edge=1;
            fill_rect(c,x+inset,y+j,edge,1,color);
            fill_rect(c,x+w-inset-edge,y+j,edge,1,color);
        }
    }
}
void theme_label(Canvas *c,int x,int y,int size,const char *label,bool selected) {
    text(c,x+1,y+1,size,selected?0x65000000:0x45ffffff,label);
    text(c,x,y,size,selected?0xfffafbf5:0xff161f24,label);
}
/* Measured idle-selection penumbra from the supplied Games/Media captures.
   A dark ridge sits just below the selected row, then softens over 15 pixels. */
static float shadow_coverage(float distance) {
    static const unsigned char alpha[]={128,154,143,130,117,105,94,83,72,61,50,39,28,18,8,0};
    if(distance<0||distance>=15)return 0;
    unsigned i=(unsigned)distance;float t=distance-i;
    return alpha[i]+(alpha[i+1]-alpha[i])*t;
}
/* The left end retains a thin contact edge, while its soft penumbra vanishes.
   The full underside develops over roughly half the selected row's width. */
static float shadow_contact(float distance) {
    static const unsigned char alpha[]={70,76,51,20,0};
    if(distance<0||distance>=4)return 0;
    unsigned i=(unsigned)distance;float t=distance-i;
    return alpha[i]+(alpha[i+1]-alpha[i])*t;
}
static void shadow_strip(Canvas *c,int x,int y,int width,float ramp,float left,float right,uint32_t rgb) {
    if(width<=0)return;
    float step=ramp>0?1/ramp:1;
    int begin=0;unsigned previous=(unsigned)left;
    for(int i=1;i<width;i++) {
        unsigned alpha=(unsigned)(left+(right-left)*anim_clamp(i*step));
        if(alpha!=previous) {
            if(previous)fill_rect(c,x+begin,y,i-begin,1,(previous<<24)|rgb);
            previous=alpha;begin=i;
        }
    }
    if(previous)fill_rect(c,x+begin,y,width-begin,1,(previous<<24)|rgb);
}
void theme_menu_shadow(Canvas *c,int x,int top,int width,int bottom,bool rounded) {
    if(width<=0||bottom<=top)return;
    int right=x+width-1,radius=width<20?width/2:10;
    int end=bottom<HEIGHT?bottom:HEIGHT;
    int start=top>0?top:0;
    for(int y=start;y<end;y++) {
        float fade=1-anim_smooth((float)(y-top)/(bottom-top));
        float horizontal=shadow_coverage((float)(y-top));
        float contact=shadow_contact((float)(y-top));
        float ramp=width*.5f;
        if(!rounded) {
            if(y-top>15)break;
            int half=width/2;
            shadow_strip(c,x,y,half,ramp,contact,horizontal,0);
            shadow_strip(c,x+half,y,width-half,(float)(width-half),horizontal,horizontal*.55f,0);
            continue;
        }
        /* The underside fades to a thin contact edge on the left. Coalesce
           equal-alpha spans; only the rounded join needs pixel coverage. */
        int span=width-17;if(span<0)span=0;
        if(horizontal>0)shadow_strip(c,x,y,span,ramp,contact,horizontal,0);
        int edge_start=right-16;if(edge_start<x)edge_start=x;
        for(int px=edge_start;px<=right+1;px++) {
            float distance;
            bool corner=px>right-radius&&y<top+radius;
            if(corner) {
                float dx=(float)(px-(right-radius)),dy=(float)(y-(top+radius));
                distance=radius-sqrtf(dx*dx+dy*dy);
            } else distance=fminf((float)(y-top),(float)(right-px));
            if(distance<0) {
                if(distance>=-1.5f) {
                    float coverage=1-fabsf(distance+.5f);
                    unsigned alpha=(unsigned)(96*fmaxf(0,coverage)*fade);
                    if(alpha)fill_rect(c,px,y,1,1,(alpha<<24)|0xffffff);
                }
                continue;
            }
            float alpha=corner?shadow_coverage(distance):
                fmaxf(horizontal,shadow_coverage((float)(right-px))*fade);
            if(alpha>0)fill_rect(c,px,y,1,1,(unsigned)alpha<<24);
        }
    }
}
/* Reflect the existing shadow's solid spans instead of fitting another
   profile: upper/lower strength, penumbra and rounded bevel stay identical. */
typedef struct {Canvas *canvas;int edge;} UpperShadow;
static void upper_shadow_span(void *context,DrawOp op) {
    UpperShadow *upper=context;
    op.y=upper->edge-op.y-op.h;
    if(upper->canvas->ops)canvas_emit(upper->canvas,op);
    else fill_rect(upper->canvas,op.x,op.y,op.w,op.h,op.color);
}
void theme_menu_upper_shadow(Canvas *c,int x,int first,int width,int selected_top,bool rounded) {
    if(width<=0||selected_top<=first)return;
    UpperShadow upper={c,selected_top};
    static const CanvasOps ops={upper_shadow_span};
    Canvas reflected={.ops=&ops,.context=&upper};
    theme_menu_shadow(&reflected,x,0,width,selected_top-first,rounded);
}
void theme_frame(Canvas *c,int b,float depth,float header_y,float footer_y) {
    if(skin_frame(c,b,depth,header_y,footer_y))return;
    uint32_t base=palette[b];
    int left=(int)(theme_left(b)*(1-depth)+SUBMENU_LEFT*depth);
    int right=(int)(theme_right(b)*(1-depth)+SUBMENU_RIGHT*depth);
    int header=(int)header_y,footer=(int)footer_y;
    for(int y=0;y<header;y++) {
        int corner=y-(header-30);
        int inset=corner>0?(int)(30-sqrtf(fmaxf(0,900-corner*corner))):0;
        uint32_t top=theme_mix(base,0xff10120f,0.6f);
        uint32_t bottom=theme_mix(base,0xff080c08,0.3f);
        uint32_t row=theme_mix(top,bottom,(float)y/header);
        fill_rect(c,left+inset,y,right-left+12-2*inset,1,(row&0xffffff)|0x5f000000);
    }
    theme_round(c,left,footer,right-left+12,HEIGHT-footer+40,35,0x35000000);
    theme_outline(c,left,footer,right-left+12,HEIGHT-footer+40,35,0x25000000);
}
void theme_background(Canvas *c,int blade,float time) {
    ambient_draw(c,blade,time);
}
static void chrome(Canvas *c,int base,int b,bool active,bool label) {
    static uint32_t metal[60];
    if(!metal[0])for(int i=0;i<60;i++) {
        float t=(float)i/59;
        float shine=sinf(t*3.14159f);
        uint32_t col=theme_mix(0xff656977,0xffdedee8,shine);
        if(i<4)col=theme_mix(0xff131923,0xff777a89,t*15);
        if(i>55)col=theme_mix(0xffe8e6ed,0xff252937,(i-55)/4.0f);
        metal[i]=col;
    }
    for(int y=0;y<HEIGHT;y++) {
        float f=(float)y/HEIGHT;
        float waist=f>0.67f?powf(sinf((f-0.67f)*3.14159f/0.33f),2)*27:0;
        float neck_t=(y-86)/32.0f;
        if(neck_t<0)neck_t=0;
        if(neck_t>1)neck_t=1;
        int neck=(int)(22*(1-neck_t*neck_t*(3-2*neck_t)));
        int x=base+(int)(10*sinf(f*3.14159f))+neck+(int)waist;
        for(int i=0;i<60;i++) {
            int px=x+i;
            if(px<0||px>=WIDTH)continue;
            uint32_t col=metal[i];
            if(active&&y>=100&&i>5&&i<55)col=theme_mix(col,palette[b],0.48f);
            fill_rect(c,px,y,1,1,col);
        }
    }
    if(!label)return;
    static const char *const names[BLADE_COUNT]={"marketplace","xbox live","games","media","system"};
    text_down(c,base+47,138,30,0xffe5e6e9,names[b]);
    text_down(c,base+46,137,30,0xff242836,names[b]);
}
float theme_rail_position(int blade,int rail) {
    int left=theme_left(blade),right=theme_right(blade);
    return (float)(rail<=blade?left-(blade-rail+1)*53:right+(rail-blade-1)*53);
}
static void wings(Canvas *c,int left,int right) {
    for(int x=0;x<left+30;x++)
        fill_rect(c,x,0,1,HEIGHT,theme_mix(0xff454b55,0xffd5d8e2,0.6f+0.23f*sinf(x*0.046f)));
    for(int x=right;x<WIDTH;x++)
        fill_rect(c,x,0,1,HEIGHT,theme_mix(0xff454b55,0xffd5d8e2,0.6f+0.23f*sinf(x*0.046f)));
}
void theme_leaf(Canvas *c,int from,int to,float progress) {
    if(skin_leaf(c,from,to,progress))return;
    float t=anim_clamp(progress);
    int moving=to>from?to:from;
    int left=(int)(theme_rail_position(from,0)*(1-t)+theme_rail_position(to,0)*t);
    int right=moving!=BLADE_SYSTEM?(int)(theme_rail_position(from,BLADE_SYSTEM)*(1-t)+theme_rail_position(to,BLADE_SYSTEM)*t):(theme_right(BLADE_SYSTEM)+12);
    wings(c,left,right);
    for(int i=0;i<BLADE_COUNT;i++)if(i!=moving) {
        int pos=(int)(theme_rail_position(from,i)*(1-t)+theme_rail_position(to,i)*t);
        chrome(c,pos,i,i==to,true);
    }
    int pos=(int)(theme_rail_position(from,moving)*(1-t)+theme_rail_position(to,moving)*t);
    chrome(c,pos,moving,true,true);
    if(from==BLADE_SYSTEM||to==BLADE_SYSTEM)chrome(c,theme_right(BLADE_SYSTEM)+12,BLADE_SYSTEM,false,false);
}
void theme_chrome(Canvas *c,int blade,float position) {
    (void)position;
    theme_leaf(c,blade,blade,1);
}
void theme_submenu_chrome(Canvas *c,int blade,float depth,bool closing) {
    if(skin_chrome(c,blade,depth,closing))return;
    if(depth<=0){theme_chrome(c,blade,(float)blade);return;}
    float t=anim_clamp(depth);
    int left=(int)(theme_rail_position(blade,0)*(1-t)+38*t);
    int right=(int)((blade<BLADE_SYSTEM?theme_rail_position(blade,BLADE_SYSTEM):theme_right(BLADE_SYSTEM)+12)*(1-t)+SUBMENU_RIGHT*t);
    wings(c,left,right);
    for(int i=0;i<BLADE_COUNT;i++) {
        float destination=i<blade?-80-(blade-i)*60:i>blade?1340+(i-blade)*60:38;
        float retraction=(i<blade||i>blade+1)?anim_clamp(t/(closing?1.0f/3:.20f)):t;
        int pos=(int)(theme_rail_position(blade,i)*(1-retraction)+destination*retraction);
        chrome(c,pos,i,i==blade,true);
    }
    if(blade==BLADE_SYSTEM||t>0) {
        int edge=blade==BLADE_SYSTEM?(int)((theme_right(BLADE_SYSTEM)+12)*(1-t)+SUBMENU_RIGHT*t):SUBMENU_RIGHT;
        chrome(c,edge,blade,false,false);
    }
}
/* Small original vector icons, coloured like the launch menu's embossed silver glyphs. */
void theme_icon(Canvas *c,int x,int y,int kind,uint32_t color) {
    uint32_t shade=0xff697a82;
    switch(kind%12) {
    case 0: /* envelope */
        fill_rect(c,x,y+5,33,24,shade);fill_rect(c,x+1,y+4,31,22,color);
        line(c,x+1,y+5,x+16,y+16,shade);line(c,x+16,y+16,x+31,y+5,shade);break;
    case 1: /* person */
        circle(c,x+16,y+7,7,shade);circle(c,x+15,y+6,6,color);
        theme_round(c,x+5,y+16,24,20,5,color);break;
    case 2: /* trophy */
        ellipse(c,x+17,y+12,12,11,shade);ellipse(c,x+16,y+10,11,10,color);
        ellipse(c,x+3,y+7,5,7,color);ellipse(c,x+29,y+7,5,7,color);
        fill_rect(c,x+13,y+18,6,10,color);fill_rect(c,x+8,y+28,17,4,color);break;
    case 3: /* controller */
        theme_round(c,x,y+9,35,22,8,shade);theme_round(c,x,y+7,34,22,7,color);
        fill_rect(c,x+7,y+12,3,11,shade);fill_rect(c,x+3,y+16,11,3,shade);
        circle(c,x+26,y+13,2,shade);circle(c,x+29,y+20,2,shade);break;
    case 4: /* disc */
        circle(c,x+17,y+17,17,shade);circle(c,x+16,y+16,16,color);
        circle(c,x+16,y+16,5,shade);circle(c,x+16,y+16,2,0xffd2e0df);break;
    case 5: /* monitor */
        theme_round(c,x,y,35,25,3,color);fill_rect(c,x+3,y+3,29,18,shade);
        fill_rect(c,x+13,y+25,8,6,color);fill_rect(c,x+5,y+31,24,3,color);break;
    case 6: /* music */
        circle(c,x+12,y+25,7,color);fill_rect(c,x+16,y+3,4,23,color);
        fill_rect(c,x+19,y+3,12,6,color);break;
    case 7: /* camera */
        theme_round(c,x,y+7,35,22,3,color);fill_rect(c,x+5,y+3,11,5,color);
        circle(c,x+19,y+18,9,shade);circle(c,x+18,y+17,6,color);break;
    case 8: /* storage */
        theme_round(c,x,y+7,36,23,4,shade);theme_round(c,x,y+4,35,22,4,color);
        line(c,x+4,y+19,x+29,y+19,shade);break;
    case 10: /* arcade joystick */
        ellipse(c,x+17,y+29,16,5,shade);ellipse(c,x+16,y+27,16,5,color);
        for(int j=0;j<20;j++)fill_rect(c,x+12-j/5,y+7+j,5,1,color);
        circle(c,x+9,y+6,6,shade);circle(c,x+8,y+5,5,color);break;
    case 11: /* demos globe */
        circle(c,x+17,y+17,17,shade);circle(c,x+16,y+16,16,color);
        ellipse(c,x+16,y+16,8,15,shade);ellipse(c,x+16,y+16,6,14,color);
        line(c,x+3,y+10,x+29,y+10,shade);line(c,x+1,y+17,x+31,y+17,shade);
        line(c,x+4,y+24,x+28,y+24,shade);break;
    default: /* console */
        theme_round(c,x+9,y,16,36,3,color);circle(c,x+17,y+24,3,shade);
        fill_rect(c,x+14,y+5,6,1,shade);break;
    }
}
