# Simple 360 Player 0.2: Peer Pressure compatibility test

User selected [HG-ERIK/simple360player](https://github.com/HG-ERIK/simple360player)
after the native retail player handoff still displayed the previous error.
This test establishes whether an existing Xbox decoder/player runs on Peer
Pressure before changing Blades' media integration.

Use the unmodified [v0.2 release](https://github.com/HG-ERIK/simple360player/releases/tag/v0.2),
tagged commit `5d235c1fd24c098ccc1b2463c1b4265bf2e7844c`.
The XEX SHA256 is `0f8936175281600d897f1182467419d4ae45181370a0ebea24006267db96ae6d`.
Runtime fonts match the same tagged source. XEX header inspection finds only
XAM and kernel imports; this is encouraging but does not establish console
compatibility. The encrypted XDK image was not rebuilt or patched. The xsdk
image verifier does not apply, and no full image-hash validation is claimed.

The [complete USB ZIP](../dist/diagnostics/simple360player-v0.2-peer-pressure/Simple360Player-0.2-PeerPressure-Test-USB.zip)
contains the XEX, original fonts, two small sample videos, instructions, licence
notices and the complete upstream tagged source archive. The [source ZIP](../dist/diagnostics/simple360player-v0.2-peer-pressure/Simple360Player-0.2-source.zip)
is also separately published alongside the executable. Preserve GPLv3,
FFmpeg/dependency notices and Inter's font licence. Existing Blades code and
its MIT licence are unchanged; no GPL player code has been linked into Blades.

Extract onto USB, start `Simple360Player/default.xex` from the usual Peer
Pressure-compatible homebrew launcher or Blades File Manager, then browse
USB 1/2/3 → Simple360Player → TestVideos. Play selected.wmv first, then
test360.mp4. The WMV is byte-identical to the sample the user confirmed works
manually in the stock player. The MP4 is a new H.264 Constrained Baseline
640x360/yuv420p clip with AAC stereo44100Hz; both fully decode on the host.
Neither has yet been tested in this player on Peer Pressure. Try pause/resume
(A), short seeks (Left/Right), then B to leave playback. After both samples,
a user's own720p H.264 file is an optional throughput check.

Collect `Simple360Player/playerlog.txt` and visible picture/audio/seek results.
If loading fails before a log appears, report that plus the screen outcome.
The player creates settings.ini locally; no saved settings, personal paths,
logs, firmware, stock-player payloads or original Blades reference videos are
included. The source archive retains upstream sample configuration.

Upstream cautions that1080p H.264 is too heavy for software decode and does not
support HEVC, VP9 or AV1. It contains no retail music visualisations. It builds
with XDK2.0.21256/VS2010 SP1, while Blades uses xsdk; adapting its decoder,
audio/threading and D3D layers requires porting. Current startup opens its
browser, with no Blades-selected-file input or automatic return to Blades.
Those features are not claimed by this unmodified compatibility test.

This is not a new Blades release. Regular Blades0.37 XEX/artwork/walkthrough/USB
artifacts remain unchanged. No Xbox capture is possible without the hardware;
no player-navigation recording or console playback measurement is claimed.
The test's complete assets are bundled; its [validation report](../dist/diagnostics/simple360player-v0.2-peer-pressure/validation.json)
records provenance, checksums, archive checks and host sample decoding.
