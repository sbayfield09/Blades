# Video playback — Blades 0.41

Media > Video keeps the Blades chooser and opens the exact selected file.
Media > Video Marketplace manages scan folders/depth. There is no autoplay
while scanning. A pauses/resumes, Left/Right seeks ten seconds, X changes
fit/stretch, Y toggles controls, B/Back returns to the same list. Guide opening
pauses and closing restores the previous pause state. EOF returns to the list.

## CPU decode, GPU display

FFmpeg decodes the compressed bitstream on the CPU. Common8-bit YUV420 frames
are copied into owned frame slots and uploaded to three independent linear R8
textures. A simple GPU matrix shader converts YUV to RGB, and texture filtering
scales the picture. This follows Simple360Player's CPU/GPU split; neither project
implements compressed-video GPU decoding. No video is re-encoded/transcoded.
See [upstream source comparison](SIMPLE360PLAYER-PIPELINE.md).

Native YUV input up to1920x1080 is retained until GPU scaling; the display surface
remains1280x720. At720p YUV has1,382,400 bytes instead of3,686,400 RGBA bytes.
Other decoded pixel formats keep the CPU swscale/RGBA fallback. Music/Pictures
playback and original music visualisations remain unfinished.

## Changes after the 0.40 crash

The user reported audio without picture followed by a hard crash. The supplied
log shows decoder initialization, subsequent return/reopen input, and no completed
frame-upload diagnostic or crash address. That does not identify the exact fault.
Version0.40 changed generic AltiVec decoding, worker placement and YUV GPU output
together, so neither host arithmetic nor those logs isolate one proven cause.

Version0.41 removes the unverified generic AltiVec decoder paths and disables
compiler auto-vectorization inside FFmpeg. Decoder objects are checked for vector
instructions. Four workers return to the earlier2,3,4,5 placement, decode submission
to2, demux to5 and audio to1. The SDK's separately tested memory-copy code remains.
The codec/output quality and frame ownership rules are retained.

GPU conversion is kept as requested. The single tiled atlas texture is replaced
with separate linear Y/U/V textures and a simpler matrix shader, following the
standalone player's layout. CPU frame storage still uses compact owned slots;
GPU uploads respect the source plane strides and the previous GPU fence. All
three textures are counted, invalidated and freed together in the renderer cache.
There is no external player title, adapter, DashLaunch or code patch.

First-frame logs now identify input-header completion, initial codec flush,
first decoded/prepared frame, delivery to the renderer, texture upload, submission
and completion of the first GPU fence. video-perf still records actual decoded/
presented FPS, decode-call/preparation costs, dropped/late frames and audio underruns.
The ordinary perf: fps line measures the UI presentation loop. Pause/seek changes
those measurements; collect normal sustained playback for comparison.

## Checks and console test

Host checks cover RGBA/YUV playback, native1080p MPEG2 and720p H264 frames,
control/seek/end behaviour, early B/reopen with a held previous frame, cancellation
and missing files. Desktop GPU comparisons verify colour/range and plane sampling.
The actual compiled Xbox shader is separately checked against scalar colour math
and all three sampler slots with xsdk's microcode interpreter. These do not execute
console texture hardware or establish console frame rates.

This compatibility revision still needs a console test. Start with the same720p
MPEG2 file that worked in0.39, then H264. If it fails, send Blades/default.log and
which screen/audio appeared. Logs remain private and may contain filenames/profile
identifiers; the release contains none of those logs. The1080p30 goal is unproven.
Do not infer full GPU codec decoding from GPU colour conversion.

Complete source/FFmpeg patches/SDK/build instructions are in Blades-v0.41-source.zip.
See [LGPL notice](VIDEO-LICENSE-NOTICE.txt).
