# Current release: Blades 0.45

- All 29 host suites and eight application ASan/UBSan suites pass. External
  FFmpeg libraries are not sanitizer-instrumented; leak detection disabled.
- Official Jellyfin 10.10.7 with synthetic HEVC/AAC movies/episodes verifies
  Continue Watching, the actual next episode, fresh 4-second resume, paused,
  forward/back/rapid/end-bound seeking, audio/video output and a saved absolute
  stop position. Only this disposable short-clip server lowers resume thresholds.
- Fragmented/chunked streams, invalid/expired sessions, temporary server failures,
  completed and partial rewatch resume positions, seek errors and cancellation
  pass. Root virtual-folder pagination, empty lists and malformed results tested.
- A seek closes old I/O and decoder threads, clears queues, stops the server
  encoder and opens the same session at StartTimeTicks. Full-item clocks and
  paused state are retained. Local byte seeking and 0.43 decoder/GPU preserved.
- Native XEX 139-page/import digests, five-library instruction whitelist and
  actual Xenos shader checks pass. FFmpeg build, Xenon filters and GPU source
  remain byte-identical to the previous release.
- Archived source application rebuild produces an identical XEX SHA256:
  e53b38b0c2731af06a631a9178c62669f1ed49af7aa7344e38e31bf8183b51ec.
- ZIP CRC, source manifest/current source, one XEX, matching artwork and licence
  checks pass. No private settings/logs packaged. GPLv3 source/build scripts and
  pinned free SDK included; original licences and attribution preserved.
- Fresh 160-second 720p30 desktop GPU walkthrough with 48kHz stereo fully decodes.
  It shows actual Continue Watching resume, paused forward seek, playing backward
  seek and Next Up playback. Xbox Guide/profile/storage/FTP simulated and labelled.
- Real Xbox Jellyfin networking, keyboard and streaming still need hardware
  tests. No console timing, GPU compressed decoding or 1080p30 claim is made.

See [release](RELEASE-0.45.md), [setup/limits](JELLYFIN.md),
[audit](data/v0.45-video-audit.json). Previous validation follows.

# Current release: Blades0.44

-29 host suites pass, including XenonFin protocol, Media Center/source/browser
  routing, cancellable HTTP service/player and existing FTP/video suites.
- Application ASan/UBSan passes eight suites, including the real socket/decoder
  fixture. External FFmpeg libraries are not instrumented; leak detection off.
- Official Jellyfin10.10.7 isolated test: synthetic HEVC/AAC -> authenticated
  MPEG2/MP2 progressive MPEG-TS ->90 decoded pictures with audio, pause/end,
  watch-history reports and saved-token reconnect/disconnect. Fresh capture uses
  an8second/240picture source. No user server/media or real credentials used.
- Fragmented/chunked responses, active/queued cancellation,401 expiry,503 saved
  session retry and truncated-media errors tested. Remote32KiB AVIO avoids an
  FFmpeg2.8 refill assertion reproduced by the fixture; local64KiB retained.
- Native build,139 XEX image-page/import digest checks, five decoder-library
  instruction audit and actual compiled Xenos shader checks pass. The0.43
  FFmpeg build, Xenon filter and GPU renderer/shader/colour sources are unchanged.
- Archived application rebuild yields byte-identical XEX SHA256:
  c16cc1721b044f8225f2b017e120a12fcc33679325952274085be3269220e683.
- USB/source CRC, manifest, current-source, matching-artwork and licence checks
  pass. No private settings/logs are bundled. Combined binary GPLv3; complete
  source/build scripts/pinned free SDK and original notices supplied.
- Fresh140second720p30 desktop GPU walkthrough,48kHz stereo, fully decoded;
  includes actual Local and Jellyfin playback. Console Guide/profile/storage/FTP
  simulated and labelled. A later tested503 retry fix only affects connection
  failure handling; renderer/player/artwork match the recorded release.
- Actual Xbox Jellyfin network, keyboard and playback still need hardware
  testing. No native timing,1080p30 or GPU compressed-video decode claim.

See [release](RELEASE-0.44.md), [Jellyfin setup/limits](JELLYFIN.md),
[audit](data/v0.44-video-audit.json). Previous validation follows.

# Current release: Blades0.43

- Actual Simple360Player chroma/qpel shift/add kernels ported from pinned
  commit5d235c1fd24c098ccc1b2463c1b4265bf2e7844c. Original source hashes verified.
