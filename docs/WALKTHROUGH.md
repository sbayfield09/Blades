# Blades 0.39 walkthrough

[Watch/download the MP4](../dist/Blades-v0.39-walkthrough.mp4)

Fresh 108-second 1280x720 desktop GPU capture at 30fps with 48kHz stereo AAC,
using the release renderer and matching 235-entry artwork pack. The on-video
label distinguishes real embedded sample playback from simulated storage,
profile and Guide services. This is desktop footage; new Xbox playback remains
unverified.

The first 84 seconds cover blade navigation, profile/Guide requests,
achievements, File Manager and FTP. Console services are simulated and labelled.

- 01:24–01:29: navigate to Media, then Video.
- 01:31: A opens a synthetic WMV through the actual embedded decoder.
- 01:32/01:33: pause/resume; 01:35: B returns to the same Blades chooser.
- 01:37: select the second item.
- 01:39: play the MP4 through the embedded H264/AAC decoder.
- 01:40/01:41: pause/resume; 01:43: B returns to the list.
- 01:45–01:48: close the list and switch to System.

Both samples are original FFmpeg-generated test media in the source archive,
not reference footage. This capture tests selection, host decode/audio and
Blades GPU rendering together. It does not establish console codec performance,
physical controller/display/audio behaviour or Guide overlay behaviour.
Artwork/audio attribution remains in the matching pack and runtime notices.
