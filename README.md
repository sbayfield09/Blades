# Blades0.43

Xbox360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.43-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files when updating; personal paths/logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.43-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.43-source.zip)

0.43 ports Simple360Player's actual Xenon H264 chroma/qpel filters and matches
its four-worker placement, buffering and adaptive catch-up. GPU YUV display
and the Blades file chooser are retained. Whole-binary parity with the old XDK
player is not claimed; [comparison and boundaries](docs/SIMPLE360PLAYER-PIPELINE.md).
26 host suites, PPC arithmetic/reference, sanitizer and native integrity checks
pass. Real console speed still needs a same-file comparison. The walkthrough
is a fresh desktop capture with simulated console services explicitly labelled.

The combined executable is distributed under GPLv3. Original MIT/FFmpeg/artwork
notices are preserved. Complete source, changes, build scripts and pinned free
SDK are in the matching source ZIP; no Microsoft XDK is needed to rebuild.
See [release notes](docs/RELEASE-0.43.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
