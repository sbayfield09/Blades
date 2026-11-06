# Validation in the cloud workspace

- Debian 13 amd64, GCC 14.2, SDL 3.2.10, CMake 3.31.6.
- CMake Debug build: passed with -Wall -Wextra -Werror.
- CTest: 2/2 passed. Dashboard exercises A/B edges, repeat timing, reconnection, every blade/menu, unsupported dialogs, confirmed exit, transition completion, render buffer bounds and sound sample range. Preview smoke runs five real SDL frames with dummy video/audio.
- Core/menu/renderer tests compiled with AddressSanitizer and UndefinedBehaviorSanitizer: passed outside the traced sandbox (LeakSanitizer cannot run under the traced command sandbox).
- All four 1280x720 snapshot exports: passed; images inspected. Negative snapshot blade index: rejected with nonzero exit.
- Rootless cloud setup script rerun: passed, including rebuild and CTest.
- Xbox cross-compilation/linking: passed using patched powerpc-xsdk-elf GCC 15.3.0, binutils 2.47 and newlib 4.6. The completed SDK compiler ABI/alignment checks passed. The SDK library, minimal hello.xex and dashboard default.xex all built successfully.
- Native ELF: statically linked ELF32 big-endian PowerPC with xsdk calling-convention flag (0x100).
- Dashboard XEX: 17 image pages; valid Xbox PE subsystem and entry point; all header, image-page and import digest chains verified; xam.xex (64 import slots), xboxkrnl.exe (112 import slots).
- XEX verifier tests accept the SDK-packed fixture and actual binaries, and reject truncated/corrupted copies.
- The initial compiler archive download was denied (HTTP 403); user-supplied archives unblocked it after every pinned SHA-256 checksum matched. No verification was bypassed. Completed bootstrap and packaging commands also pass on repeat runs using retained outputs.
- No Xenia or console execution performed. No physical controller, display, sound, Guide interaction, dashboard return, 17559 import resolution or Peer Pressure compatibility validated.

The Linux preview binary dynamically depends on SDL3 and its system dependencies; the Linux archive is not a self-contained portable application. In this workspace source tools/cloud-env.sh; on another system install SDL3 first.

Visual revision 0.2.0: all neighbouring blade transitions in both directions and submenu open/close animations pass buffer-guard checks at intermediate phases. ASan/UBSan passes for the same paths. The profile shortcut cannot open a hidden dialog under exit confirmation. The uploaded reference recording decodes successfully in full; it shows version 5766 and is used as a qualified motion reference. The newly rebuilt XEX has not been console-tested.

Version 0.3.0 targets the 5766-era five-blade UI. All five root menus and submenus, all eight adjacent transitions, Marketplace edge navigation, expanded submenu animations, X/profile handling and confirmed exit pass desktop checks. The revised XEX builds with 17 image pages and valid digest/import chains. Versions 0.1.0 and 0.2.0 are user-confirmed to run on the console; 0.3.0 requires a new hardware test.

Version 0.4.0: 3/3 CTest suites pass, including XUIZ parsing and PNG decode, malformed bounds/truncation, invalid PNG fallback, safe reload and clipping. Optional tests decode the actual 116-entry 5766 pack and mapped stock PNGs. Address/undefined-behaviour sanitizer tests cover both synthetic malformed packs and the actual pack; the dashboard renderer also passes. Original firmware/resources remain outside the project and are not staged or embedded in the public executable. Native XEX builds and validates; local stock-resource loading has not been tested on the console.

Version 0.5.0: 4/4 CTest suites pass. Synthetic skin fixtures reject invalid
dimensions, off-canvas rectangles, reversed scan-line bounds and corrupt PNG
headers, and verify cache invalidation after reload/close. ASan/UBSan passes
those cases and the full dashboard transition/submenu suite using the actual
195-entry local skin/shared pack. All five 720p root snapshots were inspected;
menu icons, blade labels and tray art render in their respective states.
The mapped shared PNGs also decode through the resource test. The native XEX
has 18 verified image pages, valid header/image/import digest chains and the
same xam.xex (64 slots) and xboxkrnl.exe (112 slots) imports. XEX mutation tests
pass. Earlier versions run on the user's console; this skin build has not yet
been hardware-tested. No uploaded font or upstream art/audio is staged or
embedded in the public executable.

Version 0.6.0: 4/4 CTest suites pass, including non-square icon fit/clipping. Actual-pack resources and all menu/transition buffer guards pass, as do ASan/UBSan resource, skin and dashboard suites. All 48 current root/submenu states were captured and reviewed. Native XEX header/image/import verification passes with 18 image pages; mutation tests pass. See MENU-ALIGNMENT.md for references, changes and remaining visual differences. This build is not yet console-tested.

