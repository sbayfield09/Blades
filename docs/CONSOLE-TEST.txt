Blades 0.26.0 - profile integration hardware test build

Download Blades-v0.26-USB.zip and extract it onto your USB drive. It contains
Blades/default.xex and the complete Blades/Assets folder, with notices and
installation notes. Launch Blades/default.xex. No separate artwork extraction
or conversion is required. The ZIP excludes machine-specific scan settings and
logs, so extracting over an existing Blades folder preserves its scan paths.
Separate XEX/artwork downloads remain available.

Version 0.26 integrates the console's active profile and Guide sign-in menu.
When signed out, the account card shows Sign In, the original green default
picture and the available profile count, matching the supplied reference.
There is no previous gamertag or personal statistics. The count comes from
public content metadata; if unavailable, the card says Choose a Profile. Navigate up from the first menu item to highlight it,
then press A to open the real Xbox Guide sign-in menu. X at a blade root also
opens sign-in. A profile already signed in is picked up automatically.
The Gamerscore label uses the original circular G icon, sized and centered
against the reference text; its value has no
trailing text G. The card displays the active controller user's gamertag, gamer picture, games
played, gamerscore and achievement count. Local/offline profiles are supported.
Card fields refresh after returning from the Guide and periodically while
Blades stays open. Unknown totals show --, not an invented zero. No account
passwords, PINs or profile files are read or stored by Blades.
X at a blade root while signed in opens a sign-out confirmation (Cancel is
selected first). Confirming signs out only that controller's account and
preserves the other signed-in users. Library X shortcuts are preserved.
No profile is signed in or out automatically at startup.

Please check: start signed out; press A on the account card; choose a profile
in the Guide; compare name, picture, gamerscore and achievements with the
system dashboard; cancel and then confirm sign-out. Also try a local profile,
changing accounts in the Guide and a second controller. Send default.log if
any card field remains -- or Guide sign-in/sign-out fails. The profile log
records slot/state/field availability and operation errors, not gamertags,
XUIDs or account credentials. Real console Guide/profile API behavior still
needs hardware verification; desktop preview cannot authenticate Xbox users.

The previous /Device/Flash/dash.xex media-player handoff fix is retained.

Library setup:
Open Games -> Played Games -> Scan Folders -> Add Folder. Select a storage
device, use A to enter folders, then X to use the current folder. Choose Games,
Apps or Homebrew, then select Unlimited (first option) or a depth from 0 to 32. Depth 0
checks the selected folder only; depth 1 includes its children. Default: 3.
Existing v0.20 paths are retained with depth 3 until you change them. Repeat for additional locations; up to 32 are saved in the local
Config/library-paths.txt file and rescanned when Blades starts. B goes up in
the picker. Select a saved path to change its depth, or press Y on the path
to remove that scan location. Storage folders and their files are not deleted.

Game Library, Apps and Homebrew are under Played Games; Arcade and Demos also
use their Games blade entries. Lists scroll. A manually launches the selected
title. X or Scan Now rescans; X changes to Stop Scan while scanning.
The current folder/file, counts and depth are shown in the right panel.
Completed results stay there until Scan Now is unhighlighted. No choose-folder,
scanning or titles-found status text is drawn at the bottom left. Empty scans explicitly
finish and re-enable controls even when no runnable titles are found. Stopping
waits for the current storage read to return. Adding/scanning never launches anything. B or
changing blades cancels an in-flight launch. Guide/modal guards still apply.

File Manager is directly below Homebrew under Played Games. It lists folders
and plain .xex files. A opens a folder or manually launches an executable,
including zero/missing-Title-ID homebrew, without a configured scan folder.
B goes to the parent folder; X refreshes. No storage files are deleted.

Only plain names such as Utility.xex or default.XEX are eligible. Compound
file.dll.xex / file.exe.xex / file.any.xex names are skipped before reading
file contents. Supported downloaded package formats remain supported.

Supported: extracted .xex games/apps/homebrew, Xbox Live Arcade, Games on
Demand, demos and XNA packages. Keep downloaded packages and accompanying
.data folders together. Saves, DLC, updates and profiles are excluded. XBE,
disc-installed containers, compressed archives and incomplete packages are
outside this release's launch support. Titles fall back to folder/file names
when executable metadata is unavailable. Displayed titles omit question marks and unsupported glyphs; file/launch paths
are unchanged. XEX apps with zero or missing Title IDs launch by path, using a
folder/filename label when title metadata is unavailable.

Try one extracted game, one downloaded title, an app and homebrew on your 360.
Check saved paths after restarting Blades. Test an unplugged drive, path
removal and rescanning; verify your files remain. Send default.log if browsing,
scanning or launching fails. The log includes library commands, scan counts,
metadata errors, content-mount results and SDK launch failures.

16 host suites and 267 desktop GPU comparisons pass, alongside profile
memory checks and native XEX validation. Library storage and real title handoff
still need physical-console testing. This release includes a fresh 117-second
720p30 desktop GPU walkthrough with simulated profiles, Guide requests, storage and launch preparation.

Controls: D-pad/left stick navigate, LB/RB change blades, A select, B back.
X rescans in libraries or selects the current folder in the folder picker.
At a blade root X opens Guide sign-in, or confirms sign-out when signed in. Back opens the confirmed exit menu.
Y removes a highlighted saved scan path on Scan Folders. Elsewhere it ejects
a loaded/reading disc from ordinary menus, including submenus. Release
it before another press. The physical eject button is monitored independently.

Select the disc row on Games, Media or Xbox Live:
  Open Tray -> Opening -> Close Tray -> Closing -> Reading -> Play <title>
A opens an empty closed tray, closes an open tray, or launches a ready game.
Insertion and boot with a game already inside never request automatic launch.
Busy states cannot be selected as games or submenus. Labels follow hardware
state, with no fixed artificial reading duration. An empty close returns to
Open Tray. Original modal/startup input guards still apply.

Xbox 360 games launch their disc XEX. Audio CDs and DVD videos are recognized
separately after stable media detection, with Play CD / Play DVD labels. A
requests the Xbox system music/DVD player via the retail dashboard. Data CDs
and DVDs display Open Data Disc; A opens the Xbox system dashboard to browse
supported media. No media type autoplays. Y and the physical eject button
continue to work. The system player handles its usual format/region/parental
restrictions. These new media handoffs need hardware testing.

Game titles come from nxeart or executable XDBF metadata, with Play Xbox 360
Game as fallback. DVD-Audio, Video CD, HD-DVD and original Xbox disc launching
are outside this release's media-player support.

Check on your 360:
- Start with no disc, and separately with a game already inside.
- Use A for open/close; verify the reported states and actual game title.
- Wait on Play for several seconds: the game must not start by itself.
- Press A on Play and confirm the game launches.
- Use Y with a loaded game, then while Reading; verify ejection.
- Open/close using the physical button, including during title detection.
- Replace a game and confirm the old title clears immediately.
- Try an empty close and an unreadable/unsupported disc; verify recovery.
- Insert an audio CD, a DVD movie and a data disc. Verify their labels and
  manual A handoffs, Y/physical eject and no autoplay.
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
standalone app only changes the active sign-in session when requested; it does not edit
console configuration, account credentials, profile files, saved games,
NAND, or the configured default dashboard.

