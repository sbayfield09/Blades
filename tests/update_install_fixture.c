#define _DEFAULT_SOURCE
#include "update_install.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned point,step;static bool crash,cancelled,recover_crash;
bool update_install_test_point(unsigned phase){if(phase>UPDATE_INSTALL_FILES){if(recover_crash&&phase-UPDATE_INSTALL_FILES==point)_exit(86);return true;}if(++step==point){if(crash)_exit(86);return false;}return true;}
static bool cancel(void *unused){(void)unused;return cancelled;}
static void progress(void *unused,unsigned done,unsigned total){(void)unused;assert(done<=total);if(point&&done==point)cancelled=true;}
int main(int argc,char **argv){
    assert(argc>=3);const char *root=argv[1],*mode=argv[2];char error[256];
    if(!strcmp(mode,"stage")||!strcmp(mode,"cancel")||!strcmp(mode,"reject")||!strcmp(mode,"fuzz")){
        assert(argc==4);if(!strcmp(mode,"cancel"))point=2;
        bool ok=update_install_stage(root,argv[3],cancel,progress,NULL,error,sizeof error);
        if(!strcmp(mode,"stage")){if(!ok)fprintf(stderr,"%s\n",error);assert(ok&&update_install_pending(root));}
        else if(strcmp(mode,"fuzz"))assert(!ok&&!update_install_pending(root));
    }else{
        if(!strncmp(mode,"fail-",5))point=(unsigned)atoi(mode+5);
        if(!strncmp(mode,"crash-",6)){point=(unsigned)atoi(mode+6);crash=true;}
        if(!strncmp(mode,"recover-crash-",14)){point=(unsigned)atoi(mode+14);recover_crash=true;}
        int result=update_install_apply(root,error,sizeof error);
        if(!strcmp(mode,"apply")){if(result!=1)fprintf(stderr,"%s\n",error);assert(result==1&&!update_install_pending(root));}
        else if(!strcmp(mode,"recover")){if(result!=0)fprintf(stderr,"%s\n",error);assert(result==0&&!update_install_pending(root));}
        else if(!strcmp(mode,"retain-journal"))assert(result<0&&update_install_pending(root));
        else assert(result<0&&!update_install_pending(root));
    }
    printf("PASS: installer %s\n",mode);
}