- Big-endian PPC check: runtime self-tests pass; all positions/weights, put/avg,
  widths, alignments, destination guards and scalar small-block fallback pass.
  Independent scalar oracle matches pinned FFmpeg in66,560 cases. Test ABI
  adaptation is documented; these are arithmetic checks, not Xbox performance.
- Five decoder libraries:1,041,821 instructions audited. Vector instructions
  only in the two port objects, constrained to explicit shift/add/permute ops.
  Generic PPC/AltiVec/autovectorisation stay disabled; GPLv3 enabled, nonfree off.
- Four frame workers3/4/5/2, demux1, four picture slots, preroll and upstream
  catch-up thresholds implemented. Slow-decoder test now asserts activation.
- All26 host suites pass; application ASan/UBSan passes five video suites.
  External FFmpeg is not instrumented; leak detection disabled. Actual compiled
  Xenos shader checks and native XEX digest/import checks pass.
- Matching source application rebuild yields identical executable SHA256:
  cf614d403e54dd310d5f26c0d2c2f96d646b424acddce2c3d00a1bbe293e7ed5.
- Source/USB CRC/manifest/payload/licence checks pass. Combined executable
  distributed under GPLv3, complete corresponding source/scripts/SDK included;
  original MIT/FFmpeg/artwork notices preserved. No private logs/settings.
- Fresh108second720p30 desktop GPU walkthrough,48kHz stereo, fully decoded.
  Host uses scalar decoder; console services simulated and labelled. GPU source
  unchanged from0.42. No real console speed/parity/1080p30 claim is made.

See [release](RELEASE-0.43.md), [audit](data/v0.43-video-audit.json),
[source comparison](SIMPLE360PLAYER-PIPELINE.md). Previous validation follows.

# Current release: Blades0.42

- Private0.41 log: H264 decoded roughly29–33fps in several intervals while
  presentation sometimes0fps and late-discard counts increased. GPU upload/fence
  succeeded. This identifies a presentation defect, not universally sufficient CPU.
- Two real-engine tests reproduce0.41 PCM-before-picture and late-frame freezes
  with a controlled40ms decode budget. Both fail before and pass after this fix.
  The fixed slow-decode run delivers74pictures with42ms maximum inter-frame gap;
  that is host regression evidence, not console performance.
- An audio-only lead fixture reaches timestamped video/EOF without a bounded
  queue deadlock or showing future video early. Native compilation excludes
  the test delay macro. Decoder/GPU renderer/shader/colour source matches0.41.
- All26 host suites pass; application ASan/UBSan passes five video-related tests.
  External FFmpeg libraries are not instrumented; LeakSanitizer disabled.
  Actual compiled Xenos shader checks and native XEX digest/import checks pass.
- Source application rebuild produces identical XEX SHA256:
  1b0cd447706f31546d8c015ad9261f681856685ee681bdbff35264fa9192c155.
- USB/source CRC/manifest/payload checks pass. Artwork unchanged, no saved paths,
  credentials or uploaded logs. Fresh108-second720p30 desktop walkthrough is
  fully decoded. Console services are explicitly simulated.
- User confirmed0.41 MPEG2. Native0.42 pacing/H264 smoothness requires a console
  retest; no new SIMD kernels, faster CPU decoder or1080p30 result is claimed.

See [release](RELEASE-0.42.md), [audit](data/v0.42-video-audit.json) and
[playback](VIDEO-PLAYBACK.md). Previous validation history follows.

# Current release: Blades0.41

- The user reported0.40 audio without picture and a hard crash. The log has no
  fault address, so the exact crash cause is not proven. Raw logs remain private.
- Remove generic decoder AltiVec/compiler vectorization; restore the earlier
  four-worker2–5 placement. Five FFmpeg archives are instruction-scanned: no
  VMX operations in1,029,070 checked instructions. SDK copy code is separate.
- GPU display remains enabled with three separate linear Y/U/V textures and a
  simpler matrix shader, following Simple360Player's CPU-decode/GPU-display split.
- All23 host suites pass, including explicit RGBA fallback, early B/reopen and
  held-buffer ownership;328 desktop GPU comparisons pass. The actual compiled
  Xenos shader passes72 video samples, three sampler slots, limited/full range,
  BT601/709 and boundaries, plus the earlier UI/swirl/cap shader checks.
