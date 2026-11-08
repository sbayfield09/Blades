#include "blades.h"
#include "resources.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void press(Dashboard *d,unsigned a) {dashboard_update(d,a,1.0f/60);}
int main(int argc,char **argv) {
    if(argc==2)assert(resources_open(argv[1]));
    for(unsigned n=0;n<1600;n++) {
        float sample=selection_sample(SELECT,n);
        assert(isfinite(sample)&&fabsf(sample)<=0.0651f);
        if(n>=1536)assert(sample==0);
    }
    Input i={0};
    assert(!input_update(&i,SELECT,0.016f,true));
    assert(!input_update(&i,0,0.016f,true));
    assert(input_update(&i,SELECT,0.016f,true)==SELECT);
    for(int n=0;n<60;n++)assert(!(input_update(&i,SELECT,0.016f,true)&SELECT));
    input_update(&i,0,0.01f,true);
    assert(input_update(&i,DOWN,0.016f,true)==DOWN);
    assert(!input_update(&i,DOWN,0.1f,true));
    assert(!input_update(&i,DOWN,0.1f,true));
    assert(!input_update(&i,DOWN,0.1f,true));
    assert(input_update(&i,DOWN,0.1f,true)==DOWN);
    input_update(&i,0,0.01f,false);
    assert(!input_update(&i,SELECT,0.01f,true));
    Dashboard d; dashboard_init(&d);
    assert(d.blade==BLADE_GAMES);
    while(d.blade>BLADE_MARKETPLACE)press(&d,LEFT);
    press(&d,LEFT);assert(d.blade==BLADE_MARKETPLACE);
    for(int b=0;b<BLADE_COUNT;b++) {
        assert(d.blade==b);
        int count=menu_count(b,0,0);
        press(&d,UP);
        if(blade_has_profile(b)){assert(d.profile_card_selected);press(&d,UP);}
        assert(d.selected[b]==count-1);
        press(&d,DOWN);
        if(blade_has_profile(b)){assert(d.profile_card_selected);press(&d,DOWN);}
        assert(d.selected[b]==0);
        for(int r=0;r<count;r++) {
            if(b==BLADE_GAMES&&r==0){
                press(&d,SELECT);assert(d.profile_command.type==PROFILE_SIGN_IN&&!d.depth);
                ProfileSnapshot p={.identity={.state=PROFILE_LOCAL,.xuid=42},.revision=1};dashboard_set_profile(&d,&p);press(&d,0);assert(d.achievement_page&&d.depth==1);
                press(&d,BACK);assert(!d.achievement_page&&!d.depth);dashboard_set_profile(&d,NULL);press(&d,DOWN);continue;
            }
            if(blade_has_tray(b)&&r==count-1) {
                press(&d,SELECT);assert(!d.depth&&d.disc_command==DISC_EJECT);
                press(&d,DOWN);continue;
            }
            press(&d,SELECT);assert(d.depth==1);
            if(b==BLADE_MEDIA&&r<=3){assert(d.library_page==(r==3?LIB_UI_ROOTS:r==2?LIB_UI_VIDEO_SOURCES:LIB_UI_LIST));press(&d,BACK);assert(!d.depth&&!d.library_page);press(&d,DOWN);continue;}
            if(b==BLADE_MEDIA&&r==4){assert(d.jelly_page==JELLY_UI_SETTINGS);press(&d,SELECT);assert(d.jelly_edit==1&&!d.dialog);press(&d,BACK);assert(!d.depth&&!d.jelly_page);press(&d,DOWN);continue;}
            if(b==BLADE_SYSTEM&&r==4){press(&d,SELECT);assert(d.ftp_toggle&&!d.dialog);press(&d,BACK);assert(!d.depth);press(&d,DOWN);continue;}
            if(b==BLADE_SYSTEM&&r==6){assert(d.update_check&&!d.dialog);press(&d,BACK);assert(!d.depth);press(&d,DOWN);continue;}
            if(b==BLADE_GAMES&&(r==1||r==2||r==3)) {
                if(r==1){press(&d,SELECT);assert(d.library_page==LIB_UI_LIST);press(&d,BACK);}
                press(&d,BACK);assert(!d.depth);press(&d,DOWN);continue;
            }
            int children=menu_count(b,1,r);assert(children>0);
            for(int j=0;j<children;j++) {
                assert(menu_label(b,1,r,j));
                {
                    press(&d,SELECT);assert(d.dialog);
                    press(&d,BACK);assert(!d.dialog&&d.depth==1);
                }
                press(&d,DOWN);
            }
            press(&d,BACK);assert(!d.depth);press(&d,DOWN);
        }
        if(b<BLADE_COUNT-1)press(&d,RIGHT);
    }
    press(&d,RIGHT);assert(d.blade==BLADE_SYSTEM);
    for(int n=0;n<30;n++)dashboard_update(&d,0,0.016f);
    assert(fabsf(d.position-BLADE_SYSTEM)<0.001f);
    press(&d,EXIT);assert(d.exit_menu&&!d.exit_requested&&d.child==0);
    press(&d,SELECT);assert(!d.exit_menu&&!d.exit_requested&&!d.depth);
    press(&d,EXIT);press(&d,DOWN);press(&d,SELECT);assert(d.exit_requested);
    dashboard_init(&d);
    assert(BLADE_COUNT==5);
    const int expected_counts[BLADE_COUNT]={3,4,5,6,7};
    const char *expected_names[BLADE_COUNT]={"Marketplace","Xbox Live","Games","Media","System"};
    for(int b=0;b<BLADE_COUNT;b++) {
        assert(menu_count(b,0,0)==expected_counts[b]);
        assert(strcmp(blade_names[b],expected_names[b])==0);
    }
    assert(strcmp(menu_label(BLADE_LIVE,0,0,2),"Chat and IM")==0);
    assert(strcmp(menu_label(BLADE_GAMES,0,0,3),"Demos and More")==0);
    assert(strcmp(menu_label(BLADE_MEDIA,0,0,3),"Video Marketplace")==0);
    press(&d,PROFILE);assert(!d.dialog&&d.profile_command.type==PROFILE_SIGN_IN);
    /* Guard the raster buffer and exercise every blade and a clipped transition. */
    uint32_t *mem=calloc(WIDTH*HEIGHT+2,4);assert(mem);mem[0]=0xabcdef;mem[WIDTH*HEIGHT+1]=0x123456;
    Canvas c={.pixels=mem+1};
    for(int b=0;b<BLADE_COUNT;b++) {
        dashboard_init(&d);d.blade=b;d.position=(float)b+0.7f;d.target=(float)b;
        dashboard_render(&c,&d);
        assert(c.pixels[100*WIDTH+640]!=c.pixels[665*WIDTH+640]);
        press(&d,SELECT);dashboard_render(&c,&d);
        press(&d,SELECT);dashboard_render(&c,&d);
        assert(mem[0]==0xabcdef&&mem[WIDTH*HEIGHT+1]==0x123456);
    }
    /* Exercise every neighbouring blade in both directions, including the moving
       System rail, and both expanded-submenu paths at intermediate opacities. */
    for(int from=0;from<BLADE_COUNT;from++)for(int direction=-1;direction<=1;direction+=2) {
        int to=from+direction;if(to<0||to>=BLADE_COUNT)continue;
        dashboard_init(&d);d.blade=from;d.previous_blade=from;
        d.position=(float)from;d.target=(float)from;
        press(&d,direction>0?RIGHT:LEFT);
        assert(d.previous_blade==from&&d.blade==to);
        for(int n=0;n<40;n++) {
            dashboard_update(&d,0,0.016f);dashboard_render(&c,&d);
            assert(mem[0]==0xabcdef&&mem[WIDTH*HEIGHT+1]==0x123456);
        }
        assert(d.transition==1);
        if(to==BLADE_GAMES){d.selected[BLADE_GAMES]=1;d.highlight=1;}
        for(int depth=1;depth>=0;depth--) {
            press(&d,depth?SELECT:BACK);
            assert(d.depth==depth);
            for(int n=0;n<65;n++) {
                dashboard_update(&d,0,0.016f);dashboard_render(&c,&d);
                assert(mem[0]==0xabcdef&&mem[WIDTH*HEIGHT+1]==0x123456);
            }
            assert(d.menu_transition==1);
        }
    }
    /* Profile shortcut cannot create a hidden dialog underneath exit confirmation. */
    dashboard_init(&d);press(&d,EXIT);press(&d,PROFILE);
    assert(d.exit_menu&&!d.dialog);press(&d,BACK);assert(!d.exit_menu);
    assert(!resources_open("/definitely/missing/blades/shrdres.xzp"));
    resources_close();dashboard_render(&c,&d);
    assert(mem[0]==0xabcdef&&mem[WIDTH*HEIGHT+1]==0x123456);
    free(mem);puts("PASS: input edges/repeat/reconnect, all menus, dialogs, exit confirmation, transitions, render bounds");return 0;
}
