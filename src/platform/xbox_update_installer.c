/* Separate executable: default.xex is unloaded before any installed file moves. */
#include "update_install.h"
#include "font.h"
#include "xsdk/kernel.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
static void screen(const char *title,const char *message){
    static uint32_t w,h;
    if(!w&&(!NT_SUCCESS(xsdk_gpu_open())||!NT_SUCCESS(xsdk_present_open(XSDK_DISPLAY_720P,2,1,&w,&h))))return;
    uint32_t *pixels=malloc((size_t)w*h*4);if(!pixels)return;
    for(unsigned i=0;i<w*h;i++)pixels[i]=0x18222dff;
    const char *lines[]={title,message,"Keep the Blades folder and update backup intact."};
    for(unsigned line=0;line<3;line++){
        unsigned x=70,y=180+line*85;
        for(const unsigned char *p=(const unsigned char *)lines[line];*p&&x+64<w;p++){
            unsigned glyph=*p>=32&&*p<127?*p-32:'?'-32;
            for(unsigned py=0;py<32&&y+py<h;py++)for(unsigned px=0;px<32;px++){
                unsigned a=font_pixels[glyph*FONT_CELL*FONT_CELL+py*2*FONT_CELL+px*2];
                if(a>60)pixels[(y+py)*w+x+px]=0xffffffff;
            }x+=(font_advance[glyph]+1)/2;
        }
    }
    for(unsigned i=0;i<2;i++){
        uint32_t address;if(!NT_SUCCESS(xsdk_present_begin(&address,1000)))break;
        XsdkTexture frame;if(NT_SUCCESS(xsdk_texture_init(&frame,XSDK_FORMAT_B8G8R8A8,w,h,1,true))){xsdk_texture_attach(&frame,xsdk_present_pixels(address));xsdk_texture_upload(&frame,0,pixels,w*4);__asm__ volatile("sync" ::: "memory");xsdk_present_end();}
    }free(pixels);
}
int main(void){
    char root[512],native_root[512],target[768],error[256];const char *image=ExLoadedImageName;
    if(!image||strlen(image)>=sizeof root)return 1;
    strcpy(root,image);
    for(char *p=root;*p;p++)if(*p=='\\')*p='/';
    char *name=strrchr(root,'/');if(!name||strcasecmp(name+1,"installer.xex"))return 1;*name=0;
    name=strrchr(root,'/');if(!name||strcasecmp(name+1,UPDATE_INSTALL_WORK))return 1;*name=0;
    /* A private drive alias avoids treating NT object directories as FATX folders. */
    for(char *p=root;*p;p++)if(*p=='/')*p='\\';
    strcpy(native_root,root);
    const char *link_text="\\??\\bladesupd:";
    ANSI_STRING link={.length=(uint16_t)strlen(link_text),.maximum_length=(uint16_t)(strlen(link_text)+1),.pointer=link_text};
    ANSI_STRING target_root={.length=(uint16_t)strlen(root),.maximum_length=(uint16_t)(strlen(root)+1),.pointer=root};
    (void)ObDeleteSymbolicLink(&link);
    if(!NT_SUCCESS(ObCreateSymbolicLink(&link,&target_root)))return 1;
    strcpy(root,"bladesupd:");
    xsdk_log_write("update-installer: applying/recovering local staged transaction");
    screen("Updating Blades","Please keep your storage device connected.");
    int result=update_install_apply(root,error,sizeof error);
    xsdk_log_write("update-installer: result=%d %s",result,error);
    if(result<0&&update_install_pending(root)){
        screen("Blades update needs recovery",error);
        for(;;){struct timespec pause={1,0};nanosleep(&pause,NULL);}
    }
    if(result<=0){
        if(update_install_path(target,sizeof target,root,UPDATE_INSTALL_WORK,"result.txt")){
            FILE *f=fopen(target,"wb");if(f){fprintf(f,"%s",error);fclose(f);}
        }
    }
    if(update_install_path(target,sizeof target,native_root,"","default.xex"))xsdk_launch(target,NULL,0);
    screen("Blades could not restart","Launch default.xex from your dashboard. Check the installer log.");
    for(;;){struct timespec pause={1,0};nanosleep(&pause,NULL);}
}
