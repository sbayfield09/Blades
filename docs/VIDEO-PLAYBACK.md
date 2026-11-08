# Embedded video playback — Blades 0.40

Choose a file in **Media → Video**. Manage scan folders in **Media → Video
Marketplace**. Press A to play that exact file inside Blades' executable.
The chooser remains the Blades layout. B stops playback and reveals the same
list, selection and scroll position. End of file returns there too.

- A: pause/resume.
- Left/Right: seek backward/forward ten seconds.
- X: switch between aspect-preserving fit and stretch.
- Y or Up/Down: show/hide playback controls; they otherwise hide after three seconds.
- B or Back: stop and return to the chooser.
- Opening the Xbox Guide pauses; closing it resumes only if playback was running.

`default.xex` contains the decoder, controls and playback frontend. It uses the
same xsdk display renderer and audio device as the dashboard. It does not launch
another title, attach a Peer Pressure adapter, patch player code, or require
DashLaunch. The release ZIP contains no Players directory. An old 0.38 Players
folder is unused and may be removed manually after updating.

## Implementation

The original Blades frontend wraps an LGPL-only FFmpeg 2.8.22 build. It supports
WMV, MP4/MOV, MKV, AVI, ASF, MPEG/TS and FLV containers with the configured codecs,
including WMV2/3, VC1, H264, MPEG4/2, WMA, AAC, MP3, AC3 and DTS. A matching file
extension does not guarantee that its codec/profile, DRM or damaged contents
are supported. Music/Pictures playback and the original music visualisations
remain unfinished.

File I/O/demux, video decode and audio decode run on workers. Bounded packet
queues and three owned RGBA frame slots keep decoder buffers separate from
frames held by the renderer. The renderer uploads only new frame revisions. For 8-bit YUV420 videos it
copies the native Y/U/V planes into an owned R8 atlas, uploads that atlas and
performs BT601/BT709 limited/full-range colour conversion and scaling in the GPU
shader. A 1920x1080 frame stays 1920x1080 until GPU scaling; the dashboard/display
surface remains 1280x720. Other formats retain the RGBA/swscale fallback at up to
1280x720. Input above 1920x1080 is rejected.

The Xbox FFmpeg build now enables PowerPC/AltiVec decoding and compiler vector
support. VSX, POWER8, PPC4xx and ldbrx are disabled; the xsdk patch bypasses the
G5 dcbzl probe/clear path. Three decoder workers are placed on hardware threads
2,4,0 (one per physical core), with decode submission on3, demux on5 and audio
on1. This replaces the 0.39 scalar build and its2,3,4,5 mapping over two cores.
No fast/approximate decoding flags or H264 deblocking removal are enabled.

At 720p a YUV atlas is1,382,400 bytes instead of3,686,400 bytes of RGBA, a62.5%
reduction in frame upload payload. At1080p it is3,110,400 bytes. Late frames are
skipped before output preparation. This reduces wasted preparation/uploads but
does not guarantee the CPU can decode every H264 stream in real time.

Audio is resampled to48kHz stereo or six channels through the existing xsdk
output. Consumed PCM drives video timing; silent video uses the monotonic clock.
The real stream frame rate supplies timestamp fallback/end timing. Original
pause/seek/stop/Guide behaviour and short audio ramps are retained.

Every five seconds default.log now prints video-perf: actual decoded/presented
frame rates, decode-call and preparation time, late/dropped frames and audio
underrun counts. The separate perf: fps line measures the UI presentation loop,
which can remain60fps while decoded video is slow. Pauses/seeks affect the video
rates, so compare a sustained normal-playback interval.
Errors return to a Blades message instead of switching dashboards.

## Why the adapter was removed

The user confirmed the separate Simple360Player 0.2 test worked. The 0.38
integration then hard-crashed the console. Its uploaded Blades log reaches the
player launch, while the adapter log contains only its first startup message.
That narrows the observed failure to early adapter startup; it does not identify
a proven faulting instruction. Version 0.39 removes that runtime path entirely.
It shares the underlying FFmpeg approach, but links neither Simple360Player's
GPL UI nor its Microsoft XDK executable.

## Validation and limits

The user confirmed0.39 plays720p MPEG2 and WMV well and reported slow/jittery
720p H264. The supplied log shows smooth UI presentation during video; it has no
decoder timing and therefore cannot establish actual decoded frame rates.
The 0.40 changes above address confirmed scalar-build/CPU-conversion costs.

Host engine checks exercise WMV2/WMA and H264/AAC in both RGBA/YUV modes, native
1080p30 MPEG2 and720p H264 frames, pause/seek/end/Guide behaviour, held-frame
ownership, rapid cancel/reopen, missing files and retained chooser selection.
GPU shader comparisons cover1080p YUV, BT601/709 and limited/full range against
an independent scalar reference. A standalone PPC/VMX H264 chroma arithmetic
harness passes all alignments/weights and destination guards under QEMU's7400
model; its temporary ABI save adaptation changes no vector math or production
object. This is not Xbox execution or a speed measurement.
ASan/UBSan cover application code; FFmpeg static libraries are not instrumented.

The new vector/GPU path and1080p30 performance still need console testing.
Use the supplied six-second1080p30 MPEG2 sample (12Mbps target, eight slices,
BT709, MP2 stereo), then retry the same720p H264 file. Send Blades/default.log
after normal playback of at least15seconds; replay the short sample if needed.
The1080p goal is input decoding; true1080p display output is not added here.

Complete corresponding source, FFmpeg patches, SDK and rebuild instructions are
in Blades-v0.40-source.zip. See [LGPL notice](VIDEO-LICENSE-NOTICE.txt).
