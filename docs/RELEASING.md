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

The v0.22.0 pack contains 234 XUIZ entries: five blade states, additional menu
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
