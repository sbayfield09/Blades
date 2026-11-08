# Blades0.44 downloads

[Complete USB ZIP](../dist/Blades-v0.44-USB.zip) — XEX, matching Assets and notices.

[XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Fresh desktop walkthrough](../dist/Blades-v0.44-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.44-source.zip)

Media Center now manages a Jellyfin server/account using the Xbox keyboard.
Video contains Jellyfin and Local folders. Jellyfin authenticates, browses
video libraries/folders/series/seasons/episodes with pagination, and plays the
selected video through the existing Blades player. The server negotiates a
compatible MPEG2/MP2 progressive MPEG-TS stream up to720p30. Local and the0.43
H264/worker/GPU implementation are retained. Video Marketplace still manages
local scan paths. No extra XEX, server plugin or full-media download is used.

Based on the native code in the user's XenonFin repository. A private saved
session reconnects at startup; passwords are not saved. Disconnect/remove clear
it. Playback reports and encoder cleanup follow XenonFin. Account data is never
packaged, logged or shown in the recording. [Setup/limits](JELLYFIN.md): native
HTTP numeric IPv4, transcode permission required, no progressive-stream seeking.

Production worker/player tests and an isolated official Jellyfin10.10.7 server
pass: synthetic HEVC/AAC -> MPEG2/MP2 ->90decoded pictures plus audio, pause/end,
watch history, saved-token reconnect. Native compilation/integrity and renderer
checks pass. Actual Xbox Jellyfin/keyboard/network behaviour still needs testing.
The combined executable is GPLv3; all source/changes/build scripts and preserved
MIT/FFmpeg/Simple360Player/XenonFin/SDK/artwork notices are provided.
