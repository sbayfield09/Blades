# Blades 0.51 — updater test

Version-only test release. No feature, layout, artwork, settings or playback changes
from0.50. Only version strings, XEX version and matching publication metadata change.

In0.50, open System > Update Blades. It should find0.51.0. Select Download Update;
the verified ZIP is saved in Blades/Updates. Exit Blades, extract the ZIP over the
installation and keep your Config files. Relaunch: installed version should be
0.51.0 and Check for Updates should report that it is current.

[USB ZIP](../dist/Blades-v0.51-USB.zip) · [XEX](../dist/Blades/default.xex) ·
[Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.51-walkthrough.mp4) ·
[Complete source](../dist/Blades-v0.51-source.zip)

The fresh desktop walkthrough uses the same renderer/artwork. Guide/profile/
storage/FTP/updater snapshots are simulated and labelled. Actual repository
update detection/download is validated separately. Physical Xbox testing remains
with the user; this release adds no automatic installer.
