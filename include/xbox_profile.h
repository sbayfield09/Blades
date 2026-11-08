#ifndef BLADES_XBOX_PROFILE_H
#define BLADES_XBOX_PROFILE_H
#include "profile.h"
typedef struct XboxProfile XboxProfile;
XboxProfile *xbox_profile_open(void);
ProfileService *xbox_profile_service(XboxProfile *profile);
unsigned xbox_profile_signin(void);
unsigned xbox_profile_show(XboxProfile *profile,ProfileCommand command);
void xbox_profile_close(XboxProfile *profile);
#endif
