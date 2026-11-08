#include "xbox_ftp.h"
#include <arpa/inet.h>
#include "xsdk/net.h"
#include "xsdk/thread.h"
#include "xsdk/log.h"
/* XAM reads these before main; its 16 KiB defaults limit title throughput.
   63 KiB is the supported startup maximum. Keep default socket counts. */
const XsdkNetworkLimits xsdk_network_limits={.receive_kb=63,.send_kb=63};
static bool address(void *context,char ip[16]){(void)context;struct in_addr addr;if(xsdk_net_address(&addr))return false;return inet_ntop(AF_INET,&addr,ip,16)!=NULL;}
static void start(void *context){(void)context;(void)xsdk_thread_move(5);(void)xsdk_thread_set_priority(XSDK_CURRENT_THREAD,-2,NULL);}
static void transferred(void *context,const FtpTransfer *s){
    (void)context;xsdk_log_write("ftp-transfer: direction=%s bytes=%llu elapsed_ms=%u disk_calls=%u recv_buf=%d send_buf=%d success=%u",s->upload?"upload":"download",s->bytes,s->elapsed_ms,s->disk_calls,s->receive_buffer,s->send_buffer,s->success);
}
FtpServer *xbox_ftp_open(void){
    static const FtpDrive drives[]={
        {"HDD","hdd:/"},{"USB0","usb0:/"},{"USB1","usb1:/"},{"USB2","usb2:/"},
        {"MU0","mu0:/"},{"MU1","mu1:/"},{"InternalMemory","bladesmu:/"}};
    FtpOptions options={.drives=drives,.count=sizeof drives/sizeof drives[0],.port=21,
        .config="game:/Config/ftp-settings.txt",.address=address,.worker_start=start,.transfer_done=transferred};
    return ftp_open(&options);
}
