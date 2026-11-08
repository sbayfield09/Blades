# Blades 0.50 walkthrough

[Watch/download](https://github.com/sbayfield09/Blades-reimagined/releases/download/v0.50.0/Blades-v0.50-walkthrough.mp4): fresh 240-second 720p30
shared desktop GPU capture, 48kHz stereo, matching 235-entry artwork pack.
The video labels desktop GPU rendering and simulated Xbox services, including updater states. Jellyfin
uses an actual isolated official 10.10.7 server, a generated test account and
synthetic HEVC/AAC movies/episodes with baked source frame counters; no passwords or session tokens are shown.

- 00:00–01:24: blades, profile/Guide requests, achievements, File Manager,
  FTP controls and submenu navigation. Xbox services are simulated and labelled.
- 01:29–01:31: Video's Jellyfin/Local folders, then the Local list.
- 01:33/01:40: actual local 1080p MPEG2 and 720p H264 samples through CPU decoding
  and GPU YUV display, including pause/resume and return to the Blades list.
- 01:47: Jellyfin root with Continue Watching, Next Up and video libraries.
- 01:50–01:53: selected episode artwork, overview and runtime on the right.
  Continue Watching includes Series Name S1E2 - Episode Name labels; actual
  server playback resumes at 4 seconds. Next Up uses the same series/season/episode format.
- 01:54–01:59: pause, seek forward while paused, resume, then seek backward;
  the baked source frame counter visibly changes with the seek, alongside the
  full-item playback timeline. The server source encodes its frame number in
  eleven luma bars for independent actual-content checks.
- 02:02–02:17: sustained actual playback after the backward seek.
- 02:17–02:22: return to the episode details, refresh, then Next Up and its
  next episode artwork/overview/runtime. The server uses generated local images.
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

The capture uses the final corrected player. The official-server setup first
verifies actual content on fractional/one-second/ten-second seeks and12seconds
of continuous content/clock agreement. The capture waits for actual asynchronous page/seek work before advancing the
scripted controller. Host FFmpeg uses scalar code; native Xenon kernels are
unchanged and audited separately. This is not console performance evidence.
Artwork/font/audio attribution remains in the runtime notices and artwork pack.

- 03:29–03:49: System > Update Blades, simulated version check, download progress,
  cancellation/retry and verified ZIP location. The real HTTPS worker is tested
  separately with trusted/untrusted servers; no fake public Release is created.
- 03:51–04:00: return through Media and System. Media highlights/fades throughout
  this recording use the production animated list path.

The recording cannot demonstrate console startup/exit static or hardware updater
transport. Those require an Xbox retest.
