Simple 360 Player v0.2 — Peer Pressure compatibility test

This is the existing third-party player, NOT an integrated Blades media player.
Its default.xex and font assets are unchanged from the upstream v0.2 release.
No DashLaunch, native-video bridge or dashboard sideload helper is required
by this package. Actual Peer Pressure playback is still unverified.

1. Extract the USB ZIP onto your USB drive. Keep the whole Simple360Player
   folder together, including Media/Fonts. Use a separate folder from Blades.
2. Start Simple360Player/default.xex through the file manager you normally
   use for homebrew on Peer Pressure. You can use Blades' File Manager.
3. In the player's drive list open USB 1 (or the USB number containing the
   files), then Simple360Player, then TestVideos. Press A on selected.wmv.
   This is the exact WMV you previously confirmed works in the stock player.
4. Check picture, sound and smooth playback. Press A to pause/resume. Try
   Left/Right for a10-second seek; B returns to the player's browser. The
   samples last about12 seconds, so seek near the start or replay if needed.
5. Also play test360.mp4 from the same folder. It is a small640x360 H.264/AAC
   sample, intended to test another decoding route without starting at720p.
6. If both play, optionally try one of your own720p H.264 files. Upstream says
   1080p H.264 is too heavy; H.265/HEVC, VP9 and AV1 are unsupported.
7. Send Simple360Player/playerlog.txt after testing, and tell me whether each
   sample had video/audio, seeking worked, and playback stayed smooth.
   If it fails before a log appears, report that and what remained on screen.

B leaves playback. Back in the browser exits to the console's configured
default dashboard; automatic return to Blades is not added yet. Use your
normal method to reopen Blades. The app may create settings.ini beside its
XEX; this test folder starts without any saved settings or personal paths.

The aim is to establish that its XEX and decoder run on Peer Pressure BEFORE
porting the engine into Blades or adding an exact-selected-file handoff.
There are no original Xbox music visualisations in this player.

VERIFICATION / LIMITS
USB ZIP contents/checksums/CRC and upstream file equality have been checked.
Both samples fully decode on the host. XEX header inspection shows only XAM
and kernel imports, but does not prove the image loads on Peer Pressure.
No Xbox playback, frame-rate measurement, native surround-output test or
console navigation recording has been performed in this cloud environment.
This standalone test has no Blades artwork/walkthrough release. Blades0.37
and its XEX/artwork/video/USB ZIP remain unchanged.

See THIRD-PARTY-NOTICE.txt, UPSTREAM-README.md, Notices and the complete tagged
source ZIP in Source. Do not discard these when redistributing this bundle.
Delete only this Simple360Player folder to remove the test.
