# Blades 0.47 walkthrough

[Watch/download](../dist/Blades-v0.47-walkthrough.mp4): fresh 215-second 720p30
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
- 01:50–01:53: Continue Watching includes Series Name S1E2 - Episode Name labels; actual
  server playback resumes at 4 seconds. Next Up uses the same series/season/episode format.
- 01:54–01:59: pause, seek forward while paused, resume, then seek backward;
  both timestamps and real decoded picture/audio use the full-item timeline.
- 02:02–02:17: sustained actual playback after the backward seek.
- 02:17–02:22: return, refresh, then Next Up and its next episode.
- 02:25–02:31: actual next-episode playback, pause/resume and return.
- 02:41–02:47: Media Center connection/status controls; password remains hidden.
- 02:49: System blade.
- 02:53–03:04: Video Marketplace media paths; browse USB0/Mixed Media, press X
  directly to Scan Depth, select Unlimited and add the shared folder. There is
  no category screen. Storage is explicitly simulated; filename classification
  uses the production library code.
- 03:06: shared root information shows Music, Pictures and Video.
- 03:12/03:16: Music/Pictures each list their supported file from the shared path.
- 03:20–03:22: Video > Local retains only videos.
- 03:27–03:35: System blade and completion.

The capture waits for actual asynchronous page/seek work before advancing the
scripted controller. Host FFmpeg uses scalar code; native Xenon kernels are
unchanged and audited separately. This is not console performance evidence.
Artwork/font/audio attribution remains in the runtime notices and artwork pack.
