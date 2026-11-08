#include "media.h"
#include <string.h>
bool media_dashboard_data(LibraryCategory category,unsigned user,uint8_t data[1020]) {
    unsigned command;
    if(category==LIB_MUSIC)command=6;
    else if(category==LIB_PICTURES)command=44;
    else if(category==LIB_VIDEO)command=39;
    else return false;
    memset(data,0,1020);data[7]=(uint8_t)command;data[11]=(uint8_t)(user<4?user:0xff);
    return true;
}
