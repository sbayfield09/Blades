# Release delivery

Current user instruction supersedes Releases-only delivery: publish matching
versioned USB ZIPs, XEX, artwork, source bundle, walkthrough and dist/update.json
in the repository's main branch, where downloads were previously uploaded.
The updater reads raw.githubusercontent.com/sbayfield09/Blades-reimagined/main/dist/.
Keep browsable application source on main. The repository rename is verified
and origin now points to sbayfield09/Blades-reimagined. Retain historical files.
tools/prepare-update.py verifies files and generates the manifest/checksums;
commit all matching source/artifacts/manifest together. Never overwrite an
already-published versioned download. GitHub Releases may be optional mirrors;
they are not required for updates. Provide immutable matching download links.

The user requires every new release to include the compiled Xbox 360
`default.xex`, a premade artwork pack, and a navigation walkthrough video,
uploaded to `sbayfield09/Blades`. Provide direct GitHub links for all three.
Prepare the artwork pack here; do not require the user to run a converter.
Record the video from that release's renderer and matching artwork pack,
showing blade changes and submenu navigation. Label desktop captures as such.
Preserve artwork attribution.
Also publish a versioned USB-ready ZIP containing Blades/default.xex and the
complete Blades/Assets folder, with notices and installation notes. Users
should only need to extract that ZIP onto a USB drive. Exclude local scan paths
and logs. Keep providing the matching walkthrough and separate XEX/artwork links.

See `docs/RELEASING.md` for packaging and verification. Keep unrelated local
changes intact when publishing release artifacts.
