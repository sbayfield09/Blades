# Blades 0.19 disc-reading walkthrough

[Watch or download the MP4](../dist/Blades-v0.19-walkthrough.mp4)

52 seconds, 1280×720 at 30 fps, with 48 kHz stereo audio. Fresh desktop GPU
playback is labelled in the picture. Disc hardware and metadata are simulated;
this is not a recording of physical Xbox I/O or actual game launch.

- 00:00 Blades opening and startup whoosh
- 00:02 Media blade, Music submenu and return
- 00:07 Games disc row; A opens and closes the tray
- 00:13 Reading through pending media responses
- 00:17 Cached tray state changes while Reading continues
- 00:20 Positive title-ID return and Play Example Game; no autoplay
- 00:22 Media disc row
- 00:25 Explicit A launch request, acknowledged by the simulator
- 00:26 Y eject
- 00:29 Simulated physical-button close with an empty drive
- 00:31 Reading during pending detection
- 00:35 Confirmed no-media returns to Open Tray
- 00:36 Games and Xbox Live disc rows
- 00:39 Another open; physical close at 00:42
- 00:44 Reading, then Play Example Game at 00:48

The injected title Example Game is test metadata. Native readers use the disc's
actual name. The simulator keeps capturing after a manual launch request;
native code hands the executable to XAM through xsdk_launch.

The pending word FFFFFFFE and positive return 5553083B are based on the supplied
console log. They pass through the same media-classification policy as native
queries. See [DISC-READING.md](DISC-READING.md) for the fix and hardware limits.

Artwork/indicator: zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard.
The matching 234-entry pack is unchanged from v0.18.
