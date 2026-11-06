# Blades 0.20 library walkthrough

[Watch or download the MP4](../dist/Blades-v0.20-walkthrough.mp4)

75 seconds, 1280×720 at 30 fps, with 48 kHz stereo audio. Fresh desktop GPU
playback uses this release's shared navigation/renderer and matching 234-entry
artwork pack. The picture is labelled **simulated library and storage — no
console launch**. Sample titles, storage listings and launch preparation are
injected snapshots. The recording does not establish physical storage access,
package mounting, actual game handoff or console performance.

- 00:00 Blades opening and startup whoosh
- 00:02 Media blade, Music submenu and return
- 00:07 Games → Played Games → Scan Folders
- 00:12 Existing Apps and Homebrew scan paths; select Add Folder
- 00:15 Storage picker, HDD, Games folder, X: Use Folder
- 00:19 Choose Games category; scan all subfolders
- 00:24 Return to Game Library
- 00:29 Wrap to the last title and scroll the list; return to the first title
- 00:31 Explicit A launch request (simulated)
- 00:35 Apps and its manual launch request
- 00:41 Homebrew and its manual launch request
- 00:48 Xbox Live Arcade library and its manual launch request
- 00:54 Demos library
- 00:57 Xbox Live and Marketplace submenu navigation
- 01:05 Return through the blades to System → Console Settings
- 01:12 Return to System root

The example Apps and Homebrew paths were configured before capture. The video
adds a Games path through the real menu actions. The simulator keeps recording
after a manual launch request; native code prepares the executable/package on a
worker and hands it to XAM through xsdk_launch.

See [LIBRARY.md](LIBRARY.md) for configuration, supported formats and validation.
The existing disc behavior remains covered by the regression suite; previous
disc-reading videos remain available in the repository.

Artwork and sounds: zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard.
The compatible artwork payload is unchanged from versions 0.18 and 0.19.
