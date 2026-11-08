# Blades0.44 walkthrough

[Watch/download](../dist/Blades-v0.44-walkthrough.mp4): fresh140second720p30
shared desktop GPU renderer capture,48kHz stereo, matching235-entry artwork.
The on-video label identifies the desktop GPU and simulated Xbox services.
The Jellyfin portion talks to an actual isolated official Jellyfin10.10.7 server
with a generated test account and synthetic media; account tokens/passwords are
not displayed. The Xbox Guide, profile data, console storage and FTP are simulated.

-00:00–01:24: blade changes, profile/Guide requests, achievements, File Manager,
  FTP controls and submenu navigation.
-01:29–01:31: Video's Jellyfin/Local folders, then Local's scanned list.
-01:33: real1080p MPEG2 fixture through CPU decoding/GPU YUV scaling; pause/resume
  at01:34/01:35, B returns01:37.
-01:40: real720p H264/AAC fixture; pause/resume01:41/01:42, return01:44.
-01:47–01:50: authenticated Jellyfin video library/folder navigation.
-01:53: actual Jellyfin progressive MPEG2/MP2 stream from synthetic HEVC/AAC
  source through Blades' shipping HTTP AVIO/decoder/display path.01:54/01:55
  pause/resume,01:58 return to the list.
-02:07–02:13: Media Center server/account/status controls; password stays hidden.
-02:15–02:20: System blade and completion.

The local and streamed samples are original synthetic media, not reference
footage. Host FFmpeg uses scalar code; native Xenon kernels are unchanged from
0.43 and verified separately. This is not Xbox hardware or console-speed evidence.
Artwork/font/audio attribution remains in the artwork pack and runtime notices.
