Original Blades test fixtures, generated with FFmpeg testsrc/sine sources.
MIT/CC0: these samples may be used, modified and redistributed without restriction.
sample.wmv: 12 seconds, WMV2/WMA2, 640x360, stereo.
sample.mp4: 12 seconds, H264 Constrained Baseline/AAC, 640x360, stereo.
These are synthetic test media, not uploaded dashboard reference footage.

sample1080-mpeg2.mkv: 6-second 1920x1080p30 MPEG2, 8 slices, 12Mbps target,
BT709 limited range, MP2 stereo. testsrc2 + sine; no uploaded footage.
sample720-h264.mp4: 6-second 1280x720p30 H264 High3.1, 3Mbps target,
BT709 limited range, AAC stereo. testsrc2 + sine. These are codec/pipeline fixtures, not a console performance guarantee.

audio-preroll.mkv: synthetic7sec AAC stereo, with160x90 MPEG2 video only
from5sec to6sec. Exercises bounded audio queues before the first video packet.
