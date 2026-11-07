# Selected-file video playback — Blades0.38

Choose the file in **Media > Video**. Manage video scan paths under **Video
Marketplace**, using the existing folder/category/depth/scan controls. Scanning
never starts playback. The selection launches the bundled Simple360Player0.2
straight into that file. Its file browser is not part of this flow. The bundled player.exe.xex and
BladesVideoAdapter.dll.xex use compound filenames so existing scan/File Manager
filters keep these internal components out of launchable app lists.

A pauses/resumes; Left/Right seeks; Y opens playback options. B/Cancel stops and
returns to Blades; EOF returns too. Blades reopens the Videos list without its
intro and restores the selected row once the scan finishes. Existing player
options, subtitle sidecars, track/aspect/brightness controls remain available.

The upstream unmodified player is user-confirmed to run/play on Peer Pressure.
The new adapter, selected-file handoff and return need console testing. If the
browser appears or returning fails, collect `Blades/default.log` and both
`Blades/Players/Simple360Player/playerlog.txt` and `blades-video.log`. A missing
adapter log is useful evidence. No physical Xbox capture is claimed.

## Installation

Extract the complete versioned USB ZIP into the existing install. Keep Assets,
Players and their notices/source. Saved scan folders and FTP credentials are
excluded from the release and preserved on update. No converter, Microsoft XDK,
DashLaunch, global HDD config or stock-dashboard sideload installation is needed.
The player's local `sideload.ini` is confined to Players/Simple360Player.

Separate XEX/artwork updates also need the matching video-player pack. The USB
ZIP already includes it. Existing native-video diagnostics are not used by this
route; the retail dashboard/player is not launched for files selected here.

## Implementation and limits

Blades' worker revalidates the selection, checks player/adapter/fonts/config,
creates the exact autoplay request by atomic rename and publishes a typed video
launch. Cancellation uses the library's existing epoch check. The render thread
hands a1016-byte bounded packet to XAM, including stable physical storage paths,
filename and Blades return XEX. It never opens a second file chooser.

Peer Pressure's official per-title DLL mechanism loads our original adapter
before the player's entry. It maps the selected folder as `bladesvideo:` and
uses the player's existing `player.ini` autoplay feature. A line-injection/path
validator rejects malformed/traversal/overlong requests. Full-method CRCs, base
and execution ID/version guard two redirects inside the player's own XEX:
UpdateBrowser and RenderBrowser. Both return to Blades; no XAM/kernel code is
patched. Only32 bytes of application code change in memory, restored on detach
when still ours. Missing handoff data clears stale autoplay. Playback controls
and decoding are unmodified upstream code. Details/source are included.

Supported container extensions: MKV,MP4,M4V,MP4V,MOV,AVI,WMV,ASF,TS,M2TS,MPG,MPEG,
FLV. Codecs/profile/bitrate still matter. Upstream targets up to720p H.264;
1080p H.264 can exceed CPU capacity. HEVC, VP9 and AV1 are unsupported. The
player uses its own FFmpeg decoder; no retail optional media update is required.
Music/Pictures playback and the original retail music visualisations are not
added by this release. Long/invalid paths fail before title handoff.

The two titles remain separate. Blades is MIT; the player and accompanying
adapter are GPLv3 with complete corresponding sources/notices in Players/Source.
The wire protocol remains MIT-compatible. No Microsoft update/dashboard binary
or original reference-video asset is distributed.

## Validation

New host tests cover exact request/return packets, XAM's size limit, device
translation, root/spaces filenames, path/config-injection rejection, atomic
replacement for a second selection, incomplete/wrong helpers, and Videos row/
scroll/chrome restoration after return. The existing library tests cover
cancellation, deleted files and no autoplay on scan. Actual upstream decoded
application methods verify both full-method CRCs; mutations, truncation and
wrong base are refused, and both32-byte changes restore exactly. Native XEX/
adapter page/import/header digests validate. User-confirmed standalone playback
does not establish this new adapter's runtime behaviour.
