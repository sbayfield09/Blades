# Media libraries — Blades 0.41

Video now decodes inside the Blades executable, preserving the existing chooser.
There is no player handoff or adapter. See [video playback](VIDEO-PLAYBACK.md).
Music and Pictures retain the explicit unimplemented-playback guard.

The following also retains the earlier retail-player investigation as history.

Music, Pictures and Video enter filtered file libraries. Media > Video
Marketplace manages the three categories' paths using the existing Add Folder,
category, scan-depth, rescan/stop, progress and Y-to-remove flow. Saved game and
media paths share the worker/configuration but appear in separate managers.
There is no autoplay during startup or scanning.

Settings retain version 1/version 2 game paths and save version 3 when media
paths are present. Numeric game categories remain unchanged; Music/Pictures/
Video are6/7/8. A folder can be registered for multiple media categories.
Each category retains its own scan depth. Removed paths never delete files.

The current extension allowlists are case-insensitive:

- Music: MP3,WMA,M4A.
- Pictures: JPG,JPEG,PNG,BMP,GIF,TIF,TIFF.
- Video: MKV,WMV,ASF,AVI,MP4,M4V,MOV,MP4V,TS,M2TS,MPG,MPEG,FLV.

These are container/type filters, not a guarantee that every codec/profile,
DRM variant or damaged file is playable. The independent video decoder does not require the retail optional media update. Files from the wrong category,
archives, XEX files and unsupported extensions do not enter the media lists.
Only names are read for media discovery; game metadata parsing is bypassed.
Manual preparation revalidates that the selected file still exists and has
the expected category/type. The existing launch-epoch cancellation is retained.

## Historical retail-player requirement (superseded for Video)

During the retail-player investigation the user required the exact file selected in Blades to open in the designated
Xbox media player. Opening a native library and asking the user to select the
file again was explicitly rejected. A library-only handoff was not accepted. The user subsequently approved the independent player route now described above.

The17559 retail dashboard from the user's reference archive was inspected
read-only. Its1020-byte dashboard launch-data dispatcher at 9217E598 reads the
command at offset4. Commands6/39/44 open musiclibrary/videolibrary/
picturelibrary via XamLaunchURI. Music only checks the flag at offset12 to
choose musiclibrary:LaunchCD. Video and Pictures use fixed library URI strings
and do not consume a filename from the reserved bytes. Putting a path there
would not open the selected file.

The supported library-opening helper is isolated in media.c and covered by
the real-SDK launch-preflight fixture. It is deliberately not wired to A.
The Xbox preparation hook rejects unimplemented media handoffs before any
package mounting or executable launch, preserving the running UI. A verified
selected-file interface or an agreed playback approach is still needed.

Reference: thedev0ps/Xbox-360-Dashboard-Archive, commit
0938397d60975630db108fded069bc04d52740b1,
Metro/V2/Retail/17559/dash.xex. Private binary inspection files are under
build/media-v0.38/reference and must not be packaged or published.

## Validation completed

Real filesystem fixtures cover three extension-filtered catalogues, overlapping
category roots, finite/unlimited depth, symlink loops, typed manual requests,
no autoplay, removed files, persistence/migration and filtered root indexing.
Navigation tests cover all three app entries, the path manager, category/depth
selection and removal with interleaved game/media roots. GPU comparisons cover
populated/empty media libraries and path/category/depth/action screens. These
checks establish discovery/UI behavior; they do not establish file playback.

## Uploaded update extraction and XAM inspection

The five xbox-update parts reassembled to a 115753114-byte ZIP; every ZIP
member passed CRC. The update contains su20076000_00000000, an 11862016-byte
read-only PIRS package. A bounded extractor uses the STFS directory and block
chains to recover $flash_xam.xex. Every used data block and its L0/L1 hash
matches the descriptor root hash. RSA signature verification is not claimed.

The extracted XAM is 2424832 bytes, version 2.0.17559.0, SHA256
78b3dbd8ae866f4258f5444205486ea9f8451afaa30f469c875dcf345f828467.
Its compressed-image block hashes also match and it decodes to a 6356992-byte
PowerPC image. The matching Flash dash is byte-identical to the 17559 dashboard
already inspected. The package also supplies the 49152-byte native MiniMediaPlayer
module; its resources identify the system music-library/playlist interface.
It is not evidence of a generic native picture/video file-player interface.

