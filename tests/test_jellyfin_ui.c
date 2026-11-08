#include "blades.h"
#include "draw_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void press(Dashboard *d,unsigned a){dashboard_update(d,a,.016f);}
static void browser_pages(void){
 xf_browser b;xf_browser_init(&b);char json[4096],path[1501];unsigned used=(unsigned)snprintf(json,sizeof json,"{\"Items\":[");
 for(unsigned i=0;i<12;i++)used+=(unsigned)snprintf(json+used,sizeof json-used,"%s{\"Id\":\"%032x\",\"Name\":\"Library %u\",\"Type\":\"CollectionFolder\",\"IsFolder\":true,\"CollectionType\":\"%s\"}",i?",":"",i+20,i,i==11?"music":"movies");
 used+=(unsigned)snprintf(json+used,sizeof json-used,"]}");
 xf_response *r=calloc(1,sizeof *r);assert(r);r->status=200;r->complete=1;memcpy(r->body,json,used);r->body_size=used;
 const char *user="11111111111111111111111111111111";
 assert(xf_browser_request(&b,XF_ROOT,user,path,sizeof path)==1);assert(xf_browser_result(&b,r,NULL)==1);
 assert(b.page.count==10&&b.page.total==13&&!strcmp(b.page.items[0].name,"Continue Watching")&&!strcmp(b.page.items[1].name,"Next Up"));
 assert(xf_browser_request(&b,XF_NEXT,user,path,sizeof path)==1);assert(xf_browser_result(&b,r,NULL)==1);assert(b.current.start==10&&b.page.count==3&&b.page.total==13);
 assert(xf_browser_request(&b,XF_PREVIOUS,user,path,sizeof path)==1);assert(xf_browser_result(&b,r,NULL)==1);assert(!b.current.start);
 b.current.selected=0;assert(xf_browser_request(&b,XF_OPEN,user,path,sizeof path)==1&&strstr(path,"/Items/Resume?"));
 strcpy(r->body,"{\"Items\":[],\"TotalRecordCount\":0,\"StartIndex\":0}");r->body_size=strlen(r->body);assert(xf_browser_result(&b,r,NULL)==1&&b.depth==1&&!b.page.count&&b.current.view==XF_RESUME);
 assert(xf_browser_request(&b,XF_REFRESH,user,path,sizeof path)==1&&strstr(path,"/Items/Resume?"));assert(xf_browser_result(&b,r,NULL)==1);
 assert(xf_browser_request(&b,XF_BACK,user,path,sizeof path)==1);memcpy(r->body,json,used);r->body_size=used;assert(xf_browser_result(&b,r,NULL)==1&&!b.depth);
 b.current.selected=1;assert(xf_browser_request(&b,XF_OPEN,user,path,sizeof path)==1&&strstr(path,"/Shows/NextUp?"));
 /* Invalid next-up JSON preserves the root, without inserting history. */
 strcpy(r->body,"{}");r->body_size=2;assert(xf_browser_result(&b,r,NULL)==-1&&!b.depth&&b.page.count==10);
 free(r);
}
static void episode_labels(void){
 xf_item item={0};strcpy(item.type,"Episode");strcpy(item.series,"Test Series");strcpy(item.name,"Episode Title");char label[JELLY_LABEL_BYTES];
 item.season=1;item.index=2;
 jellyfin_item_label(&item,XF_RESUME,label,sizeof label);assert(!strcmp(label,"Test Series S1E2 - Episode Title"));
 jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(!strcmp(label,"Test Series S1E2 - Episode Title"));
 jellyfin_item_label(&item,XF_CHILDREN,label,sizeof label);assert(!strcmp(label,"Episode Title"));
 strcpy(item.name,"Test Series - Episode Title");jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(!strcmp(label,"Test Series S1E2 - Episode Title"));
 strcpy(item.name,"Test Series S01E02 - Episode Title");jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(!strcmp(label,"Test Series S1E2 - Episode Title"));
 strcpy(item.name,"S1E2 - Episode Title");jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(!strcmp(label,"Test Series S1E2 - Episode Title"));
 strcpy(item.name,"Episode Title");item.series[0]=0;jellyfin_item_label(&item,XF_RESUME,label,sizeof label);assert(!strcmp(label,item.name));
 strcpy(item.series,"Test Series");strcpy(item.type,"Movie");jellyfin_item_label(&item,XF_RESUME,label,sizeof label);assert(!strcmp(label,item.name));
 strcpy(item.type,"Episode");item.season=0;item.has_season=1;jellyfin_item_label(&item,XF_RESUME,label,sizeof label);assert(!strcmp(label,"Test Series S0E2 - Episode Title"));
 item.has_season=0;jellyfin_item_label(&item,XF_RESUME,label,sizeof label);assert(!strcmp(label,"Test Series - Episode Title"));
 item.season=1;item.index=0;item.has_index=1;jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(!strcmp(label,"Test Series S1E0 - Episode Title"));
 memset(item.series,'S',sizeof item.series-1);item.series[sizeof item.series-1]=0;memset(item.name,'E',sizeof item.name-1);item.name[sizeof item.name-1]=0;
 item.season=UINT32_MAX;item.index=UINT32_MAX;jellyfin_item_label(&item,XF_NEXTUP,label,sizeof label);assert(strlen(label)>330&&strlen(label)<sizeof label&&strstr(label,"S4294967295E4294967295 - E"));
 xf_page page;const char *json="{\"Items\":[{\"Id\":\"11111111111111111111111111111111\",\"Name\":\"Special\",\"Type\":\"Episode\",\"IndexNumber\":0,\"ParentIndexNumber\":0}],\"TotalRecordCount\":1}";
 assert(xf_parse_page(json,strlen(json),0,0,&page)&&page.items[0].has_index&&page.items[0].has_season);
}
static void artwork_layout(void){
 Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;d.position=d.origin=d.target=BLADE_MEDIA;d.depth=d.previous_depth=1;d.menu_transition=1;d.jelly_page=JELLY_UI_BROWSE;
 JellySnapshot *j=calloc(1,sizeof *j);assert(j);j->state=JELLY_CONNECTED;j->revision=1;j->loaded=true;j->page.count=j->page.total=1;
 xf_item *i=j->page.items;strcpy(i->id,"11111111111111111111111111111111");strcpy(i->name,"A selected film");strcpy(i->type,"Movie");strcpy(i->overview,"A description that belongs to this selected film.");i->ticks=61200000000ULL;
 strcpy(j->artwork.item,i->id);j->artwork.width=100;j->artwork.height=150;j->artwork.revision=1;
 dashboard_set_jellyfin(&d,j);DrawList list={0};assert(draw_list_update(&list,&d));bool found=false;
 for(size_t n=0;n<list.count;n++)if(list.ops[n].image.pixels==j->artwork.pixels){DrawOp *op=list.ops+n;assert(op->x>=815&&op->x+op->w<=1100&&op->y>=94&&op->y+op->h<=270);assert(abs(op->w*150-op->h*100)<=150);found=true;}
 assert(found);j->artwork.item[0]='2';j->revision++;dashboard_set_jellyfin(&d,j);assert(draw_list_update(&list,&d));
 for(size_t n=0;n<list.count;n++)assert(list.ops[n].image.pixels!=j->artwork.pixels);
 draw_list_close(&list);free(j);
}
int main(void){
 browser_pages();episode_labels();artwork_layout();
 char runtime[64];jellyfin_runtime(0,runtime,sizeof runtime);assert(!strcmp(runtime,"Length unavailable"));
 jellyfin_runtime(450000000,runtime,sizeof runtime);assert(!strcmp(runtime,"1 min"));
 jellyfin_runtime(61200000000ULL,runtime,sizeof runtime);assert(!strcmp(runtime,"1 hr 42 min"));
 jellyfin_runtime(UINT64_MAX,runtime,sizeof runtime);assert(runtime[0]);
 Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_MEDIA;d.selected[BLADE_MEDIA]=2;
 JellySnapshot j={.state=JELLY_CONNECTED,.revision=1,.loaded=true,.page={.count=1,.total=1}};strcpy(j.page.items[0].name,"Movies");j.page.items[0].folder=true;
 dashboard_set_jellyfin(&d,&j);press(&d,SELECT);assert(d.depth==1&&d.library_page==LIB_UI_VIDEO_SOURCES&&dashboard_library_count(&d)==2);
 assert(!strcmp(dashboard_library_label(&d,0),"Jellyfin")&&!strcmp(dashboard_library_label(&d,1),"Local"));
 press(&d,DOWN);press(&d,SELECT);assert(d.library_page==LIB_UI_LIST&&d.library_filter==LIB_VIDEO);press(&d,BACK);assert(d.library_page==LIB_UI_VIDEO_SOURCES&&d.library_row==1);
 press(&d,UP);press(&d,SELECT);assert(d.jelly_page==JELLY_UI_BROWSE&&d.jelly_command.type==JELLY_ROOT&&d.jelly_wait);
 j.revision++;dashboard_set_jellyfin(&d,&j);press(&d,SELECT);assert(d.jelly_command.type==JELLY_OPEN&&d.jelly_command.row==0);
 j.depth=1;j.revision++;j.page.items[0].folder=false;strcpy(j.page.items[0].type,"Episode");dashboard_set_jellyfin(&d,&j);press(&d,SELECT);assert(d.jelly_command.type==JELLY_PLAY);
 j.revision++;dashboard_set_jellyfin(&d,&j);press(&d,BACK);assert(d.jelly_command.type==JELLY_BACK&&d.jelly_page==JELLY_UI_BROWSE);
 j.depth=0;j.revision++;dashboard_set_jellyfin(&d,&j);press(&d,BACK);assert(d.library_page==LIB_UI_VIDEO_SOURCES&&!d.jelly_page);press(&d,BACK);assert(!d.depth&&!d.library_page);
 d.selected[BLADE_MEDIA]=4;press(&d,SELECT);assert(d.jelly_page==JELLY_UI_SETTINGS&&d.depth);
 for(unsigned field=1;field<=3;field++){press(&d,SELECT);assert(d.jelly_edit==field);press(&d,DOWN);}
 press(&d,SELECT);assert(d.jelly_command.type==JELLY_DISCONNECT);j.revision++;j.state=JELLY_OFF;dashboard_set_jellyfin(&d,&j);press(&d,SELECT);assert(d.jelly_command.type==JELLY_CONNECT);
 j.revision++;dashboard_set_jellyfin(&d,&j);press(&d,DOWN);press(&d,SELECT);assert(d.jelly_command.type==JELLY_REMOVE);press(&d,BACK);assert(!d.depth&&!d.jelly_page);
 d.selected[BLADE_MEDIA]=2;press(&d,SELECT);press(&d,SELECT);j.revision++;j.busy=true;dashboard_set_jellyfin(&d,&j);press(&d,BACK);assert(!d.jelly_page&&d.library_page==LIB_UI_VIDEO_SOURCES);
 press(&d,RIGHT);assert(d.blade==BLADE_SYSTEM&&!d.jelly_page&&!d.library_page);
 puts("PASS: Media Center connection controls, Jellyfin/Local folders, page/back/cancel/play routing and blade changes");
}
