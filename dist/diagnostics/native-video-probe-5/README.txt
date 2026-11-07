Blades native video diagnostic 5 — separate from Blades 0.37

Diagnostic 4's selected test stopped before handoff: XexLoadImage returned
C000007B (invalid image format), then the diagnostic exited to Aurora. Its
control log records a non-error Video Library request. Lua did not run in the
failed selected test; this result says nothing about WMV/path compatibility.

Diagnostic 5 fixes a concrete image-layout error in the original resource DLL:
base9B000000 belongs to the 4 KB XEX address range, but diagnostic4 used 64 KB
page descriptors and left IMAGE_PAGE_SIZE_4KB clear. Its three descriptors
covered only12 KB under that address range's geometry, before the entry code
and Lua resource. The DLL now sets the 4 KB image flag and has33 descriptors
covering135168 bytes, including all code/resource data. The validator now checks
the page flag against the address range, rejects the previous malformed DLL,
and checks the full page/hash chain. Native acceptance still needs a console
test; this format correction is not proof that playback now works.

The Lua package and sample WMV are byte-identical to diagnostic4. The .lex is
still an original XEX resource DLL, not a renamed loose archive. Its entry
returns TRUE; it has no imports, TLS, renderer or playback engine.

The launcher now loads that DLL, checks its lua XUIZ resource, logs the result,
and unloads it before asking XAM to switch to the retail dashboard. That check
does not run Lua or prove that dashboard PAM can load/activate the module.
The bridge also has optional checkpoint text and responds to the dashboard's
OnActivate callback, as well as TransitionEnd, with a guard against restarting.
It requests the stock player's EcPlayVideo command with the exact file URI.
Native playback through this bridge still needs console verification.

1. Extract NativeVideoProbe5 onto USB, keeping Assets and selected.wmv together.
   Use this whole folder: the earlier standalone XEX/Assets do not match it.
2. Run NativeVideoProbe5/default.xex. No need to repeat the control test yet.
3. Report whether selected.wmv plays, stock Home appears, or checkpoint/error
   text appears. If text remains, send a photo and default.log from this folder.
   Send default.log even if it returns Home/Aurora without visible text.

control.xex remains the independent Video Library control, with identical XEX
bytes. It does not load the DLL or require the WMV. The selected test still
uses internal XamLaunchURI flags80000004; no fallback command or native binary
patch is applied. It requires kernel2.0.17559. The first signed-in user is used,
or255 when signed out. Native return errors and DLL failures stop the test.
A non-error native return is observed for up to10 seconds; STILL_RUNNING means
the probe remained active. Returning0 alone is never proof of playback.

Optional checkpoints (only if Main runs and a native text visual is available):
  Native video bridge loaded; waiting for activation...
  Native bridge active: requesting Xbox video player...
  Native video request failed: <Lua/native error>
Absence of text does not prove Main failed: creating the optional visual can
fail independently. The pre-handoff DLL check cannot capture post-handoff errors.

probe-mode.txt defaults to uri. Keep it unchanged for this test. Its older path
option only changes selected-file syntax; the previous syntax tests both failed.
Return to Blades/Aurora using your usual shortcut after the native app opens.
Automatic return to Blades, Music/Pictures playback and Xenia retail-dashboard
support are not implemented by this diagnostic.

This is not a new Blades release and has no Blades renderer/walkthrough. The
public Blades0.37 XEX/artwork/video/USB ZIP remain unchanged. The Assets ZIP here
contains the diagnostic DLL only. There are no Microsoft executables, stock UI
payloads or extracted update files in this download. Xbox's player/resources
come from your installed dashboard. Original launcher, Lua and container source
are included under Source. The test WMV is self-generated.

Build requires the pinned xsdk (5f6dc726f65f6d171493a3628c2415c03551f27b),
its PowerPC toolchain, Python3, CMake, and a Lua5.1 luac:
  python3 build_bridge.py --luac /path/to/luac --output /tmp/bridge
  python3 build_lex.py --converter /path/to/xsdk/tools/elf2xex.py --package /tmp/bridge/native-video.xzp --output /tmp/bridge/native-video.lex
  cmake -S . -B /tmp/probe-build -DCMAKE_TOOLCHAIN_FILE=/path/to/xsdk/cmake/xsdk.cmake -DXSDK_TOOLCHAIN=/path/to/xsdk-gcc -DCMAKE_BUILD_TYPE=Release
  cmake --build /tmp/probe-build
Copy native-video.lex to Assets beside the generated default.xex and WMV.
No stock binary is needed to build this diagnostic. check_loader_gate.py is an
optional research check requiring your own private decoded17559 reference.

Delete NativeVideoProbe5 to remove it. Console settings are not changed. Logs
stay local. No credentials are collected. Hardware playback remains unverified.