- Application ASan/UBSan passes; external FFmpeg libraries are not instrumented,
  LeakSanitizer disabled. Native XEX integrity passes. The source application
  rebuild produces identical XEX SHA256:
  9b9aa6a51c0f7dcbe40fed8c9e06bcda08b2f381f26209559c0e83493b208b26.
- USB/source CRC/manifest/payload checks pass; no saved paths, credentials or logs.
  Artwork is unchanged; fresh108-second720p30 desktop GPU walkthrough is fully
  decoded and screenshots inspected. Console services are labelled simulations.
- New first-frame stage logs isolate future failures better. Console texture
  behaviour, crash outcome and1080p30 performance remain unverified. Display
  remains720p; neither player implements full GPU compressed-video decoding.

See [release](RELEASE-0.41.md), [audit](data/v0.41-video-audit.json),
[upstream pipeline](SIMPLE360PLAYER-PIPELINE.md) and [playback](VIDEO-PLAYBACK.md).
Earlier validation history follows.

# Current release: Blades 0.40

- User-confirmed0.39 baseline:720p MPEG2/WMV work well;720p H264 is slow/jittery.
  The supplied log measures UI presentation, not decoded video. It stays private.
- Xbox FFmpeg now has ARCH_PPC/AltiVec enabled, with three workers on2,4,0.
  No VSX/POWER8/ldbrx; the G5 dcbzl probe/clear path is guarded out. A fresh
  decoder build, native XEX build and all header/image/import checks pass.
- All22 host suites pass, with final targeted video/GPU checks after new fixtures.
  Actual1920x1080p30 MPEG2 and1280x720p30 H264 fixtures decode to native YUV
  frames; controls, seeking, return, ownership, cancellation and RGBA fallback
  tests pass. The host rate is not console performance evidence.
-328 GPU comparisons pass, including non-neutral and neutral1080p YUV ramps in
  BT601/709 limited/full range. The new path uploads native YUV and converts/
  scales on the GPU; the display surface remains1280x720.
- Standalone PPC VMX H264 chroma checks pass all16 source alignments,8x8 fractional
  weights, put/average operations and destination guards against scalar math.
  QEMU7400 uses a temporary GPR-save ABI adaptation; production/vector code is
  unchanged. This is neither Xbox execution nor a speed measurement.
- ASan/UBSan pass the application wrapper's RGBA/YUV and HD paths. FFmpeg static
  libraries are not instrumented; LeakSanitizer is disabled in the traced executor.
- The archived application/shaders rebuild to byte-identical default.xex using
  the pinned SDK and exact freshly built optimized decoder. SHA256:
  945459e437acfc648599e840cb322e7742398adf64b050ad486e2f8ec6e9ffa9.
- USB/source CRC, source manifests, exact payloads and safe-entry checks pass.
  No adapter, saved configuration, credentials or logs are shipped. The235-entry
  artwork is unchanged. A fresh108-second desktop GPU walkthrough includes actual
  1080p MPEG2/720p H264 playback and fully decodes without errors.
- New vector/GPU behaviour and1080p30 performance need console verification.
  video-perf telemetry now measures decoded/presented rates and costs independently
  of the UI FPS. No new console performance result is claimed.

See [release audit](data/v0.40-video-audit.json), [release](RELEASE-0.40.md),
[playback](VIDEO-PLAYBACK.md) and [walkthrough](WALKTHROUGH.md).
Previous validation history follows.

# Current release: Blades 0.39

- All 22 current CTest suites pass: 21 host/core/GPU/preview suites and the FTP
  loopback regression, run outside the sandbox to permit its socket bind.
- The actual WMV2/WMA and H264/AAC embedded engine tests pass for frame ownership,
  pause/resume, Guide pause restoration, seeking/end, repeated opens/cancellation,
  stereo/six-channel resampling, silent-video clock and preserved chooser state.
- ASan/UBSan pass for the new application wrapper/frontend. FFmpeg libraries are
  not instrumented; LeakSanitizer is disabled in the traced executor.
- A fresh pinned FFmpeg Xbox build passes; the single native XEX has 138 verified
  64KiB image pages and valid header/image/import chains. Engine symbols are
  present; video adapter/handoff symbols are absent.
