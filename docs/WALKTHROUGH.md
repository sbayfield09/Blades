# Blades 0.40 walkthrough

[Watch/download the MP4](../dist/Blades-v0.40-walkthrough.mp4)

Fresh108-second1280x720 desktop GPU capture at30fps with48kHz stereo AAC and
the matching235-entry artwork pack. The label identifies the desktop GPU and
1080p MPEG2/720p H264 inputs; storage/profile/Guide services are simulated.
No console performance or true1080p display output is demonstrated.

The first84seconds cover blade navigation, profile/Guide requests,
achievements, File Manager and FTP, with console services explicitly simulated.

- 01:24–01:29: navigate to Media > Video.
- 01:31: play the actual native1920x1080p30 MPEG2 fixture using the YUV atlas
  shader, scaled to the1280x720 display surface by the GPU.
- 01:32/01:33: pause/resume; 01:35: B returns to the same Blades list.
- 01:37: select the720p H264 fixture.
- 01:39: actual1280x720 H264/AAC playback using the same GPU YUV path.
- 01:40/01:41: pause/resume; 01:43: B returns to the list.
- 01:45–01:48: close the list and switch to System.

These are original synthetic FFmpeg-generated samples, not uploaded reference
footage. The host uses portable decoder libraries; the new target AltiVec build
and console frame rates need a separate hardware test. Artwork/audio attribution
is retained in the pack and runtime notices.
