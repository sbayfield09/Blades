# Blades0.42 walkthrough

[Watch/download](../dist/Blades-v0.42-walkthrough.mp4): fresh108-second720p30
desktop GPU capture with48kHz stereo and matching235-entry artwork. The on-video
label distinguishes CPU decoding/GPU YUV display from simulated console services.

The first84seconds show blades/submenus, profile/Guide requests, achievements,
File Manager and FTP. At01:31 the real1080p MPEG2 fixture is decoded on the CPU
and displayed through three GPU planes at720p;01:32/01:33 pause/resume and01:35
return to the list. At01:39 the720p H264/AAC fixture plays through the same path;
01:40/01:41 pause/resume and01:43 return.01:45–01:48 close and switch to System.

These are original synthetic samples, not reference footage. This is not console
performance evidence. The separate controlled40ms-decoder regression tests verify
the timing fix; their artificial delay is absent from the captured/shipping engine.
Artwork/audio attribution is retained in the pack and runtime notices.
