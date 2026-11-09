# Blades0.52 — Update and Restart

Selecting Update and Restart now downloads, verifies and stages the complete
package, launches a local installer helper, installs it and relaunches Blades.
The running executable is unloaded before replacement. Config, personal data,
logs and existing downloads are preserved. Verified backups and a persistent
journal allow rollback after failed or interrupted copying; default.xex remains
present and is replaced last. There is no manual-extraction prompt after this
feature is installed. Network/disk work finishes before restart.

Install this complete USB ZIP manually once:0.50/0.51 only download and cannot
retroactively install the new updater. Keep update-installer.xex beside default.xex.
Future releases use Update and Restart. No DashLaunch/plugin or system changes.

[USB ZIP](../dist/Blades-v0.52-USB.zip) · [XEX](../dist/Blades/default.xex) ·
[Installer helper](../dist/Blades/update-installer.xex) ·
[Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.52-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.52-source.zip)

ZIP extraction is bounded and validates paths/CRC; TLS/SHA256 remain verified.
Existing valid downloads are reused; damaged caches are safely replaced.
Host tests cover real HTTPS download-to-install, failures at every replacement,
process interruptions during installation/recovery, damaged-backup retries,
cancellation, unsafe paths/duplicates/symlinks and malformed DEFLATE/ZIP data.
The source bundle builds both native executables and retains all dependency
sources/licences. Prior playback, artwork, layout and settings are unchanged.

The new240second720p30 walkthrough uses the desktop renderer and matching artwork.
Installer/restart and Xbox services are simulated and labelled; actual transaction
and TLS code have separate tests. XAM helper/relaunch, console storage/cache
power-loss behavior and launch/exit static still require an Xbox test. Journaling
does not guarantee recovery from a future build that cannot boot.
