# Jellyfin and Local video — Blades0.49

The Jellyfin browser shows selected movie/episode primary artwork, runtime and
overview on the right. Thumbnails load in a cancellable low-priority worker
operation with bounded PNG storage and three cached items. No player decoder
changes in0.49; the0.48 seek generation isolation is retained.

Media > Video contains Jellyfin and Local. Media Center manages the Jellyfin
server/login. See [connection setup and limits](JELLYFIN.md). Local retains the
existing scanned-video library and the0.43 decoder/GPU path. New shared media
folders in Video Marketplace are classified by supported file extension; no
category selection or migration of old paths. Music/Pictures playback remains
unimplemented.

Small-seek correctness in0.49: each restart has a unique User-Agent generation,
which isolates Jellyfin's progressive cache filename while preserving the device/
logical playback session. Stop/delete is asynchronous and seek time is not part
of that filename; reuse returned old pictures under a new clock. Frame-coded
actual-content tests now verify fractional and10second seeks, returns and EOF
bounds against the official server. Network EOF margin0.5s; local seek unchanged.

Jellyfin uses authenticated, bounded custom HTTP AVIO and server-negotiated
MPEG2/MP2 MPEG-TS up to720p30. The console remains in the same executable.
A pause/resume, Left/Right10second seek, X fit/stretch, B/Back return to the list.
Continue Watching and Next Up use Jellyfin's account-specific endpoints; opening
any partial video fetches fresh resume ticks. Remote seeking restarts transcoding
at StartTimeTicks within the same session, cancels old opens, resets decoder/audio/
picture queues and maps both clocks to the full-item position. Pause is retained.
Local byte seeking and the verified0.43 codec/GPU path remain unchanged.
Jellyfin media inactivity allows30seconds rather than8, retaining short control
API deadlines and immediate cancellation. Remote opening/probing is bounded at
60seconds. The previous timeout is reproduced by a post-seek ten-second delivery
gap and the fixed reader recovers; genuine30second stalls still fail. Resume/
Next Up episode labels include the series once. Codec kernels and GPU unchanged. Audio FIFO now preserves source gaps/overlaps
and resampler delay, keeping its clock anchored to the seek origin. The flash/
tone regression verifies alignment within100ms initially and after seeking.
The remote AVIO buffer uses FFmpeg2.8's standard32KiB. A64KiB original buffer
can trigger its nonseekable probe/refill assertion after short TCP reads;
the fragmented-stream regression exercises the fixed path. HTTP errors and
incomplete chunk/content-length EOF fail visibly instead of silently ending.

## Decoder changes

The user reports that the same H264 file still plays better in Simple360Player
than Blades0.42. This release ports its actual Xenon H264 chroma/quarter-pixel
motion-compensation algorithms and matches its worker placement, four-picture
buffer, preroll and catch-up policy. See [source comparison](SIMPLE360PLAYER-PIPELINE.md)
for exact sources and remaining SDK/FFmpeg boundaries.

Only the two ported filter objects enable vector instructions. The rest of
FFmpeg retains scalar compilation, no generic PPC/AltiVec dispatch, no VSX,
no vector integer multiply and no G5 cache probe. Each port runs a serialized
self-test against the scalar functions before installing pointers. A failure
leaves scalar routines installed and records the verdict in default.log.

H264 uses four frame workers on3,4,5,2, priority+1. Demux moves off worker5 to1;
video feeding stays2 and audio1. MPEG2/other codecs without frame threading keep
slice fallback. Four picture slots replace three. Audio waits for three ready
pictures (at most2seconds after the first picture) before normal start, with
the existing bounded-queue escape for a genuine audio-only lead. Rendering
chooses the newest picture due within5ms, replacing the former35ms tolerance.

When late by40ms, skip deblocking on non-reference pictures; over250ms, also
skip decoding non-reference pictures. Retain the level until back on time,
then restore full decoding/filtering. Pictures over500ms late may be discarded
but one remains eligible at least every500ms. This matches upstream's adaptive
policy instead of doing all decode work and merely discarding its output.
Seek/reopen reset the policy. Frame ownership, pause, stop and buffer bounds stay.

video-perf records decoded/presented FPS, decode/preparation cost, lag_ms,
late_presented, audio_preroll, catchup level and catchup_episodes. Startup logs
record the chroma/qpel verdicts and thread layout. Normal UI perf: fps is a
separate measure. Compare steady playback of the same file for15seconds.

## Display and validation

The GPU still converts three independent linear Y/U/V planes and scales to720p.
Native YUV input up to1920x1080 remains full resolution until GPU scaling. Other
pixel formats retain the RGBA/swscale fallback. CPU FFmpeg decodes compressed
video; there is no GPU codec decoder, transcoder or claim of proven1080p30.

The big-endian PPC harness checks ported filters, every subpixel position,
put/average, widths, alignments and destination guards. Its scalar oracle is
separately checked in66,560 cases against FFmpeg. Actual console self-tests
remain enabled. Host playback/ownership/seek/preroll/slow-decoder tests, native
instruction/XEX checks and shader tests complement this; they do not measure
console speed. FFmpeg external libraries are not sanitizer-instrumented.
A fresh desktop GPU walkthrough labels simulated console services. The source
bundle contains the complete GPLv3 combined work and free SDK/build scripts.
Music/Pictures playback and original music visualisations remain unfinished.
