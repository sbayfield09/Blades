# Simple360Player decoder parity — Blades0.43

Pinned upstream: [Simple360Player0.2](https://github.com/HG-ERIK/simple360player/tree/5d235c1fd24c098ccc1b2463c1b4265bf2e7844c).
Preserved sources and hashes are in third_party/simple360player/UPSTREAM.json.

| Component | Simple360Player | Blades0.43 |
|---|---|---|
| Video decoder call | CPU avcodec_decode_video2 | Same API |
| H264 workers | Four, frame threading | Four, frame threading |
| Worker hardware threads |3,4,5,2; above normal |3,4,5,2; priority+1 |
| Demux / video feeder / audio CPU |1 /2 /1 |1 /2 /1 |
| Picture slots | Four | Four |
| Audio preroll | Three ready pictures, at most2s | Same target/timeout after first picture; explicit audio-lead deadlock escape |
| Presentation | Newest picture due within5ms | Same; held picture ownership preserved |
| Catch-up |40ms: skip nonref deblocking;250ms: skip nonref frames | Same thresholds and hysteresis; restore when back on time |
| Very late output | Over500ms: drop, but show at least every500ms | Same time check, measured from actual picture take |
| H264 chroma MC | Custom shift/add VMX, widths8/4 | Actual upstream algorithm ported |
| H264 qpel MC | Custom six-tap VMX, widths16/8 | Actual upstream algorithm ported |
| YUV display | Three linear GPU planes, matrix conversion/scaling | Retained three linear R8 GPU planes |

The filter port changes XDK intrinsics to GCC operations with the same byte/lane
semantics, the split FFmpeg DSP context/const-stride API, and synchronized runtime
self-tests. No generic AltiVec decoder path is enabled. Small blocks/high bit
depths retain scalar dispatch. Arithmetic is checked against an independent
scalar oracle; that oracle is separately checked against pinned FFmpeg routines.
Both ports also compare themselves against scalar routines on the actual console
before use. Log verdict0 means the table was installed; positive means fallback.

Important boundaries: the complete upstream player is built with proprietary
XDK/D3D/XAudio2 and its old FFmpeg0.7-era fork. Blades keeps its xsdk frontend,
FFmpeg2.8.22 container/audio/decoder API and bounded queues. MPEG2 and codecs
without frame threading retain slice-thread fallback. Its incomplete vertical
luma-deblock hook passes vector results by value and writes the original pixels
back; Blades retains the correct scalar filter instead. These are deliberate,
documented differences; the whole decoder binary is not byte-identical.

CPU decoding, GPU colour conversion/scaling: neither player implements compressed
video reconstruction on the GPU or transcodes during playback. Upstream's README
calls1080p H264 too heavy for the CPU; no1080p30 claim is made here. Real performance
parity still requires the same file on the same console. No XDK player binary,
external executable, adapter or DashLaunch plugin is needed. The Blades chooser
and return-to-list behaviour are retained. The combined0.43 executable is GPLv3;
complete corresponding source accompanies it.
