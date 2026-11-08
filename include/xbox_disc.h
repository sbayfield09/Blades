#ifndef BLADES_XBOX_DISC_H
#define BLADES_XBOX_DISC_H
#include "disc.h"
#include <stdio.h>
#define DISC_NATIVE_XEX "/Device/CdRom0/default.xex"
typedef struct XboxDisc XboxDisc;
XboxDisc *xbox_disc_open(void);
void xbox_disc_status(XboxDisc *drive,DiscStatus *status);
bool xbox_disc_request(XboxDisc *drive,DiscCommand command,uint32_t generation);
bool xbox_disc_take_launch(XboxDisc *drive,DiscMedia *media);
void xbox_disc_launch_failed(XboxDisc *drive);
void xbox_disc_close(XboxDisc *drive);
bool xbox_disc_xex_title(FILE *file,uint32_t *id,char title[DISC_TITLE_BYTES]);
#endif