- The application and bundled SDK rebuilt from the source archive produce a
  byte-identical default.xex (SHA256
  3cb719e2b19e15a300dffd6ec9a66f139984bf2f4d42fa813a7ef7aafbbe5a5e).
- USB/source ZIP CRC, source manifest, exact XEX/artwork payloads and safe-entry
  checks pass. The USB contains one XEX and no Players, sideload.ini, saved scan
  folders, credentials or logs. The matching 235-entry artwork pack is unchanged.
- Fresh 108-second 720p30 walkthrough: actual host WMV/MP4 playback and return,
  48kHz stereo, fully decoded without errors; screenshots inspected. Storage,
  profiles, FTP and Guide events remain explicitly simulated.
- New embedded Xbox playback, performance and Peer Pressure runtime still need
  console testing. No Xbox/Xenia playback result is claimed. The previous 0.38
  adapter crash is observed only as early startup in the supplied logs; its exact
  faulting instruction was not established. Version 0.39 removes that runtime path.

See [release audit](data/v0.39-video-audit.json), [playback](VIDEO-PLAYBACK.md),
and [walkthrough](WALKTHROUGH.md). Earlier validation history follows.

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


## Version 0.30 plain FTP

19/19 host suites and 290 CPU/desktop GPU comparisons pass (worst mean
0.478439, peak channel error 4). A real loopback ftplib fixture exercises the
production C server against temporary exported HDD/USB directories: bad/good
login, virtual root, PASV/EPSV and directory formats, binary upload/download,
resume/append, metadata, Unicode names, rename/delete, path boundaries, unique
IP device counts, transfer cancellation, off/on persistence across new server
instances, immediate-toggle/exit persistence and settings-save errors. It also
checks status-page navigation/toggle and display-cache invalidation. The full
integration fixture passes ASan/UBSan/LSan outside tracing. Loopback socket
creation requires running outside the network sandbox.

Native XEX, header/image/import digests, deliberate image corruption, ZIP CRC,
matching XEX/payload and omission of local settings/logs are checked. The same
234-entry artwork payload remains compatible. The fresh 52-second 720p30 stereo
walkthrough labels its FTP status/devices/profile/storage values as simulated.
It demonstrates the UI, not console FTP I/O. Native XAM socket behavior, HDD/USB
transfers, link reconnect, actual IP/device display, saved-state restart and
animation performance under transfer load require a physical Xbox 360 check.
See FTP.md and the v0.30 FTP audit for boundaries and hashes.

File Manager X refresh is retained. An additional real-directory fixture adds
and removes a file between refresh requests and verifies the updated listing;
the existing navigation fixture checks X issues LIB_CMD_FILES for that folder.


## Version 0.31 profile, achievement, FTP and audio fixes

21/21 suites and 290 CPU/desktop GPU comparisons pass. The recorded final audit
contains exact pixel-error bounds and native image/import counts. New actual
Xbox-profile adapter fixtures validate current-user tile/stat parameters,
transient-picture retention, native Guide sign-out/profile targets and stale/UI
busy guards. A real filesystem fixture counts public profile-package headers,
deduplicates devices and excludes non-profile/common entries. Achievement
fixtures now use real 0x10000/0x20000 flags and reject the former high-bit mask.

FTP integration changes credentials, reconnects after restart, rejects invalid
login text and upgrades the v0.30 boolean setting. Shared navigation covers
System/Computers, both keyboard requests and off/on controls. Native keyboard
input/cancellation remains a hardware check. Audio tests verify a bounded,
monotonic device fade and silent shutdown tail while original clip PCM tests
remain unchanged. Native/profile-count/achievement/FTP/audio fixtures pass
ASan/UBSan/LSan. These checks cannot demonstrate removal of console-only static.

Visual references include the user's Blur error photo, the original achievement
video near 05:20–05:25, clear Sign In footage near 22:00, and video-games.jpg.
The selected card's reference brightness peaks centrally; its sampled RGB is
recorded in PROFILE-INTEGRATION.md and the audit. The original achievement icon spacing is retained; only the focus box is
inset below its divider. Xbox Live text has a 28-pixel gutter after the buttons.
New generated previews are labeled simulations. The fresh 78-second walkthrough
decodes fully. XEX digest/mutation, ZIP CRC, payload compatibility and exclusion
of personal settings/logs are verified. Native Guide/profile/keyboard behavior,
profile package availability, actual storage transfers and launch/exit audio
still require a physical Xbox 360 check.

