# Blades 0.18 navigation and disc walkthrough

[Watch or download the MP4](../dist/Blades-v0.18-walkthrough.mp4)

118 seconds, 1280×720 at 30 fps, with stereo audio at 48 kHz. This is fresh
**desktop GPU playback with simulated disc hardware**, visibly labelled in
its frames. It uses the release renderer and matching 234-entry pack. Disc
observations enter the production state machine; the recording does not show
physical Xbox I/O, actual game launch, or console performance.

- 00:00 Blades opening and late startup whoosh, landing on Games
- 00:08 Games navigation and submenu
- 00:14 Media and submenu
- 00:20 System and Console Settings
- 00:42 Xbox Live and submenu
- 00:49 Marketplace and submenu
- 00:59 Exit confirmation and cancel
- 01:07 Forward blade sweep and boundary whooshes
- 01:12 Reverse sweep, ending at Games
- 01:22 Select the disc row; A opens, then closes the tray
- 01:28 Reading with animated indicator
- 01:34 Play Halo 3; waits for an explicit action
- 01:36 Media disc row
- 01:39 Explicit A launch request (simulated; no game process starts)
- 01:40 Y eject
- 01:43 Simulated physical-button close
- 01:45 Reading on Media, followed by Games and Xbox Live navigation
- 01:49 Ready on Xbox Live
- 01:52 Simulated physical-button eject; finishes with Close Tray

The title Halo 3 is injected test metadata for this desktop sequence. Native
readers obtain names from the actual disc. The simulator acknowledges manual
launch without leaving the capture; native code calls XAM through xsdk_launch.

Artwork/indicator credit: zivvoz / DashX360, inspired by Microsoft's Xbox 360
Dashboard. Original assets retain ownership with their respective rights holders.
See [DISC-READING.md](DISC-READING.md) and [CONSOLE-TEST.txt](CONSOLE-TEST.txt).
