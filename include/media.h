#ifndef BLADES_MEDIA_H
#define BLADES_MEDIA_H
#include "library.h"
/* These verified retail dashboard commands open a media library, not an
   arbitrary selected file. Do not put a filename in the reserved payload. */
bool media_dashboard_data(LibraryCategory category,unsigned user,uint8_t data[1020]);
#endif