The final 0.6.0 pack uses BSK2 alpha-derived root and expanded shell bounds. New skin tests cover the active tab and curved side regions across five blades, top/middle/bottom and half expansion, malformed expanded bounds and BSK1 compatibility. Both the new pack and previous BSK1 pack pass the full renderer suite.

Version 0.7.0: 5/5 CTest suites pass, including independent source-animation colour samples, visible motion, shared phase across blades, invalid time handling and buffer guards. The new 195-entry pack passes resource decode and full menu/transition checks; ASan/UBSan passes animation/dashboard checks. Native XEX validates 17 image pages and the same 64 XAM / 112 kernel import slots; XEX mutation tests pass. A fresh 63-second 720p30 H.264/AAC walkthrough decodes in full. Physical-console animation performance remains unverified.

Version 0.8.0: 6/6 CTest suites pass, adding synthetic local-font metrics/raster, scaling/clipping, corrupt metadata, reload and fallback checks. The actual 198-entry pack passes all menu/transition paths and ASan/UBSan. Source font verification matches 271/271 compressed-page SHA-1 values and all 95 ASCII outline and advance/bearing records. All 48 menu states were captured and reviewed. Native XEX validates 17 image pages with unchanged XAM/kernel import counts; mutation tests pass. A fresh 63-second video is provided. Physical-console rendering remains unverified for this build.

Version 0.10: seven desktop suites passed, including bounded stereo WAV loading and full-clip mixing; all six actual-pack clips matched sample-for-sample at 70% gain. Audio ASan/UBSan checks passed. The native XEX passed structural/digest checks. Hardware playback remains unverified.

Version 0.11: eight desktop suites, actual-pack renderer bounds and ASan/UBSan passed. Original-reference cap/wipe positions were measured against pre/post frames; see TRANSITION-AUDIT.md and its raw data for scored windows and limitations. Native XEX digest/mutation checks passed. A fresh 63-second 720p30 stereo walkthrough is included.

Version 0.12: existing desktop suites and actual-pack renderer bounds pass after selected-menu shadow refinement. All 48 menu states captured; first/middle/last root selections reviewed. See MENU-SHADOW.md for reference observations and remaining limits.

Version 0.13: eight desktop suites and actual-pack renderer bounds pass after adding the lower-left shadow taper. All 48 root/submenu snapshots captured; Games, Media, System, Console Settings and Xbox Live reference profiles compared. See MENU-SHADOW.md.

Version 0.14: nine desktop suites and actual-pack render guards passed. SIMD row math verified for all weights and alignments on desktop and a PowerPC VMX harness. Thirty full backgrounds and 48 menu states differ by at most one RGB level from 0.13. Native XEX validation and mutation checks pass; console FPS is recorded by new five-second performance summaries and remains unmeasured here.

Version 0.15 replaces native software rendering with a retained GPU compositor. Eleven desktop suites, 69 command-list scenarios, 113 desktop GPU image comparisons and memory checks pass. The actual compiled Xenos shader containers pass SDK microcode simulation, including 240 swirl samples. Native XEX checks pass. See GPU-RENDERER.md for numerical tolerances and scope: desktop and interpreter checks do not establish console performance.

Version 0.16: eleven desktop suites and 113 desktop GPU image comparisons pass with the 214-entry pack. The additional panel background includes padded shadow coverage; the prior 213 payloads, including all stereo recordings and the logo itself, remain byte-identical. See BRANDING-AUDIO-REVIEW.md.

Version 0.17 adds the short live-rendered Blades startup with one-shot whoosh and Games-root landing. Twelve desktop suites pass, including 36 startup command frames and 149 GPU comparisons. The settled startup pose is pixel-identical to the normal root in reference playback. See STARTUP-ANIMATION.md for timing, audio and hardware-validation scope.

Version 0.20: 14 host suites pass, including the new filesystem/async library
suite with 180 nested directories, overlapping roots, downloaded package types,
auxiliary-module filtering, saved path updates, failed writes, disconnected
files, cancellation and pagination. Library and disc ASan/UBSan/LeakSanitizer
checks pass. 236 desktop GPU comparisons cover the new library, folder picker,
category/removal pages, empty/busy states and long scrolling titles; worst mean
error 0.478396 and peak channel error 4. Native XEX validates 19 pages, 72 XAM
and 122 kernel import slots; verifier mutation fixtures pass. The unchanged
234-entry payload and ZIP integrity/checksum pass. Fresh labelled 75-second
720p30 H.264/48kHz stereo AAC video decodes fully. Actual storage, content mounts
and title handoff remain to be tested on a 360. See LIBRARY.md and
data/v0.20-library-audit.json.

