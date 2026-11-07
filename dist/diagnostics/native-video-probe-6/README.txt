Blades native video diagnostic 6 — Peer Pressure test, separate from Blades 0.37

This test uses Peer Pressure's documented per-title DLL sideloading. It does
not use DashLaunch. Peer Pressure's loading of this helper into the stock dash,
the native Lua bridge and exact selected-file playback require console testing.
Build/import tests are not proof of native playback.

Diagnostic5 proved that the original bridge DLL loads and contains its Lua
resource. The later 8B050008 error is consistent with the native component
catalog failing to resolve the external bridge. Diagnostic6 adds an original
title DLL to redirect the stock dashboard's own XdfLoadXex import for that
bridge only. Other component loads call the original function. XAM and kernel
code are not patched. The title DLL unloads with the dashboard and restores
its import on detach when our redirect is still present.

ONE-TIME HELPER INSTALLATION — stock dashboard/kernel 2.0.17559.0 only

1. Look in Hdd1:\PeerPressure\SideLoading for fffe07d1.ini.
   If it already exists, DO NOT overwrite or blindly append to it. Send that
   existing INI first so its current DLL configuration can be preserved.
2. Copy these TWO files from the included Install-HDD\PeerPressure\SideLoading
   folder into Hdd1:\PeerPressure\SideLoading\:
     BladesNativeMedia.xex
     fffe07d1.ini
   Create SideLoading if missing. Do not replace the entire PeerPressure folder.
   The included INI targets only the retail17559 dashboard title/version.
3. Keep NativeVideoProbe6 on USB with Assets, selected.wmv and default.xex
   together. Keep probe-mode.txt set to uri. The Install-HDD folder on USB is
   only the source for step2; leaving it on USB does not install the helper.
4. Run NativeVideoProbe6/default.xex. No need to repeat control.xex initially.
   It checks helper/config presence and bridge validity, then requests the
   native dashboard handoff. Presence does not prove that the helper attached.
5. Report what appears: video, stock Home, Aurora, checkpoint or error.
   Send BOTH local log files after this test:
     USB:\NativeVideoProbe6\default.log
     Hdd1:\PeerPressure\SideLoading\BladesNativeMedia.log
   If the HDD log is absent, report that too. Peer Pressure documents a DLL-load
   notification; report whether you saw one. If an error appears, send a photo.

The helper log has no selected filename or credentials. It distinguishes:
  Peer Pressure title helper attached; dashboard import only 00000000
  Blades bridge local load <kernel result>
  Bridge returned to native PAM host 00000000
An unsupported image/import leaves normal routing intact and records refusal.
Attachment or a successful local load alone does not establish Lua execution.
The log is reset on attachment; collect it after the selected-file test before
another stock dashboard launch. Missing logs may mean no sideload occurred.

Optional native UI checkpoints, only if the Lua bridge and text visual run:
  Native video bridge loaded; waiting for activation...
  Native bridge active: requesting Xbox video player...
  Native video request failed: <Lua/native error>
Absence of text is inconclusive. The video and Lua resource DLL are unchanged
from diagnostic5; the user previously confirmed this WMV plays manually in the
stock player. The bridge requests EcPlayVideo with the exact file URI.

control.xex has identical XEX bytes but selects videolibrary from its filename.
It remains independent of the bridge/video/helper presence checks. Default
uses XamLaunchURI flags80000004, first signed-in user or255; errors stop the
test, and a non-error result is observed for up to10 seconds. STILL_RUNNING
means the launcher remained active, not native playback success.

TO REMOVE: remove only the two helper files you installed, then leave/reopen
stock dashboard so its title DLL unloads. Do not delete other sideload configs.
Remove NativeVideoProbe6 from USB when finished. No firmware or stock files
are changed. Use your normal shortcut to return to Blades/Aurora after testing.

No Microsoft executables, extracted update resources, Peer Pressure binaries
or custom playback engine are distributed. The original helper, launcher,
Lua and container sources are included. The sample WMV is self-generated.
This isolated diagnostic has no Blades renderer or navigation walkthrough;
regular Blades0.37 XEX/artwork/video/USB ZIP remain unchanged. Music/Pictures,
automatic return to Blades and Xenia retail-player support are not implemented.

OFFICIAL PEER PRESSURE DOCUMENTATION
https://github.com/grimdoomer/Xbox360PeerPressure/wiki/Softmod-Features#dll-sideloading
The INI uses the official v1.0.477 example syntax. This test uses title DLLs,
not system DLLs, and its hook stays in the target dashboard's own XEX.

BUILD FROM Source (no stock binaries needed)
Requires xsdk commit5f6dc726f65f6d171493a3628c2415c03551f27b, its PowerPC
toolchain, Python3, CMake and Lua5.1 luac. From Source:
  python3 build_bridge.py --luac /path/to/luac --output /tmp/bridge
  python3 build_lex.py --converter /path/to/xsdk/tools/elf2xex.py --package /tmp/bridge/native-video.xzp --output /tmp/bridge/native-video.lex
  python3 peerpressure/build_helper.py --sdk /path/to/xsdk --toolchain /path/to/xsdk-gcc --output /tmp/helper
  cmake -S . -B /tmp/probe-build -DCMAKE_TOOLCHAIN_FILE=/path/to/xsdk/cmake/xsdk.cmake -DXSDK_TOOLCHAIN=/path/to/xsdk-gcc -DCMAKE_BUILD_TYPE=Release
  cmake --build /tmp/probe-build
verify_xex.py --title-dll checks the helper format; --resource-dll checks the
Lua resource DLL. Neither establishes console compatibility. Private stock
fixture checks require your own reference images and are optional for building.