## Version 0.32 FTP layout, highlights and inset cap shadows

The FTP status column now reads status, IP, username, password, connected
devices. Login values wrap at 400 pixels using 20-pixel text; following fields
move down for long values. Synthetic maximum-length wide-character inputs
remain above the footer. No protocol or persistent-settings changes.

References inspected: original uploaded dashboard video 04:23, 04:28–04:29,
04:37 and 05:09, plus the uploaded achievement video 22:05 and the supplied
System still. The menu sheen has broader shoulders and a slightly warm tint.
Card focus uses the same shape and reaches its rounded inner edge; the darker
name band is drawn above it. All row dividers remain. Achievement geometry is
unchanged from the corrected 0.31 build.

Cap shadow contours are cached from existing top/bottom artwork alpha masks.
The header shadow is inside its lower edge, fading upward; footer shading is
inside its upper edge, fading downward. Both follow cap animation and curved
ends. Only small edge arrays persist; the decode scratch is released, no
per-frame image allocation is added, and foreground commands remain cached.

Five relevant suites pass: resources, skin, draw-list, startup, GPU. The skin
fixture checks shading on the correct side of each cap and cache invalidation;
292 CPU/desktop GPU comparisons include maximum-length FTP credentials with
both edit buttons selected. Final native XEX digests/mutation, ZIP integrity,
unchanged artwork payload and a fresh fully decoded 78-second labeled desktop
walkthrough are checked in the release audit. Physical screen appearance on
an Xbox 360 remains the final visual check.

## Version 0.33 row edge and rounded card-shadow corrections

Five relevant suites pass: resources, skin, draw-list, startup and GPU,
including 292 CPU/desktop GPU comparisons. New actual-pack pixel checks cover
20 selected row boundaries across all five blade roots, available ordinary
submenus and all three FTP controls. They verify positive focus fill through
offset 50 and a continuous shadow from 51 replacing the selected divider.
The tests reject a bright divider stripe cutting through that shadow. A
full-width check includes the rounded bevel and both shadow styles, ensuring
neither starts above the selected edge.
These would fail the former fill/shadow boundary. Both lower card corners
are checked for shadow following the curve and ending before the centre tail;
the focus reaches the inner border while the frame remains visible.

The draw-list fixture still verifies 74 static/transition/profile command
states and cached/direct pixel equivalence. ASan/UBSan/LSan checks the real-pack
renderer fixture. Native XEX digests/mutation, ZIP CRC, unchanged artwork and
the fully decoded fresh 78-second video are recorded in the release audit.
Source comparisons use original reference footage around 04:28–04:29, 04:37,
05:09 and Sign In at 04:23 / 22:05, plus the signed-in Games reference still.
The final console display appearance remains a physical check.

## Version 0.34 mirrored upper group and first-button divider

Five relevant suites pass: resources, skin, draw-list, startup and GPU.
Pixel checks compare the complete upper shadow with a reflected downward
shadow for both existing rounded and flat styles. First-row calls produce no
shadow; samples at the upper group's first/selected boundaries stay untouched
outside its bounds. The rounded edge curves upward away from the selection.
All five blade roots and FTP are checked for the restored top divider and its
clear gap before the first row. Existing 0.33 row/card pixel checks remain.

The GPU suite adds six positions from first to third row while the foreground
cache follows the animated highlight. Full CPU/GPU comparisons, sanitizer
results, native XEX digests/mutation, archive exclusions/checksums and fresh
walkthrough decode are recorded in the 0.34 release audit. The 234-entry artwork
payload and lower-shadow function are unchanged. Reference frames checked:
Games 04:28–04:29 vs 04:37, Media 05:09, supplied System/Xbox Live stills.
Desktop tests establish shared-renderer parity; physical screen appearance
still requires comparison on an Xbox 360.

## Version 0.35 top-edge alignment and FTP spacing/motion

Five relevant suites pass: resources, skin, draw-list, startup and GPU.
The row-edge checks include the newly filled upper two pixels, while the
existing lower shadow/card contracts and 256000 mirrored pixels remain.
First-row tests now require the fill to meet the top divider rather than
leaving an unpainted gap. Static row positions round to their exact targets.

