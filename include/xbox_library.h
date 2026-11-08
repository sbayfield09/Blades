#ifndef BLADES_XBOX_LIBRARY_H
#define BLADES_XBOX_LIBRARY_H
#include "library.h"
typedef struct XboxLibrary XboxLibrary;
XboxLibrary *xbox_library_open(void);
LibraryService *xbox_library_service(XboxLibrary *library);
/* Preserve the execute mount through xsdk_launch's atexit handoff. If launch
   preflight returns, clear it and close the mount before returning to UI. */
void xbox_library_handoff(XboxLibrary *library,bool preserve);
void xbox_library_close(XboxLibrary *library);
#endif
