# Games, apps and homebrew — Blades 0.20

Configure locations under **Games → Played Games → Scan Folders**.

1. Select **Add Folder**, choose the HDD, USB drive or memory unit, and press A
   to enter folders. Press B to go to the parent or back to the storage list.
2. Press **X: Use Folder** at the location you want to scan.
3. Choose **Games**, **Apps** or **Homebrew**. The folder is saved and scanned.
   Every descendant folder is searched; there is no configured depth limit.
4. Repeat for your other locations. X rescans from the library or Scan Folders;
   **Scan Now** does the same. Selecting an existing path offers removal of
   that scan location. Removal leaves the games and their files intact.

The Games category includes extracted Xbox 360 executables, Xbox Live Arcade,
Games on Demand and demos. Arcade and demos also appear under their original
Games blade entries. Apps and Homebrew have separate lists under Played Games.
Choose the corresponding category when adding those folders. Lists scroll and
sort by title; A launches the selected entry. Scanning and insertion never
request automatic launch. Y retains its existing disc-eject action.

Locations are stored alongside Blades in `Config/library-paths.txt`, with up to
32 locations. They load and scan when Blades starts. Save uses a temporary file,
flush and rename; a failed save leaves the previous configuration in place.
No system dashboard, NAND, profile, saved game or console-wide configuration
is edited. There is no imposed scan-depth setting, though filesystem/SDK path
length and available memory still limit which paths can be read. Unavailable
folders and partial scans are reported in the status line and console log.

## Executables and downloaded packages

- Loose `.xex` executables: read their execution ID and XDBF title when available;
  otherwise use the containing folder's name for `default.xex`, or the file's
  name for another executable. Blades excludes itself. A folder with
  `default.xex` contributes one entry rather than each auxiliary XEX module.
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
Non-ASCII titles retain their UTF-8 metadata, but the existing font atlas still
only covers the original 95 ASCII glyphs.

The scanner uses an iterative heap worklist rather than recursive function
calls. Directory browsing, settings I/O, metadata and launch preparation all run
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

Host tests exercise real temporary filesystem scans through 180 nested folders,
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
