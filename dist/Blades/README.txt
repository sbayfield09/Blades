Blades 0.40.0 - Video playback inside the Blades executable

Extract Blades-v0.40-USB.zip onto USB and launch Blades/default.xex. The ZIP
includes default.xex, complete Assets and licences. No Players folder, adapter,
DLL, DashLaunch or stock player is required. Existing 0.38 Players files are
unused; you may remove that old folder manually. The ZIP excludes saved scan
paths, FTP credentials and logs, preserving those on an update.

Version 0.40 video performance:
- AltiVec decoding enabled; three workers span all physical cores.
- Common YUV420 frames use GPU colour conversion/scaling.1080p input remains
  full resolution until the GPU; the actual display surface is still720p.
- Try the supplied1080p30 MPEG2 sample and the same720p H264 file from0.39.
  Leave a long file playing normally for15seconds, then send default.log.
  New video-perf lines show decoded/presented FPS, preparation time and underruns.
- User-confirmed0.39 baseline:720p MPEG2/WMV good,720p H264 slow/jittery.
  0.40 performance and vector/GPU runtime still need console verification.

Retained video controls:
- Keep choosing files in Media > Video. Add folders under Media > Video
  Marketplace; choose Video, a scan depth and Scan Now.
- A plays the exact selected file inside Blades. A pauses/resumes, Left/Right
  seeks ten seconds, X switches fit/stretch, Y toggles the controls.
- B or Back stops and returns to the same list and selected row. EOF returns
  there too. Opening Guide pauses and restores the previous pause state on close.
- WMV/MP4/MOV/MKV/AVI/ASF/MPEG/TS/FLV containers are accepted; their codec/profile
  must also be supported. Input above 1920x1080 is rejected; output is at most
  1280x720. Console performance for this new engine is not yet measured.
- Start with the synthetic 640x360 sample.wmv and sample.mp4 in tests/media of
  Blades-v0.40-source.zip. Host playback/controls and Xbox build/integrity passed;
  this embedded Xbox playback needs a new hardware test.
- If it fails, send Blades/default.log and the last screen/sound observed.
  No separate player/adapter logs are used. Logs may contain selected filenames.
- Music/Pictures playback and original Xbox music visualisations are unfinished.
- FFmpeg is LGPL2.1+; complete corresponding source and relink instructions are
  in Blades-v0.40-source.zip. See licenses/VIDEO-LICENSE-NOTICE.txt.
- Artwork is the unchanged matching 235-entry pack with retained tints/shadows.

Retained 0.37 corrections:
- Marketplace tint is halfway between 0.35 and 0.36. Xbox Live keeps the
  0.35 hue with lower saturation. Media is about 7% darker than 0.36.
  Games and System retain their 0.36 colours. Cap/tray masks match the tints.
- Restore the pre-0.36 Gamerscore G box position (y148 instead of y142).
  Keep the original icon pixels, size and horizontal alignment.
- Fix a drawing-cache rounding mismatch that could retain a one-pixel lower
  shadow when scrolling down to Chat and IM. The cache and renderer now both
  round the cursor position. Test Down from Friends and Up from Open Tray.
- Retain the full bottom Live divider and bounded rounded group edges,
  stock 360-logo drop shadow, FTP spacing/cursor animation and card shadows.
- The matching premade artwork pack has 235 entries and is included here.

Profile fixes:
- Signed-out profile count reads public profile-package headers on available
  HDD/USB/memory storage, deduplicating copies. No account/GPD payload is read.
- The card uses the active user's actual local/normal gamer picture. Temporary
  read failures keep the last valid picture until the account changes.
- Up from the first menu item selects the card signed in or out. A requests
  native Guide sign-in or the user's native profile/gamer-card menu.
- X at a blade root requests the Guide's sign-out-enabled interface. The old
  custom confirmation/direct background sign-out has been removed.
- Card focus survives sign-in, and identity changes invalidate its draw cache.
- The card/menu highlight is brightest at the centre and fades toward both
  edges, using the clearer uploaded sign-in video reference near 22:00.
  The name band stays above the sheen and fades without a hard inner separator.
  Other button dividers remain; the selected lower groove is covered by its
  shadow, including in the FTP controls.