Version 0.21: 14 host suites pass. Library regressions cover empty boot/manual
scans, non-game-only folders, stop/retry, depth boundaries and legacy defaults,
displayed question-mark cleanup and manual preparation of XEX files with zero
or absent execution Title IDs. ASan/UBSan/LeakSanitizer pass for library/disc
checks. 240 desktop GPU comparisons cover depth and progress/stopping pages;
worst mean 0.478396 and peak error 4. Native XEX and mutation fixtures pass
(19 pages, 72 XAM/122 kernel slots). The compatible 234-entry payload is
unchanged and its ZIP/checksum is verified. A fresh labelled 102-second 720p30
stereo H.264/AAC walkthrough decodes fully. Native storage/handoff and the
reported scan problem still need retesting on the user's console.

Version 0.22: 14 suites pass, including strict filename filtering before XEX
content/metadata reads, case-insensitive plain XEX names in dotted parent
directories, File Manager browsing/direct launch, Y scan-path removal and
Scan Now completion retention until focus moves away. CD/DVD/data regressions
cover stable identification, no autoplay, launch data and stale/eject behavior.
Library and disc ASan/UBSan/LeakSanitizer pass. 248 GPU comparisons pass, worst
mean 0.478396 and peak channel error 4. Native XEX validates 20 pages with
72 XAM/122 kernel imports; verifier mutation fixtures pass. Fresh labelled
97-second 720p30 stereo H.264/AAC video decodes fully. The combined USB ZIP
includes the matching XEX and unchanged 234-entry artwork payload, plus notices;
local scan settings and logs are excluded. Actual CD/DVD playback and new file
manager handoffs still require physical-console verification.

Version 0.23: 15 suites pass, adding a regression against the pinned SDK's real
resolver/stat and launch preflight. It reproduces the missing Flash: ENOENT
condition and verifies native-device resolution, the generated XAM alias,
preserved DVD/music/data launch bytes and failure guards. Existing 248 GPU
comparisons pass; disc ASan/UBSan/LeakSanitizer passes. Native XEX digest/import
and mutation checks, compatible artwork and combined ZIP checks pass. Fresh
97-second 720p30 stereo video is labelled as simulated desktop playback.
Hardware DVD/CD playback remains unverified for this correction.

## Version 0.24 profile integration

16/16 host suites pass, including bounded XAM settings and gamer-picture
readers, asynchronous card refresh/identity replacement, controller-slot
switches, stale sign-out rejection, other-slot preservation, confirmation
cancellation and selectable signed-out card navigation. The profile suite also
passes AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer outside
the tracing sandbox (LSan cannot operate under ptrace).

264 CPU/desktop GPU comparisons pass, worst mean 0.478482, peak channel error
4. New cases cover local/Live/signed-out cards, long names/large totals, unknown
statistics, picture replacement and confirmation overlays. Foreground cache
checks verify profile revisions trigger redraw and idle frames still reuse the
cached list. No swirl/GPU behavior is changed.

Native GCC 15.3 build: XEX 0.0.24.0, 20 image pages, 82 XAM/122 kernel import
slots, verified header/image/import digest chains. The compatible artwork has
234 entries and unchanged payload SHA-256 c6470ec2ab422e1f5f2984bd244a79173269fd40fa157714be98af8dd183c55c.
The USB ZIP contains the verified XEX and complete Assets folder, excluding
local scan settings and logs. The new 117-second 720p30 walkthrough has stereo
48 kHz AAC audio and decodes completely. Profile/Guide/storage events are
explicitly simulated; native Guide UI and real profile reads/sign-out need a
physical-console check. See PROFILE-INTEGRATION.md and the v0.24 audit JSON.

## Version 0.25 sign-in card and gamerscore icon

16/16 host suites pass. Profile count refresh tests distinguish a real zero
from an unavailable result and exercise refresh while signed out. 267 CPU/GPU
comparisons pass (worst mean 0.478421, peak 4), including plural/singular/zero
profile counts, the signed-out tile, signed-in GScore icon and long totals.
Profile AddressSanitizer/UndefinedBehaviorSanitizer/LeakSanitizer checks pass.

The native build uses XEX 0.0.25.0 with 20 verified pages and 86 XAM/122 kernel
import slots. The artwork remains the same 234-entry payload; the original
DefaultPicture and GScore PNGs were already present. The new 117-second 720p30
stereo walkthrough is freshly rendered and labels profile/count/Guide events
as simulated. Console public-content enumeration and Guide/account behavior
still need physical-console validation. See the v0.25 audit JSON.

## Version 0.26 GScore alignment

Measured the supplied video-games.jpg and the original PNG/Convection ink
bounds to correct the circle's size, vertical centre and left overhang. The
32 px icon canvas aligns its centre with the suffix's visible ink; its previous
24 px box placed that centre 4 px too high. The suffix/numeric budget follows
the corrected spacing. The original asset is preserved.

