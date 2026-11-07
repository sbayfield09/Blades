# Native video diagnostic 3

Diagnostic 2's supplied selected-video log returns **80070005 (Access denied)**
from XamLaunchURI with the export available. It then exits normally, which
explains returning to the configured default dashboard, Aurora. This log
does not establish interception by Aurora or execution of the Lua bridge.
The user reports the same visible outcome for the library control; its log
was not included, so its exact return value is unknown.

The concrete error in diagnostic 2 was passing flags zero for internal URIs.
In the supplied 17559 XAM, the request's bit31 becomes the alias permission
value at819A06E8. The alias callback at8199FF64 checks that value and the alias's
table flag. Both pam and videolibrary have table flag zero; with bit31 clear,
the callback constructs80070005 at8199FF80..8199FF84. A bounded instruction
check of that branch reproduces denial for both aliases with flags00000000
and permits alias expansion with80000004. It is a static native-branch test,
not a complete URI request or console playback test.

The stock dashboard's helper921902F0 uses80000005 for internal routes.
Diagnostic 3 uses **80000004** to retain the internal URI and activation bits
while keeping fallback bit0 clear. Returned errors remain visible. The native
adapter test checks this exact flag value. No binary patches, private-function
calls or console settings changes are used.

Extract the USB ZIP separately from Blades, then run **control.xex** to test
the native Video Library and **default.xex** to attempt the exact selected
WMV through the bridge. Send both control.log and default.log, and describe
the visible result of each. The sample and bridge are unchanged. The probe
still observes a non-error return for up to ten seconds and does not infer
successful playback from that return code.

Native playback remains unverified. This is a separate diagnostic with no
Blades renderer; the regular Blades0.37 release remains unchanged.
