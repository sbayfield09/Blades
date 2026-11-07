Blades native video diagnostic 1 — NOT a Blades update

This tests a candidate handoff to the retail 17559 full-screen video player.
Successful playback has NOT been verified on a console. Blades itself remains
unchanged. This app has its own title ID and does not write console settings.
There are no Microsoft executables, decoded images or stock UI resources in
this download: it uses the resources already installed on your console.

1. Extract NativeVideoProbe onto your USB drive, keeping its Assets folder.
2. Copy a WMV that already plays in the stock Xbox video player into that folder.
   Name the copy selected.wmv. Do not rename an MP4/AVI to WMV.
3. Run NativeVideoProbe/default.xex from Aurora or another file manager.
4. Check whether THAT video opens and plays automatically, without selecting it
   again. Report what happens, including any retail error screen, and send
   NativeVideoProbe/default.log. B should use the retail player's own Back flow.
5. If the first test fails, edit probe-mode.txt to contain path, save it and
   run default.xex again. Send the second result and log as well. The first
   mode supplies a file:// URI; the second supplies a raw physical device path.
   Both use the retail player's own activation command and are experimental.

Expect the app to leave for the retail dashboard. Return to Blades/Aurora using
your usual dashboard shortcut. Automatic return to Blades is not implemented
in this diagnostic. It does not test music, pictures, codec compatibility or
retail media-path visibility on every USB/HDD layout. Xenia cannot validate
these retail dashboard/PAM interfaces.

Delete NativeVideoProbe to remove it. Logs are local; no credentials are read.
Original bridge and launcher: Blades project. Xbox UI/player/resources remain
Microsoft's and are accessed from the user's installed dashboard.
