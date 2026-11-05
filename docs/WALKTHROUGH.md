# Blades 0.17 navigation walkthrough

[Watch or download the MP4](../dist/Blades-v0.17-walkthrough.mp4)

81 seconds, 1280x720 at 30 fps, with stereo navigation audio at 48 kHz, including the imported transition whooshes.
This is a fresh scripted desktop GPU capture of the shared drawing plan, GLSL shaders and navigation code,
using the published v0.17 premade artwork pack and glyphs recovered from the
user's Convection font resources, including the revised ABXY prompt sprites. The opening idle view also shows the reused
DashX360 background animation in motion.

- 00:00 Blades opening with late whoosh, landing at the Games main menu
- 00:02 Idle Games view and moving background
- 00:08 Games navigation and submenu
- 00:14 Media and submenu
- 00:20 System icons and Console Settings
- 00:42 Xbox Live and submenu
- 00:49 Marketplace and submenu
- 00:59 Exit confirmation and cancel
- 01:07 Complete Marketplace → Xbox Live → Games → Media → System sound sweep
- 01:12 Reverse sweep and return to Games

Artwork credit: zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard
interfaces. Asset ownership remains with the respective rights holders.

Version 0.10 includes the six imported transition recordings. Hear menu open/close at 00:11/00:13 and Games-to-Media at 00:14; later navigation demonstrates the other blade boundaries. See [TRANSITION-AUDIO.md](TRANSITION-AUDIO.md).

Version 0.11 includes the measured transition corrections described in [TRANSITION-AUDIT.md](TRANSITION-AUDIT.md). This is a fresh capture of the revised renderer.

Version 0.12 adds the corrected selected-row shadow; see [MENU-SHADOW.md](MENU-SHADOW.md). This is a fresh capture of the release renderer.

Version 0.13 corrects the lower-left fade of the selected-menu shadow; see [MENU-SHADOW.md](MENU-SHADOW.md). This is a fresh capture of the release renderer.

Version 0.14 uses the optimized background filter. This fresh desktop capture demonstrates appearance and navigation; see [BACKGROUND-PERFORMANCE.md](BACKGROUND-PERFORMANCE.md) for benchmarks and console timing logs.

Version 0.15 was recorded through the headless EGL/OpenGL backend (Mesa llvmpipe), using the production GPU drawing plan and shader sources. It is not a recording of Xbox execution or a measurement of console FPS. See [GPU-RENDERER.md](GPU-RENDERER.md).

Version 0.16 shows the repaired grey Xbox Live panel, its narrow outer shadow, and the faint groove beneath the top branding. The complete sweeps demonstrate the original boundary-specific whooshes without pitch modification. See [BRANDING-AUDIO-REVIEW.md](BRANDING-AUDIO-REVIEW.md). This is fresh desktop GPU playback, not Xbox performance footage.

Version 0.17 begins with the live-rendered Blades opening. The curved shutters come from the supplied asset repository; it ends at the normal Games root and includes its one-shot whoosh. This fresh 81-second desktop GPU capture includes startup, navigation and the boundary-sound sweeps. See [STARTUP-ANIMATION.md](STARTUP-ANIMATION.md).
