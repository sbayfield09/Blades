# Release delivery

Current user instruction: publish new USB ZIPs, XEX, artwork, source bundle and
walkthrough as assets of a new GitHub Release, rather than adding new binary
versions to dist on main. Keep browsable application source on main. Target
repository name is sbayfield09/Blades-reimagined; perform the requested GitHub
rename when administration access is available, then update origin. Retain
historical release files already committed. tools/publish-release.py prepares
the update.json manifest and verifies draft assets before publishing. Do not
claim a Release or rename happened until GitHub confirms it.

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
