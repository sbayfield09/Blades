# Games, apps and homebrew — Blades 0.21

Configure locations under **Games → Played Games → Scan Folders**.

1. Select **Add Folder**, choose the HDD, USB drive or memory unit, and press A
   to enter folders. Press B to go to the parent or back to the storage list.
2. Press **X: Use Folder** at the location you want to scan.
3. Choose **Games**, **Apps** or **Homebrew**, then select a scan depth.
   Depth 0 checks only the chosen folder; 1 includes its children, and so on.
   Choose 0–32 or Unlimited. The default is 3. Confirm with A to save and scan.
4. Repeat for your other locations. X rescans from the library or Scan Folders;
   **Scan Now** does the same. Selecting an existing path offers removal of
   that scan location or a change to its depth. Removal leaves the games and their files intact.

The Games category includes extracted Xbox 360 executables, Xbox Live Arcade,
Games on Demand and demos. Arcade and demos also appear under their original
Games blade entries. Apps and Homebrew have separate lists under Played Games.
Choose the corresponding category when adding those folders. Lists scroll and
sort by title; A launches the selected entry. Scanning and insertion never
request automatic launch. Y retains its existing disc-eject action.

Locations are stored alongside Blades in `Config/library-paths.txt`, with up to
32 locations. They load and scan when Blades starts. Version-1 settings retain their paths
and categories and use the new default depth of 3; the next saved change writes
version-2 settings with an explicit depth for each path. Save uses a temporary file,
flush and rename; a failed save leaves the previous configuration in place.
No system dashboard, NAND, profile, saved game or console-wide configuration
is edited. Filesystem/SDK path length and available memory still limit readable paths,
even with Unlimited selected. Unavailable
folders and partial scans are reported in the status line and console log.

## Executables and downloaded packages

- Loose `.xex` executables: validate the XEX2 header; Title ID 0 and an absent execution-info header
  are accepted. Read XDBF title metadata when available;
  otherwise use the containing folder's name for `default.xex`, or the file's
  name for another executable. Blades excludes itself. A Games folder with
  `default.xex` contributes one entry rather than each auxiliary XEX module.
  Apps/Homebrew scan paths include other XEX apps alongside a default XEX.
- CON/LIVE/PIRS packages: read the content type, title ID and display name.
  Supported types are Xbox 360 Games on Demand (`00007000`), Arcade
  (`000D0000`), demos (`00080000`) and XNA (`000E0000`). Extensionless package
  names and `.con`, `.live`, `.pirs`, `.stfs` are recognized.
- Saved games, DLC, title updates, profiles and media are excluded. SVOD `.data`
  directories and `$SystemUpdate` are not enumerated as games. Keep the complete
  package and any accompanying `.data` directory in their original relationship.
- Original Xbox XBE software, disc-installed `00004000` containers, archives and
  incomplete/download-in-progress files are outside this release's launch path.

Titles are deduplicated by case-insensitive executable/package path when roots
overlap. Separate installations remain separate entries. The first matching
scan location sets the category; prefer distinct Games, Apps and Homebrew paths.
Question marks are removed from displayed title labels. Unsupported non-ASCII
glyphs are omitted because the existing atlas would draw their bytes as question
marks. Filename/path bytes are preserved for launching.

The scanner uses an iterative heap worklist with a depth attached to each
folder. Scans always publish completion, including when zero runnable titles
are found. Empty results explicitly show a completed scan and clear busy/input
wait state, allowing another scan or an edited path.

While scanning, the current folder/file path, folder/file counts, found count
and current depth are shown in the right-hand details panel. There is no
duplicate path/scanning text at the lower left. X requests Stop Scan. This is cooperative: a
currently blocked filesystem/metadata read must return before the worker can
stop. The displayed path identifies that read; no unsafe thread termination is
used. Live progress uses a separate fixed-size mailbox and is copied at most
10 times per second; the full catalogue is not duplicated for each file. Directory browsing, settings I/O, metadata and launch preparation all run
on a worker; the render thread exchanges owned snapshots under a short lock.
The Xbox worker runs on hardware thread 4 below normal priority. Cancelling or
leaving a library while preparation is in flight invalidates the launch request.
Guide/startup/modal guards prevent new requests, and completed handoff waits for
Guide and dialogs to close. The original disc worker and tray policy remain.

Loose titles use `xsdk_launch`. Downloaded titles use the existing XAM content
mount service, verify the mounted `default.xex` and its title ID, then hand that
executable to the SDK launcher. The execute mount survives the dashboard's
`atexit` cleanup; failures close it. Packages are neither converted nor edited.
Storage aliases are private to Blades, and the failed-launch path keeps the
UI running with a readable error and log entry.

Public API cross-checks (reference material, not copied application code):

- FreeStyle, pinned `df89a42edf055370e3383576c93199a3713c41e9`,
  `Freestyle/Tools/Generic/xboxtools.cpp` (`mountCon`, `unmountCon`) and
  `Freestyle/Tools/XEX/Xbox360Container.cpp` (`Mount`, `LaunchGame`). These
  establish the seven-argument `XamContentOpenFile` contract and execute mount
  flags `04000043`, including retention across title termination.
- Public XAM export table: `XamContentOpenFile`, ordinal `0269`.
- xsdk `5f6dc726f65f6d171493a3628c2415c03551f27b`: path, directory, pthread,
  content-close and launch implementations.

## Validation and console checks

Host tests exercise repeated empty boot/manual scans, directories containing
only non-game files, stop/retry, legacy migration, depth boundaries, ID-less
manual launch preparation and real filesystem scans through 180 nested folders,
overlaps, auxiliary modules, package classification, invalid metadata, symlink
loops, settings persistence and failure, removed files, worker launch requests,
cancellation, pagination, button guards and display-cache revisions. GPU checks
include every new page, long labels, scrolling, empty libraries and busy states.
The fresh walkthrough uses the release renderer with explicitly simulated
storage snapshots and launch preparation.

The native XEX is compiled and structurally verified here. Physical storage
browsing, package mounting and handoff into another title require console testing:
add one extracted game folder, an Arcade/GOD Content folder, an Apps folder and
a Homebrew folder. Wait on each entry to confirm no autoplay, then launch using
A. Restart Blades to check persisted locations. Try B while preparing, unplugged
USB storage, rescanning and removing a path; confirm the files remain. If any
step fails, send the complete `default.log`, which now includes library API,
command, scan-summary and launch-result entries.
