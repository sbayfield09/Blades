# Blades 0.45

[USB ZIP with XEX and Assets](../dist/Blades-v0.45-USB.zip) ·
[Separate XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.45-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.45-source.zip)

Jellyfin now supports Left/Right seeking in 10-second steps. A seek cancels the
old stream, stops its active encoder and requests the same playback session at
the requested timestamp. Decoder, audio and picture queues reset, with both
clocks mapped to the full video timeline. Paused seeks stay paused; rapid
presses accumulate against the requested position and targets stay in bounds.
The server must restart transcoding, so a brief buffering delay is expected.

Continue Watching and Next Up appear above the video libraries. They use
Jellyfin's own user-specific lists, ordering, watched history and pagination.
Opening a partly watched video from any Jellyfin folder fetches fresh saved
position data and resumes automatically. Missing/finished positions start over.
Stopping reports the absolute position and refreshes the current folder.

Media Center connection controls and Video's Jellyfin/Local folders remain.
The stream is still MPEG2 video/MP2 audio up to 720p30; HTTP numeric IPv4 and
transcoding permission are required. [Setup and limits](JELLYFIN.md).
Local decoding, seeking and the working 0.43 Xenon/GPU optimisations are retained.

All 29 host suites and eight application sanitizer suites pass. An isolated
official Jellyfin 10.10.7 server verifies actual Next Up/Continue Watching data,
4-second resume, forward/back/paused/rapid/bounded seeks, picture/audio output
and a saved full-item stop position. Synthetic media and disposable credentials
are used. The native XEX and shader/instruction checks pass. Actual Xbox network,
keyboard and streamed playback still need hardware testing. The walkthrough
uses the desktop renderer and labels simulated console services.

The executable remains GPLv3, with complete corresponding source/build scripts,
pinned free SDK and original MIT/FFmpeg/Simple360Player/XenonFin/artwork notices.
Private sessions, scan paths and logs are excluded from all release archives.
