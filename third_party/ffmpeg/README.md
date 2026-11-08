# Embedded decoder — Blades0.43

Pinned FFmpeg2.8.22: https://github.com/FFmpeg/FFmpeg/tree/n2.8.22
Archive SHA256:9f2ed903682524fbfb6999f2bb106f02219df1393fece58cae9da1a1ef780358

xenon/simple360_chroma.c and simple360_qpel.c port the actual Simple360Player
0.2 shift/add H264 motion-compensation algorithms. The preserved originals,
GPLv3 licence, pinned source hashes and scheduling references are in
../simple360player. The combined executable is GPLv3. Original FFmpeg LGPL
notices remain; configure enables GPL/version3 and disables nonfree code.

Only these two objects compile with -maltivec. Generic FFmpeg ARCH_PPC,
HAVE_ALTIVEC and compiler auto-vectorisation remain disabled; the port uses
an explicitly audited integer shift/add/permute subset without vector integer
multiply, VSX, POWER8 or a G5 cache probe. SDK memcpy remains separate.
Before installing either function table, a serialized once-per-process
self-test compares the compiled filters to the initialized scalar routines.
A mismatch leaves the scalar table installed; the verdict is logged.
8-bit H264: chroma8/4, qpel16/8; smaller blocks and higher bit depths stay C.

xsdk.patch hooks only these dispatch tables, places frame workers3,4,5,2
at priority+1, supplies a monotonic seed and excludes the G5 dcbzl probe.
GPU conversion/scaling stays independent of compressed video decoding.
The older upstream0.7 decoder and its incomplete vertical-deblock hook are
not substituted wholesale; see docs/SIMPLE360PLAYER-PIPELINE.md for boundaries.

The source bundle includes every ported/preserved file and the GPLv3 licence.
tools/build-video-ffmpeg.py copies xenon into the extracted decoder tree and
builds it offline. tools/verify-video-decoder.py verifies all five libraries.
