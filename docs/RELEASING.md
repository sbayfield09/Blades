# Release delivery

Standing user request: every new release must provide a compiled XEX, premade
artwork pack and navigation walkthrough video through GitHub, with direct
links for all three.

Publish these together in `sbayfield09/Blades`:

- `dist/Blades-v<version>-USB.zip`: a complete USB-ready `Blades/` folder with
  default.xex, the artwork payload in Assets/, licences and installation notes.
  Exclude local Config/library-paths.txt and logs. Lead the download links with
  this combined ZIP so installation requires one extraction.
- `dist/Blades/default.xex`
- `dist/Blades-artwork-pack.zip`, containing `Assets/blades.xzp`, installation
  instructions, source attribution and the artwork payload's SHA-256 checksum.
- `dist/Blades-v<version>-walkthrough.mp4`, showing the release's blade and
  submenu navigation with its matching artwork pack and navigation sounds.
- Runtime licence notices and updated version/install notes.

Build and verify the XEX, prepare a pack compatible with that build, check ZIP
integrity and checksum, and run the resource/renderer checks if either changes.
An unchanged compatible artwork pack may be included again with a newer XEX.
Record a fresh walkthrough from the release's renderer with its matching pack;
do not reuse footage from an earlier version. Label desktop captures as such.
The local `tools/render-walkthrough.c` captures the shared navigation/rendering
code and audio; adjust its navigation sequence when menu structure changes.
Verify the resulting video decodes completely and has the expected duration,
resolution and audio. Publish all three in one commit and verify the remote
branch after pushing. Give immutable GitHub links using that commit so the
downloads stay matched.

For this workspace, full project work is on `work`; the separate `xex-upload`
branch holds the public downloadable files. Use a temporary Git index to add
only release artifacts and their instructions/notices, preserving the current
working tree. Do not replace the public branch with unrelated local changes.

The v0.37.0 pack contains 235 XUIZ entries: five blade states, additional menu
icons, the BSK2 root/expanded layout manifest, retail 5766 shared images and the local font atlas/metrics/notice, plus eight precomposed ABXY prompt sprites, six transition WAVs and an audio notice, the baked Xbox Live panel background, and four startup shutter/reveal/layout entries, plus sixteen disc-reading indicator frames. Original project by
zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard interfaces.
Asset ownership remains with the respective rights holders. See
`DASHX360-ASSETS.md` for pinned sources, conversion details and remaining visual
differences.

Version 0.15 also provides headless desktop GPU playback: configure with BLADES_GPU_CHECKS=ON and build blades-gpu-walkthrough. Its drawing plan and GLSL sources are shared with the Xbox GPU renderer. Label that capture as desktop GPU playback.

Version 0.20 adds configurable library folders and title launching; its 234-entry
artwork payload is unchanged and compatible with the new library views. See
LIBRARY.md for setup and hardware-validation scope. Never package a local
Config/library-paths.txt containing machine-specific locations.

Version 0.21 adds per-folder scan depth, live current-path progress and stop,
explicit empty-scan completion, display-label cleanup and ID-less XEX discovery.
The artwork payload remains unchanged.

Version 0.22 filters compound XEX names before reading their contents, adds the
File Manager below Homebrew and Y to remove saved scan paths, puts Unlimited
first, retains Scan Now results while focused, and removes footer status text.
The 234-entry artwork payload is unchanged.

The v0.22 release also adds CD/DVD/data-disc recognition and explicit manual
system-player handoffs. `tools/package-usb.py` verifies and combines the XEX
and premade pack, and is called by the native build script.

Version 0.23 fixes media-player preflight on titles without a Flash: mapping,
using the native Flash device path and the SDK-created XAM launch alias.
The compatible 234-entry artwork payload is unchanged.

Version 0.24 adds native Guide sign-in, a selectable signed-out account card,
and active-controller profile names, gamer pictures and real card totals.
Metadata/picture reads and confirmed sign-out run on a worker; the renderer
keeps presenting through the Guide. Do not package or persist user profile
snapshots, gamer pictures or credentials. See PROFILE-INTEGRATION.md.
The matching fresh walkthrough labels its profile/Guide events as simulations;
it cannot demonstrate real console authentication. The 234-entry artwork
payload is unchanged.

Version 0.25 corrects the signed-out account card to use the supplied reference's
green default tile and Profiles Found count, and uses stock GScore.png for the
Gamerscore label. Both resources were already in the pack; the 234-entry
payload is unchanged. Publish a fresh matching walkthrough and combined USB ZIP.

Version 0.26 adjusts the GScore canvas size, vertical centre and left overhang
against the supplied reference and Convection ink bounds. The original PNG
and complete 234-entry artwork payload are unchanged. Include a newly rendered
walkthrough showing the corrected signed-in card.

Version 0.27 adds the profile-backed achievement browser and sign-in gate.
Use the actual uploaded achievement video and photo for the grid/list/detail
layout, and retain the simulation label in desktop walkthroughs. The console
adapter queries real local XAM data; preview fixtures are confined to tests
and the capture tool. Do not package user history, achievement pictures, profile
files or local scan paths. The same 234-entry artwork payload is compatible.

