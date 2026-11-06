# Profile integration — 0.36

The account card is selectable on Xbox Live, Games and Media whether signed
in or signed out. Up from the first menu item selects it; Down returns to the
menu. A opens native Guide sign-in when signed out, or the current user's
native Guide profile/gamer-card view when signed in. Focus remains on the card
when sign-in completes, and its display cache follows identity/data changes.
At a blade root, X requests the Guide sign-out interface when signed in.
The previous custom sign-out confirmation has been removed. Blades does not
perform a direct background sign-out; the user completes/cancels it in the Guide.

The main thread calls XamShowSigninUI(1,0), XamShowSigninUIp(user,1,4) for the
sign-out-enabled picker, and XamShowGamerCardUI(user). Dynamic ordinals are 702
and 708 for the latter two calls. Targeted actions validate the captured user
slot and XUID immediately before opening; an existing system UI blocks another
request. Rendering continues while Guide input is active, then input is rearmed
on close and profile metadata refreshed. The original retail 5766 dashboard's
calls confirm the user-index arguments for ordinals 702 and 708. The sign-out
flag is XSSUI_FLAGS_ALLOW_SIGNOUT (4). Native appearance/actions still require
physical-console verification; desktop previews show labeled request notices.

Signed-out profile count now checks the standard public package locations on
available HDD, USB and memory devices:
`Content/<profile ID>/FFFE07D1/00010000/<profile ID>`.
Only the STFS magic and public content-type header are read. Account records,
passwords, keys, PINs and GPD payloads are not decoded. Profile IDs are deduplicated
across devices; common content/non-profile files are excluded. An unreadable or
unavailable result remains unknown rather than becoming an invented zero.
The earlier generic current-title content query could report zero for profiles
belonging to the system dashboard. Counting runs on the profile worker and
refreshes while signed out and after Guide activity.

Signed-in cards read the active user's gamertag and real local games, gamerscore
and achievement totals through XAM. Unknown fields show --. Gamer pictures first
request the current user's local gamer tile (type 4), then normal gamer tile
(type 2), independently of the profile's picture-key string. Keyed type 9 is a
last fallback when a valid key exists. ARGB pixels are converted to RGBA for
the renderer. Transient picture failures preserve the last valid picture for
the same identity; changing identity clears it. Missing pictures use the
neutral default tile, not a canned picture presented as the user's choice.

The card name band fades without a hard inner separator, and its lower shadow
remains. Dividers remain between the other rows; the selected lower rule is covered
by its continuous shadow in 0.33.
Version 0.32 compares the original reference recording at 04:23 (highlighted
Sign In), 04:28–04:29 / 04:37 (Games rows), 05:09 (Media) and the uploaded
achievement recording at 22:05 (Sign In). The sheen now has broader shoulders
and a slightly warm tint, fading to transparent at either side. Rows and cards
share alpha=150*sin(pi*(column+0.5)/width)^1.6 with RGB (226,234,223).
These are fitted rendering values, not recovered retail XUI constants; source
footage has camera exposure, compression and moving-background differences.

Card focus reaches the rounded inner frame rather than leaving an inset
rectangle. Its name band is drawn over the focus, preserving the reference's
darker name area for both signed-out and signed-in cards. The supplied
video-games.jpg remains the signed-in card and Gamerscore alignment reference.
Version 0.33 extends the focus to the inner lower border, keeps both frame
outlines above the fill, and replaces the rectangular shadow strip. The shadow
now follows both actual 10-pixel lower frame corners and fades out at either
side; its centre starts immediately below the outer bottom frame. The card
position, picture/stat geometry and horizontal sheen shape remain unchanged.
See FOCUS-AND-CAPS.md for the shared menu edge correction.
The original GScore PNG/size are retained;0.36 aligns its main-ink bottom
with the adjacent word instead of vertical-centre placement. Xbox Live information now has a 28-pixel gutter past the button
column and word-wraps inside its own panel.

The profile worker revalidates identity after reading metadata/pictures.
Snapshots and cached images stay in RAM and are not packaged in releases.
Actual-adapter host fixtures verify current-user tile/stat arguments, picture
retention, Guide targets/flags and stale/busy guards. A real-directory fixture
checks package counting, cross-device duplicates and invalid/common exclusions.
The walkthrough's accounts/pictures/counts are explicit simulations; check
actual Guide actions, profile pictures/counts and account changes on a 360.

![Signed-out highlighted card, simulated profile count](images/signin-highlight-v0.36.png)

![Signed-in selectable card, simulated profile/picture](images/profile-card-v0.36.png)

Version 0.34 adds the button-list top divider beneath the card and a matching
upward group shadow for lower menu selections. The card's own frame, focus,
rounded shadow and all native profile actions are retained.

Version 0.35 aligns menu-row upper edges with their dividers. The account card,
its rounded shadow, profile data and native Guide actions are unchanged.

See COLOUR-AND-BRANDING.md for the measured icon/main-text baseline.
