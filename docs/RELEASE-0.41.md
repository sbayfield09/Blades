# Blades 0.41 downloads

[Complete USB ZIP](../dist/Blades-v0.41-USB.zip) — extract onto USB and launch
Blades/default.xex. Assets/notices are included; saved settings/logs are excluded.

[XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.41-walkthrough.mp4) ·
[Source/relink bundle](../dist/Blades-v0.41-source.zip)

After0.40's audio-only/crash report, this build removes the unverified generic
AltiVec decoder paths/compiler vectorization and restores four workers on2–5.
It keeps GPU colour conversion/scaling as requested: three separate linear Y/U/V
textures and a simpler matrix shader replace the single tiled atlas texture,
following [Simple360Player's CPU/GPU split](SIMPLE360PLAYER-PIPELINE.md).
Compressed video still decodes on the CPU; no re-encoding/transcoding occurs.

The uploaded log contains no exact fault address, so the crash cause is not
proven. This revised native path needs a hardware retest. Start with the same720p
MPEG2 file that worked in0.39, then H264; send default.log if it fails. New
first-frame markers distinguish decoder, preparation, delivery, upload, submission
and GPU fence completion. video-perf still measures real decoded/presented FPS.

Host playback/ownership/early-reopen tests,328 GPU comparisons, actual Xbox shader
arithmetic/sampler checks, application ASan/UBSan and native XEX checks pass.
The complete source rebuild matches the XEX. Display output remains720p;
1080p30 performance is not established. See [validation](VALIDATION.md).
