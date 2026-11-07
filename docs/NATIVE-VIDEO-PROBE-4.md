# Native video diagnostic 4

The user confirmed diagnostic3/control.xex opens the stock Video Library;
default.xex instead returns stock Home. Its log records HRESULT00000000 and
flags80000004, then ends during the handoff observation. This establishes the
library control's visible success and the selected request's acceptance, not
execution of the Lua bridge. The uploaded device path/log stays private.

The supplied17559 dashboard contains another rejection later in the path:
929190A8 admits external .xex and .lex, and the special BuiltIn.*.xzp form.
An ordinary external native-video.xzp reaches80B04008 at92919168..92919170.
The bounded suffix-gate check executes those selection instructions with host
strcmp stubs and a precomputed BuiltIn predicate: the old path is rejected,
.lex/.xex paths pass that gate. This is not an emulated full loader or playback.

External .lex goes through the XdfLoadXex API, then the loader reads the loaded
module's 'lua' resource. A supplied stock .lex confirms XEX module flags9,
PE DLL characteristics and a Lua5.1 BE32 XUIZ resource. No reference bytes are
copied into the new diagnostic. build_lex.py uses the pinned xsdk XEX writer
with our own8-byte return-TRUE entry stub and original Lua resource. The DLL
has no imports/TLS, uses base9B000000, and contains no player implementation.
Renaming a loose archive would still give the native loader the wrong format.

The launcher preflight now resolves kernel XexLoadImage0x199 and
XexUnloadImage0x1A1, loads only our DLL with module type9, reads its lua resource
through XexGetModuleSection, logs statuses, and unloads it before handoff.
Failure stops the launch. This check establishes neither Lua execution nor PAM
activation in the other title; subsequent errors may still return Home.

The original bridge retains EcPlayVideo and exact selected-file parameters.
It adds optional XuiText checkpoint labels and responds to the verified optional
global OnActivate(boolean), as well as TransitionEnd. The host at927CF538 calls
OnActivate with one boolean; this avoids relying solely on transition timelines
in a programmatically created scene. A shared guard prevents repeated events
or later reactivation from restarting playback. Command errors update the text
when available; an optional visual failure does not suppress the command.
There are no fixed-address function calls or native binary patches.

Download/extract NativeVideoProbe-4-USB.zip, keeping Assets/native-video.lex and
selected.wmv together, and run default.xex. The control already worked and need
not be repeated. Return default.log and describe the visible outcome; include a
photo if checkpoint/error text remains. The sample WMV is unchanged and has
already played manually in the stock player on the user's console.

Validation covers the target XEX, resource-DLL header/image/import digest chains,
embedded XUIZ/BE32 Lua round trip, mutation rejection, launcher/preflight failure
paths, and once-only exact-selection bridge requests in Lua5.1 with mocked Xbox
services. It does not verify console playback or native Lua/XUI availability.
This is an isolated diagnostic: regular Blades0.37 artifacts remain unchanged;
there is no Blades renderer/navigation video for this separate launcher.
