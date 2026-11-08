#include "blades.h"
#include "draw_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void transferred(void *context,const FtpTransfer *s){
    (void)context;assert(s->receive_buffer>0&&s->send_buffer>0);
    if(s->success)assert(s->disk_calls<=(s->bytes+262143)/262144+1);
}
static void navigation(void){
    Dashboard d;dashboard_init(&d);d.blade=BLADE_SYSTEM;d.position=d.target=d.origin=BLADE_SYSTEM;d.selected[BLADE_SYSTEM]=4;
    d.ftp=(FtpSnapshot){.enabled=true,.state=FTP_RUNNING,.devices=1,.revision=1};strcpy(d.ftp.ip,"192.168.1.50");
    dashboard_update(&d,SELECT,0);assert(dashboard_ftp_page(&d)&&!d.ftp_toggle);
    dashboard_update(&d,SELECT,0);assert(d.ftp_toggle&&!d.dialog);dashboard_update(&d,0,0);assert(!d.ftp_toggle);
    dashboard_update(&d,DOWN,0);assert(d.child==1&&!d.ftp_toggle);dashboard_update(&d,SELECT,0);assert(d.ftp_edit==1&&!d.ftp_toggle);
    dashboard_update(&d,DOWN,0);dashboard_update(&d,SELECT,0);assert(d.ftp_edit==2&&!d.ftp_toggle);dashboard_update(&d,DOWN,0);assert(!d.child);
    d.menu_transition=1;DrawList list={0};assert(draw_list_update(&list,&d));assert(!draw_list_update(&list,&d));
    d.ftp.devices=2;assert(draw_list_update(&list,&d));d.ftp.enabled=false;d.ftp.state=FTP_OFF;assert(draw_list_update(&list,&d));
    dashboard_update(&d,BACK,0);assert(!dashboard_ftp_page(&d));d.menu_transition=1;d.blade=d.previous_blade=BLADE_GAMES;d.position=d.target=d.origin=BLADE_GAMES;d.highlight=0;assert(draw_list_update(&list,&d));
    d.ftp.devices=0;assert(!draw_list_update(&list,&d));draw_list_close(&list);
}
int main(int argc,char **argv){
    assert(argc==3);navigation();
    FtpDrive drives[3]={{.name="HDD"},{.name="USB0"},{.name="USB1"}};
    for(unsigned i=0;i<3;i++){size_t a=strlen(argv[1]),b=strlen(drives[i].name);assert(a+b+2<FTP_PATH);memcpy(drives[i].path,argv[1],a);drives[i].path[a]='/';memcpy(drives[i].path+a+1,drives[i].name,b+1);}
    FtpOptions options={.drives=drives,.count=3,.bind_address="127.0.0.1",.config=argv[2],.port=0,.transfer_done=transferred};
    FtpServer *server=ftp_open(&options);assert(server);char command[32];
    while(fgets(command,sizeof command,stdin)){
        if(!strcmp(command,"quit\n"))break;
        if(!strcmp(command,"off\n"))ftp_enable(server,false);
        if(!strcmp(command,"on\n"))ftp_enable(server,true);
        if(!strcmp(command,"credentials\n"))assert(ftp_credentials(server,"custom_user","custom_pass"));
        if(!strcmp(command,"invalid\n"))assert(!ftp_credentials(server,"bad\nuser","password"));
        FtpSnapshot s;ftp_poll(server,&s);
        printf("{\"state\":%u,\"enabled\":%s,\"devices\":%u,\"port\":%u,\"save_error\":%s}\n",s.state,s.enabled?"true":"false",s.devices,s.port,s.save_error?"true":"false");fflush(stdout);
    }
    ftp_close(server);return 0;
}
