# Blades0.53 — version-only updater test

Identical application behavior, artwork, settings and installer logic to0.52.
Only release versions/labels and matching publication metadata change.

In0.52 open System > Update Blades > Update and Restart. It should offer0.53.0,
download and verify the ZIP, install via the existing helper and relaunch.
Afterwards, confirm0.53.0 is installed and your FTP/Jellyfin/library settings
remain. Another check should say the version is current. This is the actual
Xbox update/restart test; desktop tests do not establish native XAM behavior.

[USB ZIP](../dist/Blades-v0.53-USB.zip) · [XEX](../dist/Blades/default.xex) ·
[Installer helper](../dist/Blades/update-installer.xex) ·
[Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.53-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.53-source.zip)

Both native XEXs are rebuilt. The matching fresh desktop walkthrough labels
Xbox services/updater/installer/restart as simulations. Existing attribution,
including the dedicated Simple360Player credit, and all licences are retained.
No settings migration, decoder changes, plugins or console-wide changes.
Versions0.50/0.51 still need a complete manual installation to gain the automatic
installer;0.52 users should use Update and Restart to test this release.
