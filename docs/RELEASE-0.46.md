# Blades 0.46

[USB ZIP with XEX and Assets](../dist/Blades-v0.46-USB.zip) ·
[Separate XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.46-walkthrough.mp4) ·
[Complete corresponding source](../dist/Blades-v0.46-source.zip)

New media scan folders are shared by Music, Pictures and Local Video. Open
Media > Video Marketplace > Add Folder, choose a folder, press X and choose
scan depth. The Music/Pictures/Video category screen is skipped. Supported
filename extensions determine which app lists each file, including mixed
folders, nested folders and uppercase extensions. Unsupported files and Xbox
executables are ignored in media scans; overlapping paths do not duplicate files.

Existing saved paths retain their categories and depths. No migration runs.
Remove and re-add an old typed media path to use the new shared behavior.
Shared paths use configuration version 4; older configuration formats still
load unchanged. Game/App/Homebrew categories and their scan folders remain separate.

Music/Pictures playback remains unfinished; this release changes their scan
lists and path selection. Local video playback and Jellyfin connection,
seeking, resume, Continue Watching and Next Up are retained.

29 host suites pass. Application ASan/UBSan covers the changed library and UI
paths. Tests cover mixed extensions, unsupported files, depth, duplicate paths,
settings round trips, asynchronous rescanning, no autoplay, the direct folder-to-
depth flow and legacy settings. Native XEX/instruction/shader checks pass.
The fresh desktop walkthrough labels simulated storage/console services and
shows adding a shared folder and its filtered lists, plus actual local/Jellyfin
playback. Real Xbox behavior still needs hardware testing.

The executable remains GPLv3, with complete corresponding source/build scripts,
pinned free SDK and original MIT/FFmpeg/Simple360Player/XenonFin/artwork notices.
Private sessions, scan paths and logs are excluded from release archives.