- The Gamerscore G icon and real totals remain. Xbox Live's information text
  has its own bounded/wrapped column clear of the button separator.

Achievements:
Games -> Achievements asks for Guide sign-in first when needed and resumes on
success. The unlock flags are now decoded as 0x10000/0x20000, fixing the case
where real earned totals were shown but the icon grid said details unavailable.
The existing icon spacing is retained; only the selected game highlight is
inset so it no longer touches the dividing line above. Right enters the earned grid; A opens the exact
achievement in the real Guide. A on the game strip opens that game's Guide
achievements. B returns through grid, game strip and Games; X refreshes.
Changing accounts clears prior-account achievement data. Reads run on a worker;
no achievements or gamerscore are modified.

FTP is now in System -> Computers. It runs across all blades/submenus once the
main menu loads, unless previously turned off. The screen shows on/off, current
username/password (initially blades / blades), status, IP and connected devices.
Change Username/Password opens the Xbox keyboard; confirm 1-63 printable
characters, or cancel unchanged. Login changes disconnect existing clients.
The enabled state and both login fields persist in Config/ftp-settings.txt;
v0.30 files keep their saved on/off choice. Failed reads/saves are reported.
Connect to the displayed IP, port 21, plain unencrypted FTP, passive mode.
The initial / directory lists available HDD, USB0, USB1, USB2 and memory drives.
No SFTP/TLS. Transfers run on workers with bounded buffers; multiple sessions
from the same IP count as one device. Resume offsets are limited to 2 GiB - 1.
X in File Manager refreshes its current folder after uploads/removals.

Audio startup now primes silent frames and ramps device gain. Shutdown fades
to zero, queues a silent tail and drains it before unregistering the driver.
The original navigation/startup PCM and sound event timings are unchanged.
This targets app-boundary clicks/static; listen on the console to confirm the
reported launch/exit noise is gone. Host envelopes cannot reproduce that noise.

Please verify on your 360: actual profile count/picture, signed-in card A,
Guide X sign-out and cancellation, real earned achievement icons/Guide details,
selected tab/divider alignment and centre-fading card highlight. Check the Xbox
Live text gutter with Messages/Friends/Chat highlighted. In System -> Computers,
change each FTP login field, cancel an edit, reconnect, restart while off/on,
and verify both credentials persist. Try an HDD/USB transfer followed by X in
File Manager. Listen at app launch and exit and check transfer-time animation
performance. Send default.log if an API/read or save fails; it records result
codes and availability, not profile credentials or FTP login text.

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
and all regular file types, including hidden and extensionless files. Folders
sort first, then files alphabetically. A enters folders or manually launches
plain .xex executables, including zero/missing-Title-ID homebrew. Other files
show their path with an inactive A prompt; pressing A does nothing.
Compound file.dll.xex / file.exe.xex entries are visible but cannot launch.
B goes to the parent folder; X refreshes. Add Folder remains directories only.
No file is deleted, changed or previewed. Test a folder containing text, images,
archives, config files, plain XEXs and compound module names. Confirm only a
valid plain XEX launches and other files cannot be entered as folders.

Only plain names such as Utility.xex or default.XEX are eligible. Compound
file.dll.xex / file.exe.xex / file.any.xex names are excluded from launch/scanning before reading
file contents, but are visible in File Manager. Supported downloaded package formats remain supported.

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

21 host suites and 290 desktop GPU comparisons pass, alongside profile
memory checks and native XEX validation. Library storage and real title handoff
still need physical-console testing. This release includes a fresh 84-second
720p30 desktop GPU walkthrough with simulated profiles, achievements, FTP,
storage and native Guide/keyboard requests. It demonstrates the actual shared
UI but cannot prove console authentication, networking, keyboard or audio-driver
behavior. Native-adapter/count/achievement/FTP/audio sanitizer fixtures pass.
Please verify the real Guide handoffs and return selection on a physical 360.

Controls: D-pad/left stick navigate, LB/RB change blades, A select, B back.
X rescans in libraries or selects the current folder in the folder picker.
At a blade root X opens Guide sign-in, or opens Guide sign-out when signed in. Back opens the confirmed exit menu.
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