Version 0.28 replaces custom achievement detail/list panels with native Guide
handoffs and adds thin game-icon separators. Its 234-entry payload is unchanged.
The fresh 54-second walkthrough must label Guide requests as simulated and
show request notices; it cannot capture the real console Guide on desktop.
Publish the matching USB ZIP, separate XEX/pack/video and Guide API audit.

Version 0.29 lists all regular files in File Manager, while keeping only plain
XEXs launchable. Folder selection for scan paths remains directories only.
The 234-entry payload is unchanged. Include a fresh walkthrough demonstrating
ordinary-file no-op selection and XEX launch requests with simulated storage,
plus the matching XEX, premade pack and USB ZIP.

Version 0.30 adds auto-start plain FTP and the Media Center status/on-off page.
Preserve user Config/ftp-settings.txt and .tmp files by excluding them from
all release archives. Default is on; a saved off value must be respected before
listening. The 234-entry payload is unchanged. Include a fresh walkthrough
labeling FTP status/device/IP values as simulations, plus the actual loopback
integration/sanitizer results and the XEX/USB ZIP/premade pack.

Version 0.31 corrects profile counts/current gamer tiles, native Guide account
actions, achievement unlock flags and tab alignment, centred highlights and
Xbox Live text bounds. FTP moves to System/Computers and gains displayed,
editable/persistent login fields. Add audio lifecycle priming/fade/drain.
The 234-entry payload is unchanged. Ship a fresh labeled walkthrough and
exclude personal FTP settings, credentials, scan paths and logs from archives.

Version 0.32 moves the FTP login display to the right status column and refits
menu/account focus sheen and inset cap shadows from the supplied videos.
The upper-bar shadow fades upward inside the bar; the footer shadow fades
downward inside its upper edge. Both use cached column edges from the existing
artwork, including curved ends and animated cap positions. Keep the original
achievement spacing and all button dividers. The 234-entry payload is unchanged;
record a fresh labeled walkthrough and publish the four matching downloads.

Version 0.33 aligns row fills/dividers/shadows and curves the profile-card
shadow to its two lower corners, with clean side fades. Preserve the menu
spacing/tint and cap shading from 0.32. Ship a new labeled video, XEX, USB ZIP
and premade pack; the 234-entry payload is unchanged.

Version 0.34 adds the reflected matching upper group shadow for non-first
visible selections and restores the divider above the first button. Retain the
0.33 lower/card shadows and artwork payload. Ship a fresh labeled walkthrough,
XEX, USB ZIP, premade pack and first/lower-row comparison captures.

Version 0.35 aligns the top of menu focus/upper shadow with the preceding rule,
rounds interpolated cursor pixels, restores FTP cursor animation and equalizes
FTP status-field spacing. Retain the lower/card shadows and 234-entry payload.
Ship a fresh labeled walkthrough and all four matching downloads.

Version 0.36 restores rounded Xbox Live side edges bounded by the last button
divider; Chat has only that divider below it. It also bottom-aligns GScore and
refines all five tints from the supplied references. The 235-entry payload
retints 25 derived masks and adds the original 360-logo alpha shadow. All other
entries/notices remain unchanged. Publish the matching new pack/USB ZIP and
fresh five-blade/Live/branding video; do not reuse the older 234-entry pack.

Version 0.37 applies the user's tint feedback to Marketplace, Live and Media,
retaining Games/System0.36. Restore the pre-0.36 GScore y148 placement and fix
the cursor-cache rounding mismatch that retained a lower Chat shadow on Down.
The 235-entry pack changes only 15 tint masks from 0.36; alpha/dimensions,
stock logo/shadow and all other entries/notices remain unchanged. Publish a
fresh 84-second video showing Down to Chat and Up from the tray, plus all four
matching downloads and cache regression/asset audit evidence.

Version0.38 packages the unmodified user-tested Simple360Player0.2, its runtime
fonts and an original local Peer Pressure title adapter under Players. Include
both complete upstream source and adapter source with GPL/dependency notices.
The pinned upstream source/XEX come from the existing third-party diagnostic
artifacts; tools/package-video-player.py validates their SHA256. Do not ship
player.ini, settings.ini, playerlog.txt, blades-video.log or temporary requests.
The complete USB ZIP includes Players. Publish Blades-video-player-pack.zip
alongside separate XEX/artwork downloads so manual updates remain complete.
Capture the current shared renderer; label desktop player handoffs as simulated.


Version 0.39 embeds LGPL-only FFmpeg directly in Blades. Run tools/build-xbox.sh
with XSDK_ROOT/XSDK_TOOLCHAIN; it builds the pinned decoder then the single XEX.
The USB ZIP must contain no Players files or sideload.ini. Existing historical
0.38 downloads are retained for provenance, not used by 0.39. Publish the complete
Blades-v0.39-source.zip with application source, FFmpeg tarball/patches, SDK source,
licences and BUILDING.txt. Keep its download link in the runtime LGPL notice.
Record real host sample playback in the walkthrough, labelled desktop capture;
storage/profile/Guide events remain simulated. Do not claim console validation.