The actual draw-list fixture renders four identical FTP value strings and
checks matching glyph/font geometry at equal86px intervals. A real dashboard
input sequence verifies that Down changes the target without jumping the
painted cursor, subsequent frame updates pass through an intermediate position
and the settled cursor moves exactly one53px row. Cache rebuilds follow that
motion; selecting the button still requests the correct edit immediately.
The GPU suite adds six FTP cursor positions and retains maximum-length login
coverage. Native digest/mutation, renderer sanitizers, archive checks and a
fresh fully decoded78-second walkthrough are recorded in the release audit.
Physical console display and perceived animation smoothness remain a 360 check.

## Version 0.36 Xbox Live side edges and bottom-divider termination

Five relevant suites pass: resources, skin, draw-list, startup and GPU.
New actual-pack pixel checks exercise all three Live selections. Messages/
Friends show a vertical lower rail beyond the former 15px horizontal shadow;
Friends/Chat show the upper rail. The complete bottom divider and adjacent
clear space remain equal to the no-focus baseline, including the right end.
Chat has no downward curve; the raised logo face remains unchanged.
The saved0.35 root renderer fails the new side-edge regression check.

The shared upper/lower shadow function, previous 20 row-edge checks,256000
mirror pixels and FTP spacing/motion/cache tests remain.304 CPU/GPU comparisons
cover the final renderer. Native digests/mutation, sanitizer output, ZIP checks
and the fresh 78-second labeled desktop walkthrough are captured in the audit.
Actual console screen appearance remains a hardware comparison.

## Version 0.36 reference-colour, GScore and logo-shadow refinements

The final check set also includes ambient: six relevant suites now pass.
Independent double-precision source-equation calculations reproduced the old
palette's golden pixels within the existing tolerance before producing the
new 30 field/four bright-ribbon samples. The shader uses the same refined
palette as the CPU renderer, while the motion equations remain unchanged.

Actual draw-list mask/image alpha bounds require the GScore icon and main
Gamerscore word to end on the same visible row. The logo-shadow fixture compares
against the original logo on a flat background: pixels may darken, never
lighten, and visible shadow must extend outside the original silhouette.
The new mask is precomputed; all non-tint/non-shadow pack entries must match
0.35 byte-for-byte and every retinted mask keeps its original alpha/dimensions.
See COLOUR-AND-BRANDING.md and the final release audit for fitting scope/hashes.

## Version 0.37 tint feedback and Chat cache regression

A renderer/cache rounding mismatch was reproduced with two cursor positions
that both truncate to 105px but round to 105/106px. Cache equality must rebuild
at that boundary, and cached/direct full frames must match over 60 actual Down
and 60 Up updates onto Chat, then reuse idle foreground. Six additional GPU
states cross the half-pixel boundary in both directions. The restored GScore
box is verified through actual image/glyph ink bounds (the prior six-pixel
placement difference). Independent animation expectations use the adjusted
palette; prior 0.36 expectations are reproduced first. Mask alpha/dimensions
and all unmodified pack entries must remain identical to 0.36.

Version0.38: all23 host suites pass across the core run plus separately configured
GPU and loopback FTP runs. The first full run exposed a stale audio fixture
expecting the old Music placeholder; it now tests generic selection/dialog audio
through System. GPU needs the resource-pack environment variable; FTP needs
socket access outside the restricted shell. New request/backend tests pass
ASan/UBSan with leak checking disabled for the traced executor. They exercise
exact1016-byte launch/return data, physical storage paths, root/spaces names,
malformed/truncated/overlong/traversal/config-injection refusal, incomplete/wrong
player packages, atomic replacement on second selection and correct Media chrome/
row/scroll restoration. Existing library cancellation/no-autoplay guards pass.
The two guarded application-method CRCs match the independently decoded pinned
upstream player; mutations/truncation/base changes refuse all writes, and only
its two16-byte entries change/restore. Both new native XEX and title DLL validate
all page/import/header digests. The packaged adapter source independently rebuilds
to a byte-identical XEX. The upstream player/fonts are unchanged. Archive/checksum/
entry-whitelist checks exclude personal scan paths, FTP settings and player logs/
requests. A fresh108-second labeled desktop GPU walkthrough fully decodes.
The user confirmed standalone Simple360Player0.2 works on Peer Pressure; its new
adapter/alias/autoplay/return lifecycle has not been executed on the Xbox here.
