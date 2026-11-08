# Embedded video playback — Blades 0.39

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
frames held by the renderer. The renderer uploads only new frame revisions.
Video is converted to RGBA at up to 1280x720; input above 1920x1080 is rejected.
Decoding uses software, so playable resolution/bitrate depends on the codec
and console performance. Begin hardware testing with the supplied 640x360 samples.
Audio is resampled to 48kHz stereo or six channels through the existing xsdk
output. Short sample ramps smooth pause/stop/underrun edges. The consumed PCM
clock drives video timing; video-only playback uses the monotonic clock.
Seek generations clear packet/audio queues without overwriting displayed frames.
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

The host engine tests exercise actual WMV2/WMA and H264/AAC decoding, RGBA
frames, stereo/six-channel resampling, seek/end/pause, Guide state restoration,
held-frame ownership, rapid cancel/reopen, missing files and retained chooser
selection. ASan/UBSan cover the new application frontend; the separately built
FFmpeg static libraries are not sanitizer-instrumented. Native cross-compilation
and XEX integrity checks also pass. These do not establish Xbox runtime or
frame-rate behaviour. The new embedded Xbox path still needs a console test.

For that test, update from the complete USB ZIP, scan a folder containing
sample.wmv/sample.mp4 from the source archive's tests/media directory, then play,
pause, seek and return with B. If it fails, send Blades/default.log and describe
the last screen/sound. Logs remain local and can contain filenames.

See [source and LGPL notice](VIDEO-LICENSE-NOTICE.txt) and the matching
`Blades-v0.39-source.zip` for complete application/decoder sources, patches,
pinned SDK and rebuild/relink instructions.
