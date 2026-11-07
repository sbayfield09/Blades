# Blades video adapter

Original GPLv3 title DLL for Peer Pressure, scoped to the unmodified
Simple360Player0.2 XEX (SHA256
`0f8936175281600d897f1182467419d4ae45181370a0ebea24006267db96ae6d`).
It uses Peer Pressure's documented **local sideload.ini** mechanism. The runtime filename is player.exe.xex; XEX bytes are unchanged.
No DashLaunch, stock-dashboard plugin, system DLL or kernel/XAM hook is used.

Blades passes a byte-defined1016-byte XAM launch-data packet containing a
physical video folder, filename and return XEX. It writes the player's existing
`player.ini` autoplay setting atomically, with no test-key injection. At title
attachment the adapter consumes/validates the launch data and creates a
`bladesvideo:` alias for that exact physical folder. Upstream's own autoplay
code opens the selected file before drawing its browser. Sidecar subtitles
remain in the same aliased folder.

The two entry redirects are **PlayerApp::UpdateBrowser** and
**PlayerApp::RenderBrowser**, in the application's own image. Full-method CRCs,
image base and execution ID/version guard both methods before any write.
No partial original prologue executes and no C++ objects cross the adapter ABI.
Both entries return to Blades rather than choose or draw another file. This
covers explicit B/Cancel, EOF and playback errors; RenderBrowser also prevents
a browser flash if a worker ends playback between Update and Render. Standard
pause/seek/subtitles/audio/aspect controls remain in the original player.

Return clears the owned autoplay file, mounts the original Blades folder and
passes a distinct return packet. Blades skips its intro, reopens Media/Video
and restores the selected item after scanning. Only our own32 bytes of player
code are changed in memory. They are restored on DLL detach if still ours.
Unknown/modified images are refused. A missing/invalid handoff clears stale
autoplay and leaves the original player methods untouched.

`blades-video.log` is local beside the player and does not record selected
paths. The original `playerlog.txt` may contain local filenames; neither is
shipped. Actual adapter attachment, aliases, lifecycle and return-to-Blades
still require a console test. User confirmation establishes that unmodified
Simple360Player0.2 itself runs on Peer Pressure, not this new integration.

## Build

The adapter needs Python3, the open-source xsdk at commit
`5f6dc726f65f6d171493a3628c2415c03551f27b` and its PowerPC GCC toolchain.
It does not need the Microsoft XDK or a rebuilt player. From the source root:

```
python3 tools/simple360-adapter/build_helper.py --sdk /path/to/xsdk --toolchain /path/to/xsdk-gcc --output build/adapter
python3 tools/verify_xex.py --title-dll build/adapter/BladesVideoAdapter.xex
```

The helper uses the shared MIT-licensed wire format in include/video_handoff.h
and src/core/video_handoff.c, the MIT xsdk DLL writer and kernel import stubs.
The combined adapter is distributed under GPLv3; shared MIT notices are kept.
Blades communicates with the separate player title through launch data/files;
its MIT application remains separate from the GPL player.

Official sideload contract:
https://github.com/grimdoomer/Xbox360PeerPressure/wiki/Softmod-Features#dll-sideloading
Upstream player/source:
https://github.com/HG-ERIK/simple360player/tree/5d235c1fd24c098ccc1b2463c1b4265bf2e7844c

Optional build-time hook verification uses an independently decoded copy of
that open-source upstream XEX and test_hooks.py. It checks only application
code and never requires Microsoft's stock dashboard/update files.
