# Blades Reimagined

An Xbox 360 Blades dashboard recreation using the open-source xsdk, with a
shared SDL3 desktop preview. The application remains named Blades on the console.
Application source is browsable in src/, include/ and tests/ on main. Download
builds, USB ZIPs, premade artwork, walkthroughs and complete source bundles from
[GitHub Releases](https://github.com/sbayfield09/Blades-reimagined/releases).

Version0.50 adds System > Update Blades: check the latest stable Release,
download the complete USB ZIP with progress/cancel and verify its SHA256 before
saving it in Updates. Exit Blades before extracting it; the running XEX and
private Config files are never overwritten by the updater. [Details](docs/UPDATES.md).

Media libraries and Jellyfin subfolders now use the shared animated cursor and
brief content fades when changing folders. Startup initializes black scanout
before loading artwork, and shutdown drains black frames before cleanup. The
remaining hardware handoff and visible static need a physical-console retest;
desktop capture cannot prove a display handoff fix.

Jellyfin artwork/descriptions/runtime, Continue Watching/Next Up, resume and
actual-content seeking are retained, along with the0.49 FTP improvements and
transfer diagnostics. Existing settings/scan paths are unchanged; no migration.
Local decoding retains the working Simple360Player H264 filters and GPU YUV
conversion. Video decode itself runs on the CPU. [Build from source](docs/BUILDING-SOURCE.txt).

The combined binary is GPLv3. Original MIT, dependency, artwork, font and SDK
notices are preserved. Complete corresponding source and a pinned free SDK are
also supplied as a Release asset; no Microsoft XDK is required.

## Implemented

- Shared 1280x720 renderer, five blades with vertical labels, gradients, original button prompts.
- Staged blade sweeps, content fades, expanding submenus and selection highlight animation.
- Continuous ambient background motion adapted from the supplied DashX360 implementation, sharing one clock across menus.
- At least one submenu on every blade; all displayed rows are navigable. Explicit unsupported-service dialogs.
- D-pad/stick and LB/RB navigation; A select, B back; repeat delay and release-to-rearm after controller reconnect.
- Recorded transition/startup whooshes, GPU animated background and local Convection atlas.
- Manual disc control/launch and user-configured Games, Apps and Homebrew libraries.
- Embedded font/theme plus optional local XUIZ/PNG resources; absent or corrupt optional resources retain fallbacks.
- Xbox logging and confirmed exit through the SDK's standard title termination.
- Desktop keyboard/gamepad preview, deterministic screenshots, core tests and SDL smoke test.

## Remaining work

Music/Pictures playback, original music visualisations, recent-game persistence,
unsupported system settings and default-dashboard registration remain unfinished.
Profile/Guide integration, game libraries, FTP and disc functionality retain
their existing implementations; desktop captures simulate console services.

## Build on LMDE / Debian

Install GCC, CMake >=3.25, Ninja, SDL3 headers/libraries, Python >=3.11. SDL3 may require a newer package source or a source build on older LMDE releases.

```sh
python3 tools/build-video-ffmpeg.py --target host --build build/ffmpeg-host
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBLADES_FFMPEG_PREFIX="$PWD/build/ffmpeg-host/install"
cmake --build build/linux -j4
ctest --test-dir build/linux --output-on-failure
./build/linux/blades-preview
```

Keyboard: arrows or W/S for menu movement; left/right or Q/E for blades; Enter/Space select; Escape/Backspace back; X rescans libraries or selects a folder in the picker (profile dialog elsewhere); F10 opens exit confirmation. On a controller, Back opens the same confirmation. B at blade root stays in the dashboard. Desktop window close exits the preview.

`blades-preview --snapshot build/games.ppm 2` exports a deterministic 720p frame (blade indices 0–4). `--frames 5` runs a finite SDL smoke session. Tests use dummy video/audio drivers; they do not establish physical gamepad/audio/display behaviour.

In this cloud workspace, run `source tools/cloud-env.sh` before CMake or the preview. Dependencies are extracted under `/workspace/deps` because the machine has no sudo/root access. `bash tools/cloud-setup.sh` refreshes these packages and verifies the desktop build. The embedded font can be regenerated with `python3 tools/generate_font.py` when Pillow and Open Sans Regular are installed; regeneration is not needed for normal builds.

## Xbox toolchain and build

See [SDK audit](docs/SDK-AUDIT.md) for source-verified APIs and limits. The pinned SDK revision is `5f6dc726f65f6d171493a3628c2415c03551f27b`.

```sh
sudo apt install build-essential bison flex texinfo libgmp-dev libmpfr-dev libmpc-dev curl xz-utils patch python3 cmake ninja-build git
git clone https://github.com/xenosworks/xsdk.git ../xsdk
git -C ../xsdk checkout 5f6dc726f65f6d171493a3628c2415c03551f27b
export XSDK_ROOT="$(realpath ../xsdk)"
export XSDK_TOOLCHAIN="$HOME/xsdk-gcc"
PREFIX="$XSDK_TOOLCHAIN" WORK="$HOME/xsdk-gcc-work" JOBS=4 bash "$XSDK_ROOT/tools/toolchain/build.sh"
bash tools/build-xbox.sh
```

The SDK builds its patched GCC 15.3/binutils 2.47/newlib 4.6 compiler with archive SHA-256 verification. Do not substitute an ordinary PowerPC compiler: its calling convention differs. The official source domains `ftp.gnu.org` and `sourceware.org` must be accessible. Outbound source downloads were initially denied in the cloud. The user supplied all three archives, and every archive passed the SDK's pinned SHA-256 checks. The complete compiler, SDK library, minimal hello XEX and dashboard XEX now build successfully. See [toolchain provenance](docs/TOOLCHAIN.md) for the rootless setup and retained source cache. The sudo commands above are for the user's LMDE computer.

`tools/build-xbox.sh` builds the required SDK libraries and embedded FFmpeg decoder, compiles the shared GPU shaders with glslang, packs `default.xex`, verifies its digest/import chains and stages the complete USB ZIP. The staging step only runs following a successful build/check; the current build includes a deployable Xbox folder and archives. The title ID `0x424C4453` is a provisional local homebrew identifier, not a registered ID.

## First console test

Copy the entire generated `dist/Blades` folder into a separate `Hdd1:\Blades\` directory:

```text
Blades/
  default.xex
  LICENSE
  XSDK-LICENSE.txt
  THIRD_PARTY_NOTICES.md
  build-info.txt
  SHA256SUMS
  README.txt
  licenses/
  Assets/FONT-LICENSE.txt
  Assets/README.txt
  Config/README.txt
```

Launch `default.xex` from Aurora's file browser. Leave Aurora, Peer Pressure and existing default-dashboard settings intact. Confirm a 720p image, correct colour channels, all blade/submenu transitions, A/B edge handling, controller disconnect/reconnect, navigation sounds and Guide open/close. Back opens a menu: select “Return to system dashboard” to exit. Returning specifically to Aurora depends on the console's current system configuration; the code does not guess its executable path.

Read `default.log` beside the XEX after a crash. It logs boot stages, API failures, controller changes, navigation events and the first submitted frame. Send that log and a console screenshot when reporting a failure. Optional live logging: place `net_log.txt` beside the XEX containing `PC_IP 7371`, then run the SDK's `python3 tools/net_log.py --out net_log` on your PC. Logs can diagnose the last completed stage; this is not an exception handler or crash dump.

## Licensing

Original project code/artwork/synthesised sound: MIT. The adapted DashX360 background retains upstream attribution in `assets/licenses/DASHX360-NOTICE.txt`. Font: Open Sans (Apache 2.0) in [assets/FONT-LICENSE.txt](assets/FONT-LICENSE.txt). The Xbox package includes xsdk's MIT and third-party BSD notices, newlib notices and the GCC runtime exception/licence texts; the staging script copies them. The public XEX embeds no upstream media or proprietary XDK components. The separate premade artwork pack contains converted reference artwork with source attribution; ownership remains with the respective rights holders. Xbox trademarks belong to Microsoft; this project is unaffiliated.

The PNG decoder uses pinned stb_image; its MIT/public-domain notice is included
in `assets/licenses/STB-LICENSE.txt` and in the Xbox package's `licenses` folder.

The current target and historical 0.2.0 reference pass are documented in [docs/VISUAL-REVISION.md](docs/VISUAL-REVISION.md). It is closer to the supplied layout but is not an identical reproduction.

For original icon/logo/button images from a user-supplied 5766 shrdres.xzp, see [docs/LOCAL-RESOURCES.md](docs/LOCAL-RESOURCES.md). These images are not embedded in the public XEX. For blade chrome, runners, caps, tray art and additional icons from a local DashX360 checkout, see [docs/DASHX360-ASSETS.md](docs/DASHX360-ASSETS.md). The converter and instructions can be downloaded alongside the public XEX. Credit: zivvoz / DashX360, inspired by Microsoft Xbox. The supplied Convection glyphs are now supported; exact rasterization, original background matching, layout effects and audio still need comparison.

The 0.6.0 menu-by-menu comparison and validation are recorded in [docs/MENU-ALIGNMENT.md](docs/MENU-ALIGNMENT.md). Every release includes the XEX, a premade artwork ZIP and a navigation walkthrough video; no PC conversion is required.

The animated-background source, implementation and verification are recorded in [docs/ANIMATED-BACKGROUND.md](docs/ANIMATED-BACKGROUND.md). The supplied repository generates its background in code; this release reuses that implementation.

Version 0.8.0 font recovery, source verification and runtime loading are recorded in [docs/CONVECTION-FONT.md](docs/CONVECTION-FONT.md). Microsoft retains ownership of the optional font glyphs; see `assets/licenses/CONVECTION-NOTICE.txt`.


Video playback 0.50: Media > Video plays the exact file within Blades.
Manage folders under Video Marketplace. No separate player installation is
needed. See [video playback](docs/VIDEO-PLAYBACK.md).
