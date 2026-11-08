# Blades 0.45

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.45-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files when updating; private paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.45-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.45-source.zip)

Jellyfin now supports Left/Right seeking in 10-second steps, including while
paused. Continue Watching and Next Up appear above the video libraries and use
Jellyfin's own user-specific lists. Opening a partly watched video from any
folder fetches fresh server data and resumes automatically. Returning to the
list updates it. Server watched/resume thresholds still apply.

Media Center manages your Jellyfin connection, based on the native code from
[XenonFin](https://github.com/sbayfield09/XenonFin). Connect with a numeric LAN
IPv4 address and port (HTTP, default 8096), username and password. Enable video/
audio transcoding for that account. The server provides MPEG2/MP2 up to 720p30;
seeking restarts that stream at the requested timestamp, with a short buffering
delay. Sessions reconnect at startup; passwords are not saved.
[Setup and limits](docs/JELLYFIN.md).

Video retains Jellyfin and Local folders, the Blades file chooser and the
working 0.43 local decoder/Xenon/GPU optimisations. No extra player XEX or plugin.
29 host and eight application sanitizer suites, native integrity/shader checks
and official Jellyfin 10.10.7 resume/seek/Next Up playback tests pass. Actual Xbox
networking, keyboard and streaming still need hardware testing. The fresh
160-second walkthrough is a desktop capture; simulated services are labelled.

The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices are preserved. Complete source, changes, build scripts and
pinned free SDK are in the source ZIP; no Microsoft XDK is needed.
See [release notes](docs/RELEASE-0.45.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
