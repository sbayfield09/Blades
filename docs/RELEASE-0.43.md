# Blades0.43 downloads

[Complete USB ZIP](../dist/Blades-v0.43-USB.zip) — extract onto USB, launch
Blades/default.xex. Includes matching Assets/notices; personal settings excluded.

[XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.43-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.43-source.zip)

The same H264 file still plays better in Simple360Player than Blades0.42.
This release ports its actual Xenon chroma/qpel motion-compensation routines
and matches its four-worker placement/priority, four-picture buffer, preroll,
5ms presentation tolerance and40/250ms adaptive non-reference catch-up policy.
Each optimized function table must first pass scalar comparisons at runtime.
Generic FFmpeg AltiVec remains disabled; the existing GPU YUV display is retained.

Big-endian PPC arithmetic/guard tests and66,560 independent reference checks
pass, along with host playback tests, instruction/XEX/shader checks. A desktop
capture is not console speed evidence; retest the same file and send default.log
if slower. The complete binary is not identical to the old XDK/FFmpeg0.7 build;
[the source comparison](SIMPLE360PLAYER-PIPELINE.md) documents the boundaries.
No GPU compressed-video decoder or verified1080p30 performance is claimed.

The combined executable is now GPLv3 because it includes Simple360Player source.
Complete corresponding application, decoder port, originals, free SDK, notices
and build scripts accompany it. Existing artwork attribution is preserved.
