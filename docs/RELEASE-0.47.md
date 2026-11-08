# Blades 0.47

[USB ZIP with XEX and Assets](../dist/Blades-v0.47-USB.zip) ·
[Separate XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.47-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.47-source.zip)

Jellyfin media response/body inactivity now allows 30 seconds instead of eight,
so a temporary transcoder stall after seeking can recover. TCP connect/send and
control requests keep their short deadlines; cancel remains responsive. Remote
opening/probing has a cancellable 60-second limit. Genuine 30-second media
inactivity still fails with a clear error. The old timeout/I/O failure is
reproduced by a ten-second delivery gap and succeeds with the corrected reader.
The console log confirms the error, but does not establish why that server
stopped delivering data; server throttling is one possible source of such gaps.

Audio now preserves its source timestamps when filling the PCM queue. Missing
audio time becomes silence and overlapping samples are trimmed. The clock stays
anchored to the seek origin and resampler delay is accounted for. Previously,
concatenating audio across a timestamp gap made it play ahead of the picture.
A decoded MPEG2/MP2 flash-and-tone test reproduces about 0.52 seconds of early
audio and now passes within 100 ms from the start and after a seek. Diagnostic
logs include inserted-gap and overlap sample counts. Codec kernels, workers and
GPU conversion are retained. Real-console lip sync still needs a hardware check.

Continue Watching and Next Up episode labels use Series Name S1E2 - Episode
Name, using Jellyfin season/episode numbers (including specials/episode zero). Existing series/season prefixes are normalized without duplication. Movies and
missing-series titles are preserved; missing episode numbers fall back to the
series and title.
The underlying episode metadata is unchanged and long combined labels are bounded.

All 30 host suites pass. Application sanitizers cover the protocol/UI, actual
video and sync test plus stalled-network recovery/cancellation. An isolated
official Jellyfin 10.10.7 test plays for 12 wall-clock seconds after seeking,
checks actual episode labels/resume/history and uses synthetic media only.
Native integrity/shader/instruction checks pass. The fresh desktop walkthrough
shows both qualified lists and sustained post-seek playback; simulated console
services are labelled. No Xbox timing claim is made.

Shared media folders and existing saved paths remain as in 0.46, with no migration.
The GPLv3 source/build scripts and pinned free SDK are included with original
notices and artwork attribution. Private settings and logs are excluded.
