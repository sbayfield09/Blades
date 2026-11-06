# Blades 0.37 walkthrough

[Watch/download the MP4](../dist/Blades-v0.37-walkthrough.mp4)

Fresh 84-second 1280x720 desktop GPU capture at 30fps with 48kHz stereo AAC audio.
Profiles, achievements, storage, FTP and native Guide/keyboard requests are
explicitly simulated. This captures the release renderer and matching artwork;
it does not demonstrate console I/O, authentication or native hardware FPS.

- 00:02: highlighted signed-out profile card.
- 00:06: signed-in card with restored pre-0.36 Gamerscore icon position.
- 00:18–00:34: Games, achievements, file browsing and Media.
- 00:35–00:55: System and Computers/FTP, including smoothly moving focus and
  equally spaced status/IP/login/device fields.
- 01:12–01:14: Xbox Live Messages and Friends.
- 01:15: Chat selected by Down from Friends.
- 01:16: Open Tray selected.
- 01:17–01:18: Chat selected by Up from Open Tray.
- 01:19–01:24: Marketplace with the blended tint.

Both Chat approaches settle with the full bottom divider and no downward
shadow. Cache/direct-frame regression tests cover every frame of both approaches,
including the half-pixel rounding boundary that previously retained a lip.
Games and System keep 0.36 tints; Live softens 0.35, Media is slightly darker and
Marketplace is halfway between the two. The stock 360 logo shadow is retained.
Artwork/audio credit: zivvoz / DashX360 and respective rights holders.
