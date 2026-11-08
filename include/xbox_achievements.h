#ifndef BLADES_XBOX_ACHIEVEMENTS_H
#define BLADES_XBOX_ACHIEVEMENTS_H
#include "achievements.h"
typedef struct XboxAchievements XboxAchievements;
XboxAchievements *xbox_achievements_open(void);
AchievementService *xbox_achievements_service(XboxAchievements *app);
unsigned xbox_achievements_show(XboxAchievements *app,const AchievementRequest *request);
void xbox_achievements_close(XboxAchievements *app);
#endif
