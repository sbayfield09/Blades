# What Simple360Player runs on the GPU

The inspected upstream source is Simple360Player0.2, commit
5d235c1fd24c098ccc1b2463c1b4265bf2e7844c.

- [ffglue.c](https://github.com/HG-ERIK/simple360player/blob/5d235c1fd24c098ccc1b2463c1b4265bf2e7844c/src/ffglue.c#L319)
  calls avcodec_decode_video2 on the CPU. Native YUV420 frames are passed through;
  other pixel formats are converted to YUV420 with swscale.
- [FFPlayer.cpp](https://github.com/HG-ERIK/simple360player/blob/5d235c1fd24c098ccc1b2463c1b4265bf2e7844c/src/FFPlayer.cpp#L1063)
  creates three linear L8 textures, copies Y/U/V rows while respecting GPU use,
  then uses a pixel shader with a colour matrix and texture filtering to convert
  and scale for display. Its video workers occupy hardware threads2–5.
- Its README explicitly describes1080p H264 as too heavy for the CPU. It does
  not provide a compressed-video GPU decoding backend.

Blades0.41 follows that split using its own original xsdk frontend and LGPL
FFmpeg build: CPU decoding, three linear R8 GPU textures, matrix conversion and
GPU scaling. There is no re-encoding/transcoding during playback. Full GPU codec
reconstruction/bitstream decoding is not implemented by either player.
The old generic AltiVec configuration is removed; the GPU display work is kept.
GPU runtime and performance in this revised Blades path still need console testing.
