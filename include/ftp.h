#ifndef BLADES_FTP_H
#define BLADES_FTP_H
#include <stdbool.h>
enum { FTP_DRIVES=10,FTP_PATH=1024,FTP_CLIENTS=4,FTP_LOGIN_BYTES=64 };
#define FTP_USERNAME "blades"
#define FTP_PASSWORD "blades"
typedef enum { FTP_OFF,FTP_STARTING,FTP_RUNNING,FTP_WAITING,FTP_ERROR } FtpState;
typedef struct {char name[24],path[FTP_PATH];} FtpDrive;
typedef struct {
    FtpState state;bool enabled,save_error;unsigned devices,port,revision;
    char ip[16];int error;
    char username[FTP_LOGIN_BYTES],password[FTP_LOGIN_BYTES];
} FtpSnapshot;
typedef struct {
    bool upload,success;unsigned long long bytes;
    unsigned elapsed_ms,disk_calls;int receive_buffer,send_buffer;
} FtpTransfer;
typedef struct {
    const FtpDrive *drives;unsigned count,port;
    const char *bind_address,*config;
    void *context;
    bool (*address)(void *,char ip[16]);
    void (*worker_start)(void *);
    void (*transfer_done)(void *,const FtpTransfer *);
} FtpOptions;
typedef struct FtpServer FtpServer;
/* Network, disk reads/writes and setting persistence run on workers. */
FtpServer *ftp_open(const FtpOptions *options);
void ftp_poll(FtpServer *server,FtpSnapshot *snapshot);
void ftp_enable(FtpServer *server,bool enabled);
bool ftp_credentials(FtpServer *server,const char *username,const char *password);
void ftp_close(FtpServer *server);
const char *ftp_status(const FtpSnapshot *snapshot);
#endif
