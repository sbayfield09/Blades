Blades 0.19.0 - disc drive hardware test build

Copy default.xex into your Blades folder and extract the matching premade
Blades-artwork-pack.zip there so Assets/blades.xzp is present. No conversion
is required. The pack contains 234 entries; all 218 previous entries remain
unchanged, with sixteen small disc-reading indicator frames added.

Controls: D-pad/left stick navigate, LB/RB change blades, A select, B back.
X opens the existing profile placeholder. Back opens the confirmed exit menu.
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

Desktop state-machine, metadata, sanitizer and renderer checks passed. The new
DVD hardware path still requires physical-console validation. The 52-second
walkthrough is desktop GPU playback with explicitly simulated disc hardware;
it is not evidence of Xbox disc I/O or console FPS.

The existing opening, one-shot whoosh, animated background, Convection atlas
and menu artwork are retained. Metadata runs on a separate worker/core; tray
monitoring continues while files are read.

Artwork/indicator credit: zivvoz / DashX360, inspired by Microsoft Xbox 360.
Licences, attribution, build information and checksums are included. This
standalone app does not change console configuration, profiles, saved games,
NAND, or the configured default dashboard.

Version 0.19 corrects media detection from the supplied console log:
- FFFFFFFE is pending identification; Reading continues while the drive works.
- XAM can return a title ID; nonzero returns no longer discard a game result.
- Cached CLOSED/EMPTY changes during authentication retain the active read.
- Repeated settled no-media results return to Open Tray after an empty close.
- Unsupported results keep being polled so a later valid result can recover.

Please retry closing both an empty tray and a tray containing a game. Leave a
game inserted until its title appears, then press A to launch it. Autoplay
remains off. New logs distinguish the raw media word from the function return.
