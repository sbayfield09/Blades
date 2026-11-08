# Blades 0.48

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.48-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files; private paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.48-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.48-source.zip)

Small Jellyfin seeks now display the requested part of the video. The old build
could reopen cached pictures while showing an updated clock: on the official
10.10.7 server, a request for 4.25 seconds decoded the 4.00-second picture.
Each restart now isolates its progressive cache generation while retaining the
device, authentication and logical playback/history session. Delayed cleanup of
an older stream cannot delete the new generation. Left/Right remains ten seconds;
network EOF seeks use the server's half-second margin. [Setup/limits](docs/JELLYFIN.md).

Tests compare actual decoded source frame numbers, including fractional,
one-second and ten-second forward/backward/return seeks, EOF and 12 seconds of
continuous picture/clock agreement. Official-server content checks pass within
40 ms, with a maximum measured rounding of 17 ms at 30 fps. All 30 host suites,
four application sanitizer suites plus the cache-content mode, native integrity/
shader/instruction checks and an identical archived-source rebuild pass.
The fresh 215-second desktop walkthrough shows source counters changing across
seeks; simulated console services are labelled. Xbox behavior needs hardware retesting.

The 0.47 media-inactivity/audio-sync corrections and **Series Name S1E2 - Episode Name**
labels are retained. Shared media folders and existing saved paths remain as in
0.46, with no migration. Codec kernels, decoder workers, GPU conversion and PCM
functions are unchanged this release.

The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices are preserved. Complete source, changes, build scripts and
pinned free SDK are in the source ZIP; no Microsoft XDK is needed.
See [release notes](docs/RELEASE-0.48.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
