# Blades 0.45 walkthrough

[Watch/download](../dist/Blades-v0.45-walkthrough.mp4): fresh 160-second 720p30
shared desktop GPU capture, 48kHz stereo, matching 235-entry artwork pack.
The video labels desktop GPU rendering and simulated Xbox services. Jellyfin
uses an actual isolated official 10.10.7 server, a generated test account and
synthetic HEVC/AAC movies/episodes; no passwords or session tokens are shown.

- 00:00–01:24: blades, profile/Guide requests, achievements, File Manager,
  FTP controls and submenu navigation. Xbox services are simulated and labelled.
- 01:29–01:31: Video's Jellyfin/Local folders, then the Local list.
- 01:33/01:40: actual local 1080p MPEG2 and 720p H264 samples through CPU decoding
  and GPU YUV display, including pause/resume and return to the Blades list.
- 01:47: Jellyfin root with Continue Watching, Next Up and video libraries.
- 01:50–01:53: Continue Watching, then actual server playback resumed at 4 seconds.
- 01:54–01:59: pause, seek forward while paused, resume, then seek backward;
  both timestamps and real decoded picture/audio use the full-item timeline.
- 02:02–02:07: return, refresh, then Next Up and its next episode.
- 02:10–02:16: actual next-episode playback, pause/resume and return.
- 02:26–02:32: Media Center connection/status controls; password remains hidden.
- 02:34–02:40: System blade and completion.

The capture waits for actual asynchronous page/seek work before advancing the
scripted controller. Host FFmpeg uses scalar code; native Xenon kernels are
unchanged and audited separately. This is not console performance evidence.
Artwork/font/audio attribution remains in the runtime notices and artwork pack.
