# Blades 0.46

Xbox 360 Blades recreation. Extract the [complete USB ZIP](dist/Blades-v0.46-USB.zip)
onto your USB drive and launch Blades/default.xex. Matching Assets are included.
Keep existing Config files when updating; private paths, sessions and logs are not bundled.

[Separate XEX](dist/Blades/default.xex) · [Premade artwork](dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](dist/Blades-v0.46-walkthrough.mp4) ·
[Complete corresponding source](dist/Blades-v0.46-source.zip)

New media scan folders are shared by Music, Pictures and Local Video. Under
Media > Video Marketplace, add a folder, press X and choose scan depth. There
is no media category screen: supported filename extensions sort each file into
its app automatically, including mixed folders and subfolders. Unsupported
files and executables are ignored in media scans. Existing saved typed paths
retain their settings; no migration runs. Remove/re-add one to use shared scanning.
Game/App/Homebrew scan folders and category selection remain separate.

Music/Pictures playback is still unfinished; this release changes their lists
and scan paths. Local video and Jellyfin seeking, automatic resume, Continue
Watching and Next Up are retained. [Jellyfin setup/limits](docs/JELLYFIN.md).

All 29 host suites and the changed library/UI sanitizer checks pass. Native
integrity/shader/instruction checks and a byte-identical archived-source rebuild
pass. The fresh 200-second desktop walkthrough shows the new folder flow and
filtered lists, with simulated storage/console services labelled. It also uses
actual synthetic Local/Jellyfin playback. Xbox behavior still needs hardware testing.

The combined executable is GPLv3. Original MIT/FFmpeg/Simple360Player/XenonFin/
artwork notices are preserved. Complete source, changes, build scripts and
pinned free SDK are in the source ZIP; no Microsoft XDK is needed.
See [release notes](docs/RELEASE-0.46.md) and [licence notice](docs/VIDEO-LICENSE-NOTICE.txt).
