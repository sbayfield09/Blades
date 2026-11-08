Blades0.40 original1080p30 MPEG2 test clip

Six seconds,1920x1080 progressive30fps, MPEG2 video, eight slices,12Mbps target,
BT709 limited range, MP2 stereo48kHz. Synthetic FFmpeg testsrc2/sine; MIT/CC0.
Copy into a Video scan folder and play in Blades. It is also included in the
source bundle as tests/media/sample1080-mpeg2.mkv.
This is a test for the1080p30 decoding goal, not proof of console speed.
The display surface remains720p; the GPU scales the full1080p YUV input.

For useful performance evidence, also retry a long720p H264 file, let it play
normally for15seconds, then send Blades/default.log. Look for video-perf lines,
which measure actual decoded/presented frames; perf: fps only measures the UI.
