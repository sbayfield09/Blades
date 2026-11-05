# Release delivery

Standing user request: every new release must provide a compiled XEX, premade
artwork pack and navigation walkthrough video through GitHub, with direct
links for all three.

Publish these together in `sbayfield09/Blades`:

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

The v0.10.0 pack contains 213 XUIZ entries: five blade states, additional menu
icons, the BSK2 root/expanded layout manifest, retail 5766 shared images and the local font atlas/metrics/notice, plus eight precomposed ABXY prompt sprites, six transition WAVs and an audio notice. Original project by
zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard interfaces.
Asset ownership remains with the respective rights holders. See
`DASHX360-ASSETS.md` for pinned sources, conversion details and remaining visual
differences.
