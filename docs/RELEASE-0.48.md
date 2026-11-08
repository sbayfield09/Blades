# Blades 0.48

[USB ZIP with XEX and Assets](../dist/Blades-v0.48-USB.zip) ·
[Separate XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.48-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.48-source.zip)

Small Jellyfin seeks now display the requested part of the video instead of
replaying cached pictures under an updated clock. Left/Right still skips ten
seconds. Network end seeks use Jellyfin's half-second EOF margin; local seeking
and the 0.47 audio/timing corrections are retained.

Jellyfin 10.10.7 derives progressive output filenames from media, User-Agent,
device and PlaySessionId, omitting StartTimeTicks. Its stop endpoint returns
before delayed deletion. Reusing that filename could reopen the old output;
an older cleanup could also remove the restarted stream's file. Every restart
now gets a unique bounded User-Agent generation, isolating its cache without
changing authentication, the device or the logical playback/history session.
The authenticated request is replaced atomically and credentials are not logged.

The failure was reproduced against an official disposable Jellyfin server:
requesting 4.25 seconds decoded the 4.00-second picture while reporting 4.26.
The new tests encode frame numbers into the source pixels and compare actual
decoded content, rather than trusting the rebased playback clock. Quarter-second,
50-ms, one-second and ten-second forward/backward/return seeks, EOF bounds and
12 seconds of continuous picture/clock agreement pass. Typical measured rounding
is 17 ms at 30 fps. A delayed-deletion cache fixture tests the same behavior.

All 30 host suites pass. Application sanitizers cover the existing UI/protocol/
video/sync suites and the new cache/content path. Native integrity/shader/
instruction checks and an identical archived-source rebuild pass. The fresh
desktop walkthrough uses source frame counters so real seek changes are visible;
simulated console services are labelled. Xbox behavior still needs hardware testing.

Series S1E2 labels, Continue Watching/Next Up, shared media folders and existing
saved paths are retained; no migration. Complete GPLv3 source/build scripts,
pinned free SDK and original notices/artwork attribution are included. Private
settings, user media and logs are excluded.

Server references: [cache key](https://github.com/jellyfin/jellyfin/blob/v10.10.7/Jellyfin.Api/Helpers/StreamingHelpers.cs#L368),
[asynchronous stop](https://github.com/jellyfin/jellyfin/blob/v10.10.7/Jellyfin.Api/Controllers/HlsSegmentController.cs#L111),
[cached file reuse](https://github.com/jellyfin/jellyfin/blob/v10.10.7/Jellyfin.Api/Helpers/FileStreamResponseHelpers.cs#L96).
