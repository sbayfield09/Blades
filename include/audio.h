#ifndef BLADES_AUDIO_H
#define BLADES_AUDIO_H
#include <stddef.h>
typedef enum {
    UI_SOUND_NONE, UI_SOUND_FOCUS, UI_SOUND_SELECT, UI_SOUND_BACK,
    UI_SOUND_MENU_OPEN, UI_SOUND_MENU_CLOSE,
    UI_SOUND_MARKETPLACE_LIVE, UI_SOUND_LIVE_GAMES,
    UI_SOUND_GAMES_MEDIA, UI_SOUND_MEDIA_SYSTEM, UI_SOUND_STARTUP, UI_SOUND_COUNT
} UiSound;
/* Initialise/close with the device stopped. Samples are owned copies. */
void ui_audio_init(void);
void ui_audio_close(void);
/* One producer (navigation), one consumer (device or offline capture). */
void ui_audio_play(UiSound sound);
void ui_audio_render(float *stereo,size_t frames);
#endif