XamLaunchURI resolves at 0x816FC4B0 to 0x8199FBC8 in this module. Its implementation
accepts a user index, byte URI, flags and optional asynchronous context; it
validates/copies the URI into a 1028-byte request for XAM app 0xFE message 0x22003.
Routing strings include musiclibrary -> dash:library:music:%s, videolibrary ->
dash:library:video:%s and picturelibrary -> dash:library:picture:%s. These identify
the library routing layer; they do not establish a selected-file playback ABI.
No speculative filename payload or native function call has been added to Blades.

Local extraction/inspection evidence is in build/media-v0.38/reference:
extract-update.py, xam-extraction-audit.json and XamLaunchURI-implementation.asm.
The user-provided archive, extracted modules and decoded images remain private
reference files and are excluded from runtime packs and public releases.

## Next investigation targets

Prioritize file-selection handlers over library-opening shortcuts. For music,
Xenia's existing XMP app implementation documents a 36-byte song descriptor
whose first field points to aUTF-16 filename, title-playlist creation message
0x7000D and playback message 0x70002 on XAM app 0xFA. This is a concrete candidate
for selecting a specific song through the system music backend, but its 17559
contract, memory lifetime and native-UI behavior still need verification.
No successful console playback is claimed from emulator code alone.

The extracted 17559 MiniMediaPlayer independently calls XMsgInProcessCall on
app 0xFA at six sites, using messages 0x7001C,0x70041,0x70017,0x7006E,0x70014
and 0x70009. These are native system-music/provider paths to trace alongside
XMP; none was yet established as the selected-file entry point. Their bounded
assembly excerpts are saved privately as miniplayer-XMP-calls.asm.

For pictures/video, trace the 17559 dashboard's actual selected-item activation
from the USB/media list through its viewer/player constructor, recording
whether it requires a filename, URI or provider-owned media object. Then seek
a reachable entry point that preserves that selected item. Verification must
show the exact chosen sample playing/displaying, not just a library appearing.
The supplied update, Flash dashboard, XAM and MiniMediaPlayer are sufficient
for this next static-analysis step; no further upload is currently requested.

## Investigation continued: native activation diagnostic

The full update's 15880192-byte dash.xex decodes to exactly the same
16941056-byte image as Flash dash (SHA256
06a8446849184331dc1b513a894c08ad3f160dcec8473f70cdcae9f54b7c86f9).
The additional update packages were checked; they contain avatar/system/Kinect
data, not a different selected-file launch dispatcher. No extra stock upload
is needed for the current diagnostic.

XMP's real generic handler at81705168 dispatches title playlist creation
0x7000D to81704C40 and size query0x7002E to816FFA40. Native size calculation
at81709598 is136+1000*songCount, not Xenia's4+128*songCount. Always query it.
The36-byte request matches Xenia's field order; title client2 uses the36-byte
UTF-16 song descriptor. System client0 uses a different internal984-byte
record, so changing the client number is not a valid way to obtain system UI.
The native creator copies song data into playlist storage. Stop/delete must
precede freeing that storage. Native synchronous music dispatch rejects
create/play messages: use the verified I/O request path, not an emulator-only
XMsgInProcessCall success. Creation/playback alone still does not establish
opening the designated native music app. No XMP fallback is wired to Blades.

The candidate XamShowPamUI ordinal0x829 is absent in17559: its raw export
offset is zero. Adding the image base to that zero produced a misleading
address in the preliminary export listing. It must not be called.

For video, native external command EcPlayVideo (enum9) reaches92322478,
which constructs the existing FullscreenVideo.xur scene and sets VideoURL
before activation. The retail Lua binding Xbox.App.ExecuteEpixCommand is
used by the dashboard's own scripts with a command name and argument array.
PAM launch command12 can load an XUIZ Lua app package; its loader handles
.xzp/.lex as Lua packages. The bridge uses that candidate route. Native
section:// resource locators contain runtime module handles, so an image
base such as92000000 is not a verified substitute. The diagnostic delegates
resource resolution to the native command rather than constructing such a URI.

tools/native-player-probe contains a separate17559-only diagnostic launcher
and original Lua bridge. It passes a selected.wmv beside the probe XEX as an
exact physical file URI (or raw path in the alternative mode), returns a
native scene and requests EcPlayVideo once after activation. It contains no
decoder, copied retail UI resource or stock executable. Both filename forms,
external package acceptance and actual playback remain console tests. The
normal Blades media guard and existing public0.37 release remain unchanged.