16/16 host suites and 267 CPU/GPU comparisons pass (worst mean 0.478439,
peak 4), including all profile-bearing blades and large card totals. Native
XEX 0.0.26.0 has 20 verified pages and 86 XAM/122 kernel slots. ZIP and XEX
integrity checks pass. The fresh 117-second 720p30 stereo walkthrough shows
the corrected card and retains its simulation label. The profile backend is
unchanged; its real-console count/Guide verification remains outstanding.

## Version 0.27 achievements

18/18 host suites and 284 CPU/desktop GPU comparisons pass (worst mean
0.478439, peak channel error 4). New coverage includes signed-out Guide gating
and continuation, earned-only grid filtering, grid/list paging, recent/score
ordering, secret/locked/unlocked descriptions, scope invalidation, cancelled
workers, empty histories and error completion. The actual Xbox adapter is
compiled into a host fixture against the pinned SDK headers to verify local
requestee mode, dynamic ordinals/parameters, BE buffers and handle cleanup.
Shared achievement and adapter fixtures pass ASan/UBSan/LSan outside tracing.

Native XEX 0.0.27.0 has 20 verified image pages and 86 XAM/122 kernel import
slots. Header/image/import digests and a deliberately mutated image are checked.
The USB ZIP contains the matching XEX and complete Assets folder, excluding
scan paths/logs. Its compatible artwork payload is unchanged at 234 entries.

Reference: the supplied Forza Horizon grid photo and the newly uploaded
22-minute video, including the earned grid around 05:20–05:25 and gray list and
description around 04:30–04:45. The gray panel's spacing, brightness and footer
positions were measured from those frames. The fresh 86-second 720p30 stereo
walkthrough labels all profile/achievement/history/storage data as simulated.
The in-app gray panels reproduce the browser presentation; only sign-in invokes
the actual system Guide. Real profile reads, cached pictures and continuation
still require physical-console verification. See ACHIEVEMENTS.md and the
v0.27 data audit for feature boundaries and artifact/source hashes.


## Version 0.28 native achievement Guide and game separators

18/18 host suites and 280 CPU/desktop GPU comparisons pass (worst mean
0.478439, peak channel error 4). The native-adapter host fixture checks
XamShowAchievementsUI (709) and XamShowAchievementDetailsUI (736), exact
user/title/achievement parameters, success/pending/error codes, busy and stale
identity guards, and absent exports. Shared navigation verifies the actual
achievement ID after paging and preserves the selected game/grid/icon.
Both achievement fixtures pass ASan/UBSan/LSan outside tracing.

The original retail 5766 dashboard's call sites confirm the two-/three-argument
Guide signatures. See ACHIEVEMENTS.md and the v0.28 audit for source hashes,
call addresses and register arguments. These static checks and host fixtures
do not execute the real Guide. The custom gray achievement panels are removed.
The green game strip has translucent dark/light separators between icons,
checked against the uploaded reference near 05:20 and the supplied photo.

Native XEX 0.0.28.0 has 20 verified image pages and 86 XAM/122 kernel import
slots. Header/image/import digest and deliberate-mutation checks pass.
The 234-entry artwork payload is unchanged. USB ZIP CRC and matching XEX/pack
checks pass; local scan paths, profile data and logs are excluded.

The fresh 54-second 720p30 stereo desktop GPU walkthrough decodes fully.
Its label identifies simulated profile/achievement/storage/Guide events, and
it shows Guide-request notices, not an actual system Guide capture.
Real Xbox Guide handoffs to the exact game/achievement, closing back to the
same selected icon, sign-in continuation and account changes need a physical
Xbox 360 check. Console API behavior and cached artwork availability cannot be
proved by desktop fixtures.


## Version 0.29 all-file explorer

18/18 host suites and 283 CPU/desktop GPU comparisons pass (worst mean
0.478439, peak channel error 4). Real temporary-directory fixtures list plain
XEX, compound module names, images, text/config files, archives, extensionless
and hidden files. They verify folder/file distinction, directories-first and
case-insensitive sorting, a folder with .xex in its name, ordinary-file A no-op,
plain-XEX launch preparation, rejection of a non-XEX direct launch, and the
unchanged directories-only scan-folder picker. GPU checks cover executable,
ordinary-file and compound-XEX selections and their disabled/enabled A prompts.

The native XEX is rebuilt and its header/image/import digests checked, including
a deliberately corrupted image fixture. ZIP CRCs, standalone XEX/payload matches,
artwork compatibility and exclusion of scan paths/logs are checked. The fresh
50-second 720p30 stereo desktop walkthrough labels filesystem/profile/launch
requests as simulated; actual storage and XEX handoff need console verification.
See the v0.29 file-explorer audit for hashes and exact artifact properties.
