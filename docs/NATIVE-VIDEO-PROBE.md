# Native video diagnostic 1

This separate diagnostic tests opening an exact file with the retail 17559
video player. It is **not a Blades release or a confirmed playback solution**.
The current Blades release stays at 0.37.0. Music and pictures are not tested.

Extract the diagnostic USB ZIP, put a known playable WMV beside its
`default.xex` as `selected.wmv`, then launch that XEX. It should leave for the
retail dashboard and attempt to play that file automatically. Report whether
the exact video starts, what appears if it fails, and send its `default.log`.
If it fails, change `probe-mode.txt` from `uri` to `path` and repeat. These
modes test a `file://` URI and a raw physical device path respectively.
Use your usual dashboard shortcut to return to Blades/Aurora afterwards.

The launcher resolves its own physical folder, checks the kernel and file,
then hands a bounded PAM request to the retail dashboard. A small original
Lua package requests `Xbox.App.ExecuteEpixCommand("EcPlayVideo", {selection})`
once a native scene is active. The dashboard resolves and creates its own
full-screen video player. No custom playback is provided. No Microsoft
binary, extracted stock UI resource, credentials, saved paths or log is
included in the download. Delete the diagnostic folder to remove it.

Static inspection of the supplied update established the retail command's
VideoURL handler and the Lua binding; it has **not** established that this
external package and either local-file syntax work on a real console. Xenia
does not implement these retail dashboard interfaces sufficiently to prove it.
The diagnostic has no Blades renderer or navigation walkthrough: its only
purpose is a console handoff test, separate from the regular release flow.

The XEX builds with the project's pinned xsdk. The package builder needs a
Lua 5.1 `luac`; it converts compiler output to the dashboard's big-endian,
32-bit Lua 5.1 format and verifies an exact architecture round trip.
Host C tests cover path/payload bounds and preflight failures. The packed
bridge is converted back and executed in the real Lua 5.1 VM with simulated
Xbox services to check exact arguments and once-only activation. Native XEX
digest checks pass and reject a deliberately changed image byte.
Actual package loading, media visibility, playback and Back behavior require
the user's 17559 console test.