Validation: real Lua5.1 compiler/VM checks after BE32 conversion round trip;
native command argument preservation and once-only activation with simulated
Xbox services; host C checks for path/ABI bounds and launcher failure guards;
native XEX digest verification and a rejected payload mutation. The media
scanner's previously built address/undefined/leak sanitizer fixture also
passes. These checks do not substitute for native-player playback on hardware.

Console feedback after diagnostic1: the user reports returning to the stock
Home menu without playback. This is a failed test, not proof that PAM or the
Lua package loaded. The launcher's failed preflight checks also return Home
through XamLoaderTerminateTitle. Next evidence needed is the diagnostic's own
default.log and confirmation that selected.wmv plays manually in the stock
player. A STOP line identifies a launcher failure; an exit line launching
launch:\\dash.xex establishes that SDK preflight completed, but does not prove
that retail dash accepted launch data or activated the package. Do not label
this route successful or change the filename syntax based on Home alone.

The supplied diagnostic log now confirms no STOP/preflight error: file-uri
mode found the selection and bridge, then logged exit status0 launching
launch:\\dash.xex. The user also confirmed manual playback of the generated
WMV in the stock player. This rules out a missing selection/package in this
run and establishes a successful SDK preflight, not successful retail PAM
loading. There is no post-handoff log or evidence that Main/TransitionEnd
ran. The existing path mode is the next controlled console test: same XEX,
package and video, with only the selected-file syntax changed. Its result
is needed before treating local-file URI syntax as the cause. Preserve the
uploaded console log privately under build/media-v0.38/console-feedback;
do not publish the user's path/log as a release artifact.

The second uploaded log confirms device-path mode also passed preflight and
attempted the same dashboard launch, with Home reported again. Filename syntax
changes alone have not produced playback. Diagnostic2 replaces manual command12
construction with the dynamically resolved XamLaunchURI ordinal0x4AC. The real
17559 wrapper accepts (user, ASCII URI, flags, optional context), dispatches
0x22003 to app0xFE and returns an HRESULT for the synchronous null-context call.
Native routing expands pam: to dash:pam:. The adapter preserves the exact result
and does not overwrite it with a subsequent manual dashboard launch.

Identical executable bytes under control.xex request videolibrary, independently
of the local package/video; default.xex requests the bridge and selected WMV.
The first signed-in slot, or255 when signed out, is logged. A returned error
stops the test. A non-error return is observed for up to10 seconds and then
reported as STILL_RUNNING if this title remains active. No successful native
UI activation or file playback is inferred from HRESULT0. Return control.log
and default.log with both visible results. This is a new console diagnostic,
not a completed media feature or a change to the public Blades0.37 renderer.

Diagnostic2 selected-video console log: XamLaunchURI is available and returns
80070005 with user255, followed by normal probe exit status1. The user reports
Aurora after both tests; control.log was not supplied. This establishes an API
rejection before playback, followed by the configured default-dashboard exit,
not evidence of an intercepted successful URI launch.

Concrete correction: diagnostic2 passed flags0. In17559,819A06E8 derives the
alias permission value from incoming flags bit31. The alias callback at8199FF64
checks it against each entry's flag; pam and videolibrary both have table flag0.
With bit31 clear it builds80070005 at8199FF80..8199FF84. A bounded interpreter
of that actual PowerPC gate reproduces the old denial for both aliases and
allows expansion with80000004. The stock dashboard helper921902F0 uses80000005
for internal routes. Diagnostic3 uses80000004, retaining internal/activation
bits while leaving fallback bit0 clear, and records the chosen flags in logs.
This is a flag-argument correction, not a console setting or binary patch.
Alias-gate success is not proof of complete URI routing or native playback.
The bridge, WMV and two-executable console test remain unchanged; new control
and selected-video logs are needed to validate the corrected request on hardware.

Diagnostic3 hardware result: the user confirms control.xex opens the stock
Video Library; default.xex returns to stock Xbox Home. Its log records
XamLaunchURI returning00000000 with flags80000004 and no STILL_RUNNING/normal
exit line. Routing now works for the control, but the PAM bridge's execution
and selected-file playback are unverified. Do not treat this as success or
attribute the failure to Aurora. Investigate package loading/lifecycle before
further filename changes. Raw logs and the user's device path remain private.

