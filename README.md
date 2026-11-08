# Blades 0.49

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.49-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files; private paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.49-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.49-source.zip)

Selected Jellyfin movies and episodes now show their primary artwork,
description and full runtime in the right panel, including Continue Watching
and Next Up. Posters and landscape pictures keep their proportions. Descriptions
wrap with an ellipsis when needed; absent metadata has explicit fallbacks.
Artwork loads on the worker with a short selection debounce, three-item memory
cache and bounded PNG response/dimensions. Navigation/playback preempt downloads;
no artwork is saved to disk. [Jellyfin setup and limits](docs/JELLYFIN.md).

FTP now batches fragmented uploads into 256 KiB POSIX disk writes and uses
256 KiB disk reads. It bypasses small newlib stdio buffers and redundant ready
polls, increases XAM/socket buffers to 63 KiB and uses TCP_NODELAY on the control
connection. Transfer logs report bytes, elapsed_ms, disk_calls, accepted buffers
and success without paths or credentials. Existing login, on/off choice, root
drives and worker priority are retained. [FTP details](docs/FTP.md).

Desktop loopback benchmarks improved median bulk transfers by about 2x, with
much faster small-file queues. These measurements do not establish Xbox speeds
or parity with Aurora. Actual console transfer speed and menu responsiveness
need retesting on the same network/client/storage.

All 30 host suites and final targeted UI/FTP/GPU checks pass. Four application
sanitizer suites plus bounded artwork and parallel/boundary FTP modes pass.
Official Jellyfin 10.10.7 tests verify actual movie/episode images and metadata,
retained source-frame seek correctness and sustained playback. Native integrity/
shader/instruction checks and an identical archived-source application rebuild
pass. The fresh 215-second walkthrough is desktop GPU playback; console services
are simulated and labelled.

The 0.48 seek correction, 0.47 audio/timeouts/Series S1E2 labels and shared media
folders remain. No settings migration or decoder algorithm changes this release.
The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices and complete source/build scripts/pinned free SDK are supplied.
See [release notes](docs/RELEASE-0.49.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
