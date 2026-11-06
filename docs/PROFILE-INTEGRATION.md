# Profile integration — 0.26

At a blade root, X opens the native Xbox Guide sign-in menu when signed out.
The signed-out Sign In card shows the original green default tile and a
Profiles Found count, with no old account data or statistics; press
Up from the first menu row to highlight it, then A to sign in. This uses
XamShowSigninUI(1, 0), so the Guide manages profile selection and authentication.
The desktop preview explains that this requires the console.

The card uses the active controller's XAM user slot. An already signed-in local
or Live profile is detected on launch; account changes in the Guide refresh the
card. Read-only APIs supply the gamertag, games played, total gamerscore and
achievements earned. Missing fields display --. These are the console profile's
stored totals, not a separate Xbox network synchronization service. Offline
users see the totals stored in their local profile. No achievements or score
values are written or unlocked by this app.

The gamer picture is read via the profile's picture key and the XAM tile reader.
If unavailable, the original neutral gamer tile is used. Picture changes and
statistics invalidate the foreground cache without rebuilding it each frame.
The active controller slot can change without showing another user's stale card.
The same snapshot is displayed on Games, Xbox Live and Media.

When signed in, X at a blade root opens a confirmation with Cancel selected.
Explicit confirmation queues XamUserLogon with the selected slot cleared and
all other signed-in slots preserved. The captured account is revalidated before
sign-out. An account change cancels an open confirmation. Library X shortcuts
retain their existing functions. Starting/exiting Blades never automatically
signs anyone in or out.

The main thread continues presenting while the Guide is visible. Dashboard
input is suppressed until it closes; held inputs must be released before they
can activate a dashboard item. Profile metadata/picture I/O and sign-out use a
worker on hardware thread 5, away from the render loop. Identity is checked
again after slow metadata reads before publishing the result. No account files,
passwords or PINs are read, logged or packaged; snapshots and pictures stay in RAM.

Native API details: SigninState/SigninInfo/XUID identify the active slot;
ReadProfileSettings uses keys 0x10040012 (titles played), 0x10040006 (gamerscore),
0x10040013 (achievements), 0x4064000F (picture key). Bounded readers validate
big-endian result counts, pointers, types, user indices and image sizes.
Optional XamReadTileToTexture ordinal 757 reads the image; optional XamUserLogon
ordinal 523 performs the confirmed session change using flags 0x23. Unavailable
optional APIs leave metadata fallback available and produce a clear operation
error when sign-out is requested. No kernel/profile patching is used.

Host checks cover parsing, zero versus unavailable statistics, malformed data,
ARGB conversion, live profile replacement, slot switching, stale commands,
other-user preservation, confirmation and card focus. CPU/GPU comparisons cover
signed-out/local/Live cards, long names, large totals, unknown statistics,
picture replacement and confirmation overlays. The walkthrough's account
changes and Guide requests are explicitly simulated. Console Guide behavior,
profile totals and picture API compatibility require a physical 360 test.

Source/API provenance: pinned xsdk 5f6dc726f65f6d171493a3628c2415c03551f27b,
public FreeStyle dashboard session and gamer-picture API usage (commit
df89a42edf055370e3383576c93199a3713c41e9), and Xenia's profile-setting layout.
The release uses the unchanged 234-entry premade artwork payload and retains
its attribution; no user account data is included in the release ZIP.

Version 0.25 follows the supplied signed-out screenshot: Sign In heading,
square green ico_64x_DefaultPicture.png tile and a Profiles Found line. The
console worker counts public profile content metadata with content type
0x10000, user NONE and all storage devices. Enumeration is bounded, the handle
is always closed, and failed/incomplete counts display Choose a Profile.
It never calls the account enumerator that exposes account credentials.
Counts refresh while signed out, without delaying signed-in card metadata.
The walkthrough uses two simulated available profiles; this is not a fixed
number in the console build. Zero and singular counts have their own text.

The signed-in card uses the stock GScore.png icon in place of the label's
leading G, as in the second reference. The score value contains digits only.
Numeric columns shrink as needed to keep large totals clear of their labels.
Both PNGs already exist in the compatible 234-entry artwork payload.

Version 0.26 centres the GScore icon using the visible artwork and text bounds.
The supplied video-games.jpg shows a roughly 28 px visible circle at
355..384 / 150..178, extending about 4 px left of the Games label. Its suffix
begins around x387. The source GScore.png has symmetric 2..30 alpha bounds in
a 32 px canvas. At font size 28, the Convection suffix ink spans y+11..25.
Use a 32 px canvas at label_y+2 so both centres are label_y+18; the previous
24 px canvas at the same y was centred 4 px too high. The icon moves 4 px left,
its suffix moves 4 px right, and the numeric-column budget includes the new
spacing. The original PNG is unchanged. Games, Live and Media share this layout.

Version 0.27 gates Achievements through the real Guide sign-in picker, resumes
on successful sign-in and cancels continuation when the picker is dismissed
signed out. Achievement snapshots are scoped to the active user/XUID and are
cleared on account changes. See ACHIEVEMENTS.md. Version 0.28 routes game and achievement selections to
the real Xbox Guide (ordinals 709/736), retaining the same sign-in gate and
identity checks. The old in-app gray list and description are removed.
