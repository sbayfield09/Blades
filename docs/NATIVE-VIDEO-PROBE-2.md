# Native video diagnostic 2

Diagnostic 1 reached its SDK launch handoff in both `uri` and `path` modes,
but the user saw stock Home rather than playback. The same WMV plays manually
in the stock video player. The logs establish that local preflight succeeded;
they do not establish that the retail dashboard read the request or loaded Lua.

This separate diagnostic calls **XamLaunchURI** through its dynamically resolved
17559 ordinal `0x4AC`. The supplied XAM implementation confirms its parameters:
user index, ASCII URI, flags and optional asynchronous context. The diagnostic
uses flags zero and a null context, logs a returned HRESULT verbatim, and stops
on an error. It never follows the call with a manual dashboard launch that
could obscure the original result. A successful return is observed for up to
ten seconds before the diagnostic logs `STILL_RUNNING` and exits normally.
There is no fixed-address native function call or retail binary in the package.

The XEX is provided under two filenames (identical bytes):

- **control.xex** requests `videolibrary`, independent of the sample or bridge.
  It should open the native Video Library. This is a routing control only.
- **default.xex** requests the original bridge using the native `pam:` alias
  and exact selected WMV. Its source and compiled Lua payload are unchanged
  from diagnostic 1 to isolate the routing change. The filename remains a
  file URI unless `probe-mode.txt` explicitly selects a raw path.

Run both and return `control.log` and `default.log` with the visible results.
The package includes the original generated WMV. A returned zero HRESULT or
the appearance of a library is not proof of exact selected-file playback.
The default Blades release remains 0.37.0; this has no dashboard renderer and
is a separate console test rather than a regular Blades release.

Host checks cover both routes, preflight failures, user selection, bounded
waiting, dynamic resolution failures, exact native arguments, URI bounds and
HRESULT propagation. Native XEX checks verify image/import/header digests and
reject a deliberately changed payload byte. Those checks do not establish
retail package acceptance, native UI activation or successful console playback.
