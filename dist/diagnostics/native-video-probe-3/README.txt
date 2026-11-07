Blades native video diagnostic 3 — NOT a Blades update

This tests a candidate handoff to the retail 17559 full-screen video player.
Successful playback has NOT been verified on a console. Blades itself remains
unchanged. This app has its own title ID and does not write console settings.
There are no Microsoft executables, decoded images or stock UI resources in
this download: it uses the resources already installed on your console.

Diagnostic 1 found the selected video and bridge and attempted to launch dash,
but returned Home with both file-URI and raw-path selections. The test WMV plays
manually in the stock player. These facts do not establish that the bridge ran.

Diagnostic 2 returned 80070005 (Access denied) with flags zero, then exited
normally to Aurora. This was a native alias-routing rejection, not evidence
that Aurora intercepted a successful request. The supplied 17559 alias-gate
instructions reproduce that rejection for pam/videolibrary with bit31 clear.

Diagnostic 3 corrects the flags to 80000004: the stock dashboard's internal
URI/activation flags, keeping fallback bit0 clear. The alias-gate branch allows
these aliases with that flag set. This verifies the flag correction only;
subsequent native routing, bridge loading and playback still need console tests.
The route, flags, user slot, API availability and returned HRESULT are logged.
The same original Lua bridge and sample WMV are retained for comparison.

1. Extract NativeVideoProbe3 onto USB, keeping its Assets and selected.wmv.
2. Run control.xex. This requests the native Video Library WITHOUT loading the
   bridge or selecting a file. Report whether that library opens or Home appears.
   Opening the library is only a routing control, not successful file playback.
3. Run default.xex. This requests the bridge and exact selected.wmv. Report
   whether that specific video plays, Home appears, or an error is displayed.
4. Send BOTH NativeVideoProbe3/control.log and NativeVideoProbe3/default.log.

If a native request returns without an error, the launcher waits up to 10 seconds
for a title switch. If still running then, it records STILL_RUNNING and returns
Home. A zero HRESULT alone is not proof that the library, bridge or video opened.
A log ending at 'native XamLaunchURI begin' without a returned-result line can
mean the title was terminated during the call; report the visible result too.

probe-mode.txt defaults to uri. Its alternative path mode changes only the
selected-file syntax, as in diagnostic 1; no further syntax test is requested
until the new routing results have been checked.

The native API may leave for the retail dashboard. Return to Blades/Aurora using
your usual dashboard shortcut. Automatic return to Blades is not implemented
in this diagnostic. It does not test music, pictures, codec compatibility or
retail media-path visibility on every USB/HDD layout. Xenia cannot validate
these retail dashboard/PAM interfaces.

Delete NativeVideoProbe to remove it. Logs are local; no credentials are read.
Original bridge and launcher: Blades project. Xbox UI/player/resources remain
Microsoft's and are accessed from the user's installed dashboard.
