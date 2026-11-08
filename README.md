# Blades 0.47

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.47-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files; private paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.47-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.47-source.zip)

Jellyfin now tolerates up to 30 seconds of media inactivity after seeking,
retaining short control deadlines and responsive cancellation. A ten-second
post-seek stall reproduces the old timeout/I/O error and succeeds in this build;
genuine 30-second stalls still fail clearly. The server-side cause of the
uploaded console log's delivery gaps is not established by that log.

Audio preserves missing source timestamp ranges as silence, trims overlaps and
accounts for resampler delay. The old FIFO concatenated audio across gaps,
letting sound play ahead of the picture. A decoded flash/tone test reproduces
about 0.52 seconds of early sound and now aligns within 100 ms initially and
after a seek. Console lip sync still needs a hardware check.

Continue Watching and Next Up use **Series Name S1E2 - Episode Name**. Season
and episode numbers come from Jellyfin, including specials/episode zero.
Existing prefixes are normalized once; missing numbers fall back to series/title.
Movies and ordinary nested titles retain their names. [Setup/limits](docs/JELLYFIN.md).

Shared media folders and existing saved paths remain as in 0.46, with no migration.
Codec kernels, workers and GPU conversion are retained. All 30 host suites,
four sanitizer suites plus stalled-network modes, native integrity/shader/
instruction checks and a byte-identical archived-source rebuild pass. Official
Jellyfin 10.10.7 tests sustain actual post-seek playback for 12 wall-clock seconds.
The fresh 215-second walkthrough is a desktop capture; simulated services are labelled.

The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices are preserved. Complete source, changes, build scripts and
pinned free SDK are in the source ZIP; no Microsoft XDK is needed.
See [release notes](docs/RELEASE-0.47.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
