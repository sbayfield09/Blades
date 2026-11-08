# Blades 0.44

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.44-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files when updating; personal paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.44-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.44-source.zip)

Media Center now manages your Jellyfin connection, based on the native code
from [XenonFin](https://github.com/sbayfield09/XenonFin). Video contains
Jellyfin and Local folders. Browse libraries, shows, seasons and episodes;
play the selected video through the existing Blades player. Local decoding
and the working 0.43 Xenon/GPU optimisations are retained.

Connect with a LAN numeric IPv4 address and port (HTTP, default 8096), then your
Jellyfin username/password. Enable video/audio transcoding for that account.
The server provides a compatible MPEG2/MP2 stream up to 720p30. A saved session
reconnects at startup; passwords are not saved. No extra player XEX or plugin.
Progressive Jellyfin seeking is not enabled. [Setup and limits](docs/JELLYFIN.md).

29 host suites, eight sanitizer suites, native integrity/shader checks and an
isolated official Jellyfin 10.10.7 HEVC-to-MPEG2 playback test pass. Actual Xbox
networking, keyboard and streamed playback still need hardware testing. The
fresh walkthrough is a desktop capture; simulated console services are labelled.

The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices are preserved. Complete source, changes, build scripts and
pinned free SDK are in the matching source ZIP; no Microsoft XDK is needed.
See [release notes](docs/RELEASE-0.44.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
