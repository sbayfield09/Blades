# Blades 0.50

[Complete USB ZIP](../dist/Blades-v0.50-USB.zip) ·
[XEX](../dist/Blades/default.xex) · [Premade artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.50-walkthrough.mp4) ·
[Complete source](../dist/Blades-v0.50-source.zip)

Downloads and updater metadata are published in the normal dist folder on main
in sbayfield09/Blades-reimagined. The updater reads dist/update.json and fetches
the matching versioned USB ZIP directly via raw.githubusercontent.com. GitHub
Releases/API access are not required. Source is browsable on main. Install this
first repository-updater build manually; subsequent versions use the manifest.

System > Update Blades checks the repository manifest and shows installed/
available versions, progress and the saved ZIP path. Downloads use verified
HTTPS, bounded allowed HTTPS redirects and exact SHA256/length checks. X cancels;
failed partial files are removed. The verified ZIP is saved beside the app in
Updates. Exit Blades before extracting it manually. The updater does not replace
the running XEX, modify Config files, install plugins or change the dashboard.
Public download access requires no account or token. See UPDATES.md.

Media list selections, including Local, Jellyfin subfolders and media scan-folder
menus, now use the same animated highlight cursor as normal buttons. Scrolling
keeps it within the visible rows. Folder/page changes fade content in without
changing the established layout or resetting fades for artwork/status updates.
Music/Pictures menus retain their existing playback capability and supported
extension filtering; this release does not add new music/photo players.

Startup establishes initialized black presentation buffers before loading
artwork resources. Shutdown presents/drains black frames before workers and
renderer resources are released. This addresses a visible allocation/cleanup
window and adds display-handoff diagnostics. The final SDK/XAM restoration is
unchanged; actual launch/exit static still needs a physical Xbox test. No kernel,
firmware, HDMI-mode or XAM modification is distributed. Desktop video cannot
verify display handoff behavior.

All32 host suites pass, including updater, Media animation and intermediate
GPU/software rendering checks. Dedicated HTTPS tests use actual TLS with a
trusted local CA and verify successful Content-Length/chunked downloads,
relative/absolute redirects, wrong-host/untrusted-chain rejection, checksum and
length failures, truncation, cancellation/partial cleanup, older/current versions,
invalid metadata, redirect loops and HTTP downgrade rejection. A live HTTPS
request to GitHub verifies the normal proxy/CA route. Tests assert the exact
main/dist manifest and ZIP paths; TLS rejection sends no HTTP request.
Application/TLS sanitizer checks pass; prebuilt
FFmpeg is not instrumented. Native integrity and scalar TLS checks pass, and
the corresponding source rebuild produces an identical application XEX.

The fresh240second720p30 desktop walkthrough shows blade/submenu navigation,
real Local/Jellyfin playback, episode metadata and the Media list animations.
Profile/Guide/storage/FTP/updater states are simulated and labelled. The actual
HTTPS worker is validated by separate integration tests, not that simulation.

Existing settings and the0.48 seek,0.47 audio/timing/episode-label and0.49 FTP/
artwork changes are retained. No settings migration. Original licences and
complete GPLv3 corresponding source, pinned SDK and Mbed TLS/public-CA notices
are included. New console transport, handoff and visual behavior need retesting.
