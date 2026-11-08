#include "draw_list.h"
#include "resources.h"
#include "ambient.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

unsigned canvas_image_id(void) {static unsigned id;return ++id;}
void canvas_emit(Canvas *c,DrawOp op) {c->ops->append(c->context,op);}
void canvas_image_tagged(Canvas *c,DrawImage image,int x,int y,int w,int h) {
    canvas_image_tagged_opacity(c,image,x,y,w,h,255);
}
void canvas_image_tagged_opacity(Canvas *c,DrawImage image,int x,int y,int w,int h,unsigned opacity) {
    if(opacity>255)opacity=255;
    if(c->ops)canvas_emit(c,(DrawOp){.kind=DRAW_QUAD,.image=image,.x=x,.y=y,.w=w,.h=h,
        .sw=image.width,.sh=image.height,.color=(opacity<<24)|0xffffff});
    else if(image_yuv(image.format))canvas_yuv420(c,image,x,y,w,h);
    else canvas_image_opacity(c,image.pixels,image.width,image.height,x,y,w,h,opacity);
}
void canvas_mask(Canvas *c,DrawImage image,int sx,int sy,int side,int x,int y,uint32_t color,bool rotate) {
    canvas_emit(c,(DrawOp){.kind=DRAW_QUAD,.image=image,.sx=sx,.sy=sy,.sw=side,.sh=side,
        .x=x,.y=y,.w=side,.h=side,.color=color,.rotate=rotate});
}
static void append(void *context,DrawOp op) {
    DrawList *list=context;if(list->failed)return;
    if(op.kind==DRAW_QUAD&&(op.w<=0||op.h<=0||op.x>=WIDTH||op.y>=HEIGHT||
        op.x+op.w<=0||op.y+op.h<=0||!(op.color>>24)))return;
    if(op.kind==DRAW_AMBIENT&&op.w<=0)return;
    if(list->count==list->capacity) {
        size_t capacity=list->capacity?list->capacity*2:2048;
        if(capacity>262144){list->failed=true;return;}
        void *p=realloc(list->ops,capacity*sizeof *list->ops);
        if(!p){list->failed=true;return;}
        list->ops=p;list->capacity=capacity;
    }
    list->ops[list->count++]=op;
}
static bool same(const Dashboard *a,const Dashboard *b) {
    if(a->video_view!=b->video_view)return false;
    if(a->video_view)return a->video_controls==b->video_controls&&a->video_stretch==b->video_stretch&&
        a->video.network==b->video.network&&
        a->video_has_picture==b->video_has_picture&&a->video.state==b->video.state&&
        a->video_picture.revision==b->video_picture.revision&&a->video_picture.rgba==b->video_picture.rgba&&
        (int)a->video.position==(int)b->video.position&&a->video.duration==b->video.duration;

    bool reading=a->disc.state==DISC_READING&&!a->depth&&blade_has_tray(a->blade)&&
        a->selected[a->blade]==menu_count(a->blade,0,0)-1;
    return a->blade==b->blade&&a->previous_blade==b->previous_blade&&a->previous_depth==b->previous_depth&&
        (!dashboard_ftp_page(a)||!memcmp(&a->ftp,&b->ftp,sizeof a->ftp))&&
        (!dashboard_update_page(a)||a->update.revision==b->update.revision)&&
        !memcmp(a->selected,b->selected,sizeof a->selected)&&a->depth==b->depth&&a->child==b->child&&
        a->transition==b->transition&&a->menu_transition==b->menu_transition&&lroundf(a->highlight*53)==lroundf(b->highlight*53)&&
        a->dialog==b->dialog&&a->exit_menu==b->exit_menu&&a->message==b->message&&
        a->achievement_revision==b->achievement_revision&&a->achievement_page==b->achievement_page&&a->previous_achievement_page==b->previous_achievement_page&&
        a->achievement_grid==b->achievement_grid&&a->achievement_game==b->achievement_game&&a->achievement_index==b->achievement_index&&a->achievement_scroll==b->achievement_scroll&&
        a->profile_revision==b->profile_revision&&a->profile_card_selected==b->profile_card_selected&&
        a->library_revision==b->library_revision&&a->library_page==b->library_page&&a->previous_library_page==b->previous_library_page&&a->library_filter==b->library_filter&&
        a->library_row==b->library_row&&a->library_scroll==b->library_scroll&&a->library_wait==b->library_wait&&a->list_transition==b->list_transition&&
        a->library_depth_edit==b->library_depth_edit&&
        a->library_scan_details==b->library_scan_details&&
        a->jelly_revision==b->jelly_revision&&a->jelly_page==b->jelly_page&&a->previous_jelly_page==b->previous_jelly_page&&
        a->jelly_row==b->jelly_row&&a->jelly_scroll==b->jelly_scroll&&a->jelly_wait==b->jelly_wait&&a->jelly_root_pending==b->jelly_root_pending&&
        a->library_root==b->library_root&&!memcmp(a->library_path,b->library_path,sizeof a->library_path)&&
        a->disc.state==b->disc.state&&a->disc.title_id==b->disc.title_id&&a->disc.media==b->disc.media&&
        !memcmp(a->disc.title,b->disc.title,sizeof a->disc.title)&&
        (!reading||disc_reading_frame(a->disc_elapsed)==disc_reading_frame(b->disc_elapsed))&&
        a->startup_active==b->startup_active&&(!a->startup_active||a->startup_elapsed==b->startup_elapsed);
}
bool draw_list_update(DrawList *list,const Dashboard *d) {
    unsigned generation=resources_generation();
    if(list->valid&&list->generation==generation&&same(&list->state,d))return false;
    list->count=0;list->failed=false;list->valid=false;
    static const CanvasOps ops={append};Canvas canvas={.ops=&ops,.context=list};
    dashboard_render(&canvas,d);
    list->state=*d;list->generation=generation;list->builds++;
    list->valid=!list->failed;return true;
}
void draw_list_close(DrawList *list) {free(list->ops);memset(list,0,sizeof *list);}
static uint32_t blend(uint32_t a,uint32_t b,unsigned t) {
    uint32_t v=0xff000000;
    for(unsigned s=0;s<=16;s+=8)v|=((((a>>s)&255)*(255-t)+((b>>s)&255)*t)/255)<<s;
    return v;
}
bool draw_list_software(const DrawList *list,Canvas *c,float seconds) {
    if(!list->valid||c->ops||!c->pixels)return false;
    uint32_t *base=NULL,*background=NULL;bool ok=true;
    for(size_t n=0;n<list->count&&ok;n++) {
        const DrawOp *op=list->ops+n;
        if(op->kind==DRAW_AMBIENT) {
            if(op->x==0&&op->w==WIDTH)ambient_draw(c,(int)op->color,seconds);
            else {
                if(!background)background=malloc(WIDTH*HEIGHT*4);
                if(!background){ok=false;break;}
                Canvas next={.pixels=background};ambient_draw(&next,(int)op->color,seconds);
                for(int y=0;y<HEIGHT;y++)memcpy(c->pixels+y*WIDTH+op->x,background+y*WIDTH+op->x,(size_t)op->w*4);
            }
        } else if(op->kind==DRAW_FADE_BEGIN) {
            if(base){ok=false;break;}
            base=malloc(WIDTH*HEIGHT*4);if(!base){ok=false;break;}
            memcpy(base,c->pixels,WIDTH*HEIGHT*4);
        } else if(op->kind==DRAW_FADE_END) {
            if(!base){ok=false;break;}
            unsigned t=(unsigned)(op->amount*256),s=256-t;
            for(int i=0;i<WIDTH*HEIGHT;i++) {
                uint32_t a=base[i],b=c->pixels[i];
                uint32_t rb=(((a&0x00ff00ff)*s+(b&0x00ff00ff)*t)>>8)&0x00ff00ff;
                uint32_t g=(((a&0x0000ff00)*s+(b&0x0000ff00)*t)>>8)&0x0000ff00;
                c->pixels[i]=0xff000000|rb|g;
            }
            free(base);base=NULL;
        } else if(!op->image.pixels)fill_rect(c,op->x,op->y,op->w,op->h,op->color);
        else if(op->image.format==IMAGE_RGBA)canvas_image_opacity(c,op->image.pixels,op->image.width,op->image.height,
            op->x,op->y,op->w,op->h,op->color>>24);
        else if(image_yuv(op->image.format))canvas_yuv420(c,op->image,op->x,op->y,op->w,op->h);
        else for(int dy=0;dy<op->h;dy++)for(int dx=0;dx<op->w;dx++) {
            int x=op->x+dx,y=op->y+dy;if(x<0||x>=WIDTH||y<0||y>=HEIGHT)continue;
            int sx=op->rotate?dy:dx,sy=op->rotate?op->sw-1-dx:dy;
            unsigned alpha=op->image.pixels[(op->sy+sy)*op->image.width+op->sx+sx]*(op->color>>24)/255;
            if(alpha)c->pixels[y*WIDTH+x]=blend(c->pixels[y*WIDTH+x],op->color,alpha);
        }
    }
    if(base)ok=false;
    free(base);free(background);return ok;
}
