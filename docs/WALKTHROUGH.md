# Blades 0.21 scanning and homebrew walkthrough

[Watch or download the MP4](../dist/Blades-v0.21-walkthrough.mp4)

102 seconds, 1280×720 at 30 fps, 48 kHz stereo audio. Fresh desktop GPU playback
uses this release's navigation and renderer with the matching 234-entry pack.
The video is labelled **simulated storage and launch preparation**; it does not
establish native storage access, package mounting, real handoff or console FPS.

- 00:00 Blades opening, then Media/Music navigation
- 00:07 Games → Played Games → Scan Folders
- 00:15 Add an HDD Games folder and choose its category
- 00:20 Choose depth 3; observe current folder/file, counts and depth
- 00:26 Edit the saved path's depth to 2 and scan again
- 00:32 Simulated zero-games result: scanning ends and controls remain active
- 00:37 Empty Game Library shows No games found
- 00:41 Apps and an explicit manual launch request
- 00:47 Homebrew entry with Title ID 0 and manual launch request
- 00:53 Return to Scan Folders and increase the Games depth to 3
- 01:02 Scan finishes with example games found
- 01:07 Game Library, scrolling and manual selection
- 01:15 Arcade and Demos libraries
- 01:24 Xbox Live and Marketplace submenus
- 01:32 Blade changes to System → Console Settings and return

Example Apps/Homebrew paths are configured before capture. All library entries
and storage results are injected demonstrations. In this sample, game files are
at depth 3, so reducing depth to 2 produces an empty Games library; increasing it
again finds them. Native scanning uses actual directory and executable/package
reads. The simulator records manual launch requests and keeps capturing; native
code hands the chosen path to XAM through xsdk_launch.

See [LIBRARY.md](LIBRARY.md) for setup and hardware validation. Artwork/sounds:
zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard. The compatible
234-entry payload is unchanged from 0.18–0.20.
