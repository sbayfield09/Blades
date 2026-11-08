# Blades 0.49

[USB ZIP with XEX and Assets](../dist/Blades-v0.49-USB.zip) ·
[Separate XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.49-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.49-source.zip)

Selecting a movie or episode in Jellyfin now shows its primary artwork,
description and full runtime in the right-hand panel, including Continue
Watching and Next Up. Portrait posters and landscape episode pictures keep
their proportions. Descriptions wrap inside the available area and end with
an ellipsis when too long. Missing artwork, description or runtime has an
explicit fallback. Metadata comes from the authenticated server.

Artwork loads on the worker after a 200 ms selection pause. Navigation,
playback and changing selection cancel the low-priority request. A three-item
memory cache avoids repeat downloads, including missing images. Requests use
PNG output, a 288x176 decoded bound and 256 KiB response limit, checked before
allocating decoded pixels. No artwork is saved to disk. Optional image errors
do not replace library status or prevent playback. [Jellyfin details](JELLYFIN.md).

FTP now uses 256 KiB POSIX disk reads/writes and batches fragmented upload reads,
bypassing newlib's small stdio buffer. Ready uploads drain directly; polling
happens when recv would block. XAM startup and passive/data sockets request
63 KiB send/receive buffers. TCP_NODELAY on control sockets removes delayed
reply overhead for queues of small files. Memory remains bounded per transfer;
worker placement/priority, drive roots, saved credentials and toggle are unchanged.
The log adds ftp-transfer lines with direction, completed bytes, elapsed_ms,
disk_calls, accepted buffers and success, excluding names/paths/credentials.

Desktop loopback benchmarks with three 32 MiB transfers improved median upload
from about 700 to 1482 MiB/s and download from 698 to 1302 MiB/s. A queue of
48 small files dropped from 2.12 to 0.018 seconds. Those measurements show
application overhead changes on desktop; they do not predict Xbox disk/network
rates or establish parity with Aurora. Test on the same console, wired network,
client and storage to compare. [FTP behavior and diagnostics](FTP.md).

Validation covers real authenticated movie/episode artwork and overview/runtime
against official Jellyfin 10.10.7, malformed/missing/oversized images, rapid
selection/cancellation/cache, portrait/landscape rendering, and retained actual
source-frame seek checks. FTP tests verify boundary tails, fragmented sends,
parallel uploads/downloads, resume/append, hashes and off cancellation. All 30
host suites pass; targeted application sanitizers and native integrity checks
also pass. The source archive rebuilds the matching application XEX identically.
The fresh 215-second desktop walkthrough uses the shipping renderer/artwork;
console profile/Guide/storage/FTP services are simulated and labelled.

The 0.48 seek fixes, 0.47 audio/timeouts/episode labels and existing saved paths
are retained, without migration. Complete GPLv3 source/build scripts, pinned
free SDK and original notices/artwork attribution are included. Xbox image
loading, transfer speed and menu responsiveness still need hardware retesting.
