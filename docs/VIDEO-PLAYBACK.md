# Video playback — Blades0.42

Media > Video keeps the existing Blades chooser. Video Marketplace manages scan
folders/depth. A pauses/resumes, Left/Right seeks10seconds, X switches fit/stretch,
Y toggles controls, B/Back returns to the same list; EOF returns too. Guide
opening pauses and closing restores the previous pause state.

## Confirmed timing regression and fix

The user's0.41 log shows H264 at29–33 decoded FPS in several intervals while
presentation falls to0FPS and late-discard counts rise. The GPU first-frame
upload/fence succeeds. This is evidence of a presentation-policy defect, not
proof that every part of decoding is fast enough for every stream.

Audio was allowed to advance during initial decoder buffering. An early-output
rule discarded every picture more than80ms late, so a decoder near the stream's
frame rate could remain permanently behind and produce no new visible pictures.
Two controlled tests reproduce those failures in0.41: PCM before the first image,
and picture freezes with a40ms-per-frame decode budget.

Version0.42 waits for the first picture to be handed to the renderer before
starting normal PCM delivery. The first buffered picture is selected without
requiring an already-running audio clock. It also removes the all-late-frame
discard rule. The renderer still picks the newest due picture and coalesces
older queued ones, while preserving frame ownership and bounded queues.
Late pictures remain eligible for display; they are not all thrown away.

An intentional audio-only lead is handled separately: if the bounded audio queue
blocks demux while video waits for its first packet, PCM is allowed to advance
until that packet can be reached. It then respects the video's timestamp. This
breaks the dependency without using a timeout or showing future video early.
The gate resets after seeking; pause/stop/reopen and prior held frames are tested.

video-perf still reports actual decoded/presented FPS and preparation/decode-call
costs. lag_ms and late_presented now make residual A/V lag visible; audio_preroll
identifies the intentional-leading-audio path. The ordinary perf: fps line measures
UI presentation. Pauses/seeks affect readings; compare sustained normal playback.

## Decoder and GPU work

The compatible CPU decoder and three independent linear Y/U/V GPU textures from
0.41 are retained. GPU colour conversion/scaling stays enabled; no new SIMD,
quality-reduction or non-reference-frame skipping is enabled. MPEG2's decoder
and display path are unchanged. Neither project transcodes/re-encodes playback
or implements compressed-video GPU decoding. See SIMPLE360PLAYER-PIPELINE.md.

Simple360Player's source also contains Xbox-specific shift/add H264 kernels and
adaptive catch-up logic. Those differ from generic AltiVec code and may be useful
for later CPU tuning, but this release fixes the measured timing defect first.
There is no claim that new CPU kernels were ported or that CPU decoding got faster.

Native YUV input up to1920x1080 stays full resolution until GPU scaling; display
output remains1280x720. Other pixel formats retain the swscale/RGBA fallback.
Music/Pictures playback and original music visualisations remain unfinished.

## Validation scope

New startup/slow-decoder tests fail on0.41 and pass with this fix. A test with
five seconds of audio before the first video packet reaches EOF and verifies
that future pictures are not shown early. Artificial decode delay exists only
in the test library and is absent from shipping Xbox compilation.
Host ownership, controls, early B/reopen, held-frame, seek and end tests pass.
GPU shader/renderer checks and native XEX integrity checks are separate from
console testing. ASan/UBSan cover the application; FFmpeg libraries are external.

The user confirmed0.41 MPEG2 playback. The0.42 pacing fix and real H264/MPEG4
smoothness still need a console retest. Try the same long file and send
Blades/default.log after normal playback if it remains slow. Raw uploaded logs
stay private. Complete corresponding source/relink bundle: Blades-v0.42-source.zip.
