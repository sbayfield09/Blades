# Release delivery

Standing user request: every new release must provide a compiled XEX and a
premade artwork pack through GitHub, with direct download links for both.

Publish these together in `sbayfield09/Blades`:

- `dist/Blades/default.xex`
- `dist/Blades-artwork-pack.zip`, containing `Assets/blades.xzp`, installation
  instructions, source attribution and the artwork payload's SHA-256 checksum.
- Runtime licence notices and updated version/install notes.

Build and verify the XEX, prepare a pack compatible with that build, check ZIP
integrity and checksum, and run the resource/renderer checks if either changes.
An unchanged compatible artwork pack may be included again with a newer XEX.
Publish the pair in one commit and verify the remote branch after pushing.
Give immutable GitHub raw links using that commit so the downloads stay paired.

For this workspace, full project work is on `work`; the separate `xex-upload`
branch holds the public downloadable files. Use a temporary Git index to add
only release artifacts and their instructions/notices, preserving the current
working tree. Do not replace the public branch with unrelated local changes.

The v0.6.0 pack contains 195 XUIZ entries: five blade states, additional menu
icons, the BSK2 root/expanded layout manifest and retail 5766 shared images. Original project by
zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard interfaces.
Asset ownership remains with the respective rights holders. See
`DASHX360-ASSETS.md` for pinned sources, conversion details and remaining visual
differences.
