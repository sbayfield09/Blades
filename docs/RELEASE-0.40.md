# Blades 0.40 downloads

[Complete USB ZIP](../dist/Blades-v0.40-USB.zip) — extract onto USB and launch
Blades/default.xex. The matching Assets and licence notices are included.

[Standalone XEX](../dist/Blades/default.xex) ·
[Premade artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.40-walkthrough.mp4) ·
[Complete source/relink bundle](../dist/Blades-v0.40-source.zip) ·
[1080p30 MPEG2 test clip](../dist/diagnostics/blades-v0.40/1080p30-mpeg2.mkv)

Changes from0.39:
- Enable PowerPC/AltiVec decoding and place three decoder workers across all
  physical cores, replacing the scalar build and two-core worker placement.
- Keep common8-bit YUV420 frames native up to1920x1080; the GPU performs colour
  conversion and scaling.720p upload bytes decrease62.5% versus RGBA. Other
  formats retain the RGBA fallback. The display surface is still1280x720.
- Skip late frame preparation and use the actual frame rate for fallback timing.
- Log decoded/presented FPS, decode-call/preparation time, late/dropped frames
  and audio underruns in video-perf lines. UI perf: fps is a separate measure.

The user confirmed720p MPEG2/WMV work well in0.39 and reported slow/jittery720p
H264. New0.40 optimizations and1080p30 console performance are unverified.
Try the clip and the same long H264 file, play normally for15seconds and send
Blades/default.log. The six-second clip can be replayed to test stability.

Host playback/ownership/seek/control checks,328 GPU comparisons, application
ASan/UBSan, a standalone PPC VMX arithmetic harness and native build/integrity
checks pass. Source rebuild produces the identical XEX. See
[validation](VALIDATION.md) and [playback details](VIDEO-PLAYBACK.md).