Diagnostic4 corrects the external container:929190A8's suffix gate rejects
ordinary external .xzp with80B04008; .lex goes through XdfLoadXex and reads
the loaded XEX's lua resource. Build an original import-free title DLL with
our Lua XUIZ, rather than renaming the archive. Preflight loads/checks/unloads
only that DLL before handoff. Add optional native text checkpoints and the
verified OnActivate(bool) lifecycle with a once-only guard. Control's flags
and manually playable WMV are retained. Host validation does not prove native
PAM initialization or playback; run the full diagnostic4 folder on hardware.
See NATIVE-VIDEO-PROBE-4.md; no regular media-library launch is enabled yet.

Diagnostic4 console log: kernel XexLoadImage returnedC000007B with a null handle,
then STOP and exit1; Aurora reported. No dashboard handoff or Lua execution
occurred in that selected test. Its control log records videolibrary00000000.
A concrete package error was missed by the hash-only layout validation:
9B000000 lies in the 4 KB XEX range, but the DLL writer retained 64 KB pages.
Diagnostic5 sets IMAGE_PAGE_SIZE_4KB, regenerates all page descriptors/digests
with4096-byte geometry, and verifies coverage independently from the writer's
constants. Old DLL regression now fails on geometry, new DLL has33 pages and
135168-byte coverage; Lua and sample WMV are unchanged. Hardware acceptance and
playback remain unverified. See NATIVE-VIDEO-PROBE-5.md. Logs stay private.

Diagnostic5 hardware feedback: kernel XexLoadImage and lua resource lookup both
return00000000 (1515-byte XUIZ), then the selected native URI request returns
00000000. The user sees feature temporarily unavailable8B050008-F1581F3E-E4119B0C.
The library-control request also returns00000000. No native Lua Main/playback
is established. Uploaded paths/logs are private. This verifies correction of
the earlier image-format failure, not the complete PAM route.

Further17559 inspection: external module loading uses the XDF path.817F5518
resolves the component through8168CAB8/8168C7E0 before calling the kernel loader.
The resolver first queries local component metadata (816EA480); on a miss it
queries appFC/message58003. A negative service result or1510F1 becomes8B050008
at8168C864..8168C868. Generic kernel loading preflight bypasses this catalog,
so its success does not prove the dashboard's second load can resolve a custom
module path. Simply repeating the file syntax/codec tests is not justified.

The builtin PAM registration table has only Hub, ContentApp and Social.Lua.Offline
entries. ContentApp's Scene argument is checked against a defined scene map;
it does not directly import an arbitrary external bridge through that argument.
The five dash URI dispatchers are pam, pamoverlay, library, mediacenter and
networktroubleshooter; no direct video-file dispatcher was found there.
There is no new tested playback build from these findings. A scoped helper
plugin may provide a local bridge-load route; preference requested before
expanding the standalone-XEX deployment into that architecture.

Peer Pressure is the required deployment environment and cannot use DashLaunch.
Its official per-title DLL sideloading offers a different candidate: a title DLL
loaded into retail dash, redirecting only that dashboard's own XdfLoadXex import
for our bridge. Diagnostic6 implements this scoped route, forwards other loads,
checks kernel/dashboard/import shape, and logs attachment/local-load stages.
Host format/dispatch/private import-fixture checks pass. Peer Pressure dashboard
sideloading and native playback require console testing. See NATIVE-VIDEO-PROBE-6.md.
No regular library launch guard has been removed; existing INIs must be preserved.

User reports diagnostic6 still displays the previous error; no new logs were
provided. User now accepts investigation of an independent playback engine,
and selected HG-ERIK/simple360player as a candidate. Prepared its unmodified
v0.2 XEX/fonts with both the existing WMV and a small H.264/AAC sample, complete
GPL source/notices and USB instructions. This is a Peer Pressure hardware
compatibility test, not integrated media playback. See SIMPLE360PLAYER-TEST.md.
Next evidence needed: playerlog.txt and picture/audio/seek results. Existing
media launch guard and regular Blades0.37 delivery remain unchanged.

User confirmed the unmodified Simple360Player0.2 test works and explicitly
requested integration while keeping Blades' file chooser. Version0.38 now adds
a worker-prepared exact autoplay request, a1016-byte XAM packet and an original
local Peer Pressure title adapter. It redirects only the player's two browser
methods after full-method CRC checks and returns to Blades on stop/end; the
Videos view/selection is restored without replaying the intro. No Microsoft
XDK rebuild, DashLaunch or stock dashboard/PAM route is used. The user-tested
player XEX/fonts remain byte-identical; full sources/notices are bundled.
The new handoff/adapter remains hardware-unverified. See VIDEO-PLAYBACK.md.
