# Blades 0.30 FTP server walkthrough

[Watch/download the MP4](../dist/Blades-v0.30-walkthrough.mp4)

Fresh 52-second 1280×720 desktop GPU capture at 30 fps, with 48 kHz stereo AAC
audio and the matching artwork pack. The visible label identifies simulated
FTP status, IP/device counts, profile information and filesystem snapshots.
It uses the release renderer/navigation and demonstrates the Media Center UI;
it is not a console FTP/network or settings-persistence recording. Real disk
transfers and persistence are exercised separately by the host integration test.

- 00:00 Blade startup
- 00:02 Media blade, then Media Center at 00:07
- 00:09 / 00:12 Simulated connections change the device count to one, then two
- 00:14 A turns FTP off; device count clears and the IP remains visible
- 00:16 Back to Media; re-open Media Center at 00:18, still off
- 00:20 A enables FTP; simulated device reconnect at 00:22
- 00:24 Return to Media; Games → Played Games → File Manager
- 00:35 Browse simulated USB files; X refreshes the folder at 00:36
- 00:37 Select App.XEX and request launch at 00:38
- 00:40 Dismiss the desktop launch notice and return through the menus
- 00:46 Return to Media; open Media Center at 00:47, still running

The console build reads/saves its toggle on a worker and serves actual storage.
The walkthrough tool does not start a listening socket, write a user setting or
launch an external app. See FTP.md for connection details and hardware checks.
Artwork/audio credit: zivvoz / DashX360 and respective rights holders.
