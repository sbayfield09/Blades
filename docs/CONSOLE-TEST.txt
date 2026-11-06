Blades 0.20.0 - games, apps and homebrew hardware test build

Copy default.xex into your Blades folder and extract the matching premade
Blades-artwork-pack.zip there so Assets/blades.xzp is present. No conversion
is required. The pack contains 234 entries; all 218 previous entries remain
unchanged, with sixteen small disc-reading indicator frames added.

Library setup:
Open Games -> Played Games -> Scan Folders -> Add Folder. Select a storage
device, use A to enter folders, then X to use the current folder. Choose Games,
Apps or Homebrew. All descendant folders are scanned with no configured depth
limit. Repeat for additional locations; up to 32 are saved in the local
Config/library-paths.txt file and rescanned when Blades starts. B goes up in
the picker. Select a saved path to remove it without deleting its files.

Game Library, Apps and Homebrew are under Played Games; Arcade and Demos also
use their Games blade entries. Lists scroll. A manually launches the selected
title. X or Scan Now rescans. Adding/scanning never launches anything. B or
changing blades cancels an in-flight launch. Guide/modal guards still apply.

Supported: extracted .xex games/apps/homebrew, Xbox Live Arcade, Games on
Demand, demos and XNA packages. Keep downloaded packages and accompanying
.data folders together. Saves, DLC, updates and profiles are excluded. XBE,
disc-installed containers, compressed archives and incomplete packages are
outside this release's launch support. Titles fall back to folder/file names
when executable metadata is unavailable. Existing font coverage is ASCII.

Try one extracted game, one downloaded title, an app and homebrew on your 360.
Check saved paths after restarting Blades. Test an unplugged drive, path
removal and rescanning; verify your files remain. Send default.log if browsing,
scanning or launching fails. The log includes library commands, scan counts,
metadata errors, content-mount results and SDK launch failures.

14 host suites and 236 desktop GPU comparisons pass, alongside library/disc
memory checks and native XEX validation. Library storage and real title handoff
still need physical-console testing. This release includes a fresh 75-second
720p30 desktop GPU walkthrough with simulated storage/launch preparation.

Controls: D-pad/left stick navigate, LB/RB change blades, A select, B back.
X rescans in libraries or selects the current folder in the folder picker.
Outside library pages it opens the existing profile placeholder. Back opens the confirmed exit menu.
Y ejects a loaded/reading disc from ordinary menus, including submenus. Release
it before another press. The physical eject button is monitored independently.

Select the disc row on Games, Media or Xbox Live:
  Open Tray -> Opening -> Close Tray -> Closing -> Reading -> Play <title>
A opens an empty closed tray, closes an open tray, or launches a ready game.
Insertion and boot with a game already inside never request automatic launch.
Busy states cannot be selected as games or submenus. Labels follow hardware
state, with no fixed artificial reading duration. An empty close returns to
Open Tray. Original modal/startup input guards still apply.

This release targets Xbox 360 game DVDs with default.xex. Titles come from
nxeart or the executable's XDBF metadata. Missing/invalid metadata uses Play
Xbox 360 Game; long titles are shortened to fit. Other disc formats are not
played by this build. Unsupported/unreadable discs can be ejected and replaced.

Check on your 360:
- Start with no disc, and separately with a game already inside.
- Use A for open/close; verify the reported states and actual game title.
- Wait on Play for several seconds: the game must not start by itself.
- Press A on Play and confirm the game launches.
- Use Y with a loaded game, then while Reading; verify ejection.
- Open/close using the physical button, including during title detection.
- Replace a game and confirm the old title clears immediately.
- Try an empty close and an unreadable/unsupported disc; verify recovery.
- Check Guide, reconnect, normal navigation and confirmed exit.

default.log records API availability, tray state, media type, title ID, state
generation, command acceptance and manual-launch failures. GPU pixel checks
and five-second performance summaries remain enabled. Keep the complete log
if disc control, title detection or launching fails.

Desktop state-machine, metadata, sanitizer and renderer checks passed. The existing
DVD handling was confirmed working by the user; library handoff needs a new
physical-console check. The current
walkthrough is desktop GPU playback with explicitly simulated library storage;
it is not evidence of Xbox disc I/O or console FPS.

The existing opening, one-shot whoosh, animated background, Convection atlas
and menu artwork are retained. Metadata runs on a separate worker/core; tray
monitoring continues while files are read.

Artwork/indicator credit: zivvoz / DashX360, inspired by Microsoft Xbox 360.
Licences, attribution, build information and checksums are included. This
standalone app does not change console configuration, profiles, saved games,
NAND, or the configured default dashboard.

