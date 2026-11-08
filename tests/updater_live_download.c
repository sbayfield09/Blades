#define _DEFAULT_SOURCE
#include "updater.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static UpdateSnapshot wait_for(Updater *u){double until=now()+90;UpdateSnapshot s;
 do{updater_poll(u,&s);if(s.state!=UPDATE_IDLE&&s.state!=UPDATE_CHECKING&&s.state!=UPDATE_DOWNLOADING)return s;usleep(1000);}while(now()<until);
 assert(!"live updater timed out");return s;
}
int main(int argc,char **argv){
 assert(argc==2);Updater *u=updater_open(argv[1],NULL,NULL);assert(u&&updater_check(u));UpdateSnapshot s=wait_for(u);
 if(s.state!=UPDATE_AVAILABLE)fprintf(stderr,"Live check: %s\n",s.message);
 assert(s.state==UPDATE_AVAILABLE&&update_version_compare(s.version,BLADES_VERSION)>0);
 double until=now()+1;while(!updater_download(u)){assert(now()<until);usleep(1000);}
 s=wait_for(u);if(s.state!=UPDATE_READY)fprintf(stderr,"Live download: %s\n",s.message);
 assert(s.state==UPDATE_READY&&s.received==s.total&&s.saved[0]);updater_close(u);
 printf("PASS: actual GitHub dist manifest and USB ZIP downloaded, exact length/SHA256 verified; older-client simulation=%s latest=%s bytes=%llu\n",BLADES_VERSION,s.version,(unsigned long long)s.received);
}
