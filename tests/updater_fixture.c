#define _DEFAULT_SOURCE
#include "blades.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static UpdateSnapshot settled(Updater *u){double until=now()+12;UpdateSnapshot s;do{updater_poll(u,&s);if(s.state!=UPDATE_IDLE&&s.state!=UPDATE_CHECKING&&s.state!=UPDATE_DOWNLOADING)return s;usleep(1000);}while(now()<until);fprintf(stderr,"updater timeout: %s\n",s.message);assert(0);return s;}
static void unit(void){
 assert(update_version_compare("0.51.0",BLADES_VERSION)>0&&update_version_compare("0.50",BLADES_VERSION)==0&&update_version_compare("0.49.9",BLADES_VERSION)<0&&update_version_compare("0.50.1",BLADES_VERSION)>0);
 char text[512];snprintf(text,sizeof text,"{\"schema\":1,\"version\":\"0.51.0\",\"asset\":\"Blades-v0.51-USB.zip\",\"size\":4096,\"sha256\":\"%064d\"}",0);UpdateManifest m;
 assert(update_manifest_parse(text,strlen(text),&m));assert(!update_manifest_parse("{}",2,&m));assert(!update_manifest_parse(text,strlen(text)-1,&m));
 char *p=strstr(text,"0.51.0");p[0]='v';assert(!update_manifest_parse(text,strlen(text),&m));
 Dashboard d;dashboard_init(&d);d.blade=d.previous_blade=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=6;dashboard_update(&d,SELECT,0);assert(dashboard_update_page(&d)&&d.update_check&&!d.update_download);dashboard_update(&d,0,.016);assert(!d.update_check);dashboard_update(&d,DOWN,0);dashboard_update(&d,SELECT,0);assert(d.update_download);dashboard_update(&d,PROFILE,0);assert(d.update_cancel);dashboard_update(&d,BACK,0);assert(!dashboard_update_page(&d));
}
int main(int argc,char **argv){
 unit();if(argc==1){puts("PASS: updater version/manifest validation and System button routing");return 0;}assert(argc==3);Updater *u=updater_open(argv[1],NULL,NULL);assert(u&&updater_check(u));UpdateSnapshot s=settled(u);
 const char *mode=argv[2];
 if(!strcmp(mode,"github-probe")){assert(s.state==UPDATE_ERROR&&strstr(s.message,"No published update"));updater_close(u);puts("PASS: verified HTTPS to GitHub; unpublished release correctly reported");return 0;}
 if(!strcmp(mode,"github-current")){assert(s.state==UPDATE_CURRENT&&!strcmp(s.version,BLADES_VERSION));updater_close(u);puts("PASS: live repository manifest found via raw GitHub; installed version is current");return 0;}
 if(!strcmp(mode,"current")||!strcmp(mode,"older"))assert(s.state==UPDATE_CURRENT&&!updater_download(u));
 else if(!strcmp(mode,"badmanifest")||!strcmp(mode,"badhost")||!strcmp(mode,"untrusted")||!strcmp(mode,"redirect-bad")||!strcmp(mode,"redirect-loop")||!strcmp(mode,"downgrade"))assert(s.state==UPDATE_ERROR);
 else{
  if(s.state!=UPDATE_AVAILABLE)fprintf(stderr,"check failed: %s\n",s.message);
  assert(s.state==UPDATE_AVAILABLE);
  double until=now()+1;while(!updater_download(u)){assert(now()<until);usleep(1000);}
  if(!strcmp(mode,"cancel")){until=now()+4;do{updater_poll(u,&s);usleep(1000);}while(s.received==0&&now()<until);assert(s.received);double at=now();updater_cancel(u);s=settled(u);assert(s.state==UPDATE_CANCELLED&&now()-at<1);}
  else{s=settled(u);if(!strcmp(mode,"normal")||!strcmp(mode,"redirect")||!strcmp(mode,"relative")||!strcmp(mode,"chunked")){if(s.state!=UPDATE_READY)fprintf(stderr,"download failed: %s\n",s.message);assert(s.state==UPDATE_READY&&s.received==s.total&&s.saved[0]);}else assert(s.state==UPDATE_ERROR);}
 }
 updater_close(u);printf("PASS: HTTPS updater %s state=%u; trusted host, bounded verified download and temporary-file cleanup\n",mode,s.state);
}
