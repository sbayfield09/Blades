# Native video diagnostic 6: Peer Pressure title DLL

Peer Pressure is required; DashLaunch is excluded. Its [official DLL sideloading
feature](https://github.com/grimdoomer/Xbox360PeerPressure/wiki/Softmod-Features#dll-sideloading)
loads title DLLs before the target entry point and unloads them when it exits.
The configuration follows the official v1.0.477 example. Whether this mechanism
loads our helper into retail dash17559 still needs console verification.

Diagnostic5 console logs establish that kernel loading and Lua resource lookup
succeed. The subsequent native handoff returns00000000 but displays8B050008.
Private17559 inspection identifies this code in component-catalog resolution
before the kernel image loader. This motivates a scoped local-loading test;
it does not establish that every later stage will work.

Diagnostic6 adds original `BladesNativeMedia.xex`, a freestanding title DLL with
kernel imports only, no CRT, threads or system module. At attachment it requires
kernel17559 and dashboard titleFFFE07D1/version2.0.17559.0. It parses the target's
import table, finds exactly one resolved XAM XdfLoadXex thunk, checks its four
instructions, and redirects only that16-byte thunk in the dashboard image.
XAM and kernel code are unchanged. Unrelated calls forward unchanged. A matching
physical Blades bridge path uses kernel loading and validates our bridge's title
ID and Lua XUIZ resource. Synchronous success is supported by the inspected
native caller. Detachment restores the original words only if ours remain.

The actual retail import table contains three records but two library names:
two XAM records, then the kernel record. The parser consumes the entire table,
rather than mistaking the name count for the record count. Host checks use that
private table with simulated resolved import instructions, reject ambiguous or
modified thunks, and prove only16 bytes change and restore exactly. Runtime
attachment, module lifetime and native playback are not established by this test.

The USB ZIP includes the launcher, unchanged manually playable sample WMV,
unchanged diagnostic5 bridge, helper/config and recursive original source.
Read its README before installing the two helper files under
`Hdd1:\PeerPressure\SideLoading`. Preserve any existing `fffe07d1.ini`; send it
before merging. Then run default.xex and collect both the USB default.log and
HDD BladesNativeMedia.log. A missing helper log is useful evidence too.

[USB ZIP](../dist/diagnostics/native-video-probe-6/NativeVideoProbe-6-USB.zip)
includes everything. [Launcher](../dist/diagnostics/native-video-probe-6/default.xex)
and [diagnostic assets/helper ZIP](../dist/diagnostics/native-video-probe-6/NativeVideoProbe-6-Assets.zip)
are also provided. [Validation](../dist/diagnostics/native-video-probe-6/validation.json)
records host checks and hashes, never private logs or stock binaries.

This is a separate diagnostic, not a new Blades release. The regular0.37
XEX/artwork/walkthrough/USB delivery remains unchanged. There is no navigation
video for this isolated launcher. Exact native playback remains unverified;
the normal media-library launch guard remains in place.
