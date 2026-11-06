# Blades 0.36 Xbox Live side edges walkthrough

[Watch/download the MP4](../dist/Blades-v0.36-walkthrough.mp4)

Fresh 84-second 1280×720 desktop GPU capture at 30 fps with 48 kHz stereo AAC
and the matching artwork pack. Its label identifies profiles, achievements,
FTP, storage and native Guide/keyboard requests as simulations. The synthetic
profile picture and login details are preview fixtures. Native Guide/keyboard
appearance, real profile data, FTP I/O and app-boundary audio noise cannot be
validated by this video; actual protocol/metadata/audio fixtures run separately.

- 00:02 Highlight Sign In: broader warm sheen, darker name band and simulated count
- 00:03 Request native Guide sign-in; simulated completion at 00:05
- 00:07 A on signed-in card requests its native Guide profile menu
- 00:10 X requests native Guide sign-out; simulated completion at 00:12
- 00:14 Sign in again; return to the card at 00:16
- 00:19 Achievements; navigate all three game tabs and their aligned dividers
- 00:25 Enter earned grid; A requests achievement ID 101 at 00:27
- 00:32 Return to Games; navigate Media → System → Computers
- 00:43 / 00:48 Request Xbox keyboard for FTP username/password (desktop notices)
- 00:45 / 00:50 Show updated simulated login details in the right status column
- 00:53 Turn FTP off, then on at 00:55
- 01:06 File Manager; enter simulated USB and X-refresh at 01:08
- 01:12 Xbox Live; show bounded information text with Messages/Friends/Chat

Closing the desktop request notices does not execute the real system UI.
On console, the Guide/keyboard manages those interactions. Credential changes
and on/off persistence are checked with the actual C FTP server in loopback
integration tests. See PROFILE-INTEGRATION.md, ACHIEVEMENTS.md and FTP.md.
Artwork/audio credit: zivvoz / DashX360 and respective rights holders.

The capture uses the final 0.36 renderer: upper-bar shading is inside the
header and fades upward from its bottom edge; footer shading fades downward
from its upper edge. Unselected root/submenu rows retain their thin divider grooves.

Look at the selected lower edge on Games around 00:18 and System around
00:36–00:39, then each FTP control at 00:40–00:55. The highlight now reaches the
divider location and its continuous shadow covers that selected rule. The signed-out/signed-in card at
00:02 / 00:06 shows its new curved shadow ends; no console profile data appears.

The 0.34 capture adds the top divider and mirrored upper group. Compare first
and lower System selections at 00:35–00:39, FTP controls at 00:40–00:55 and
Played Games at 01:01. Lower selections have matching upward/downward edges;
the first row has only its existing lower shadow.

The 0.35 capture shows FTP focus sliding between the toggle, username and
password controls at 00:42,00:47 and 00:51–00:52, with equally spaced status
fields. Menu fill/upper-shadow edges meet the dividers above their buttons.

Version 0.36 shows Live Messages at 01:12–01:14, Friends at 01:14–01:15 and
Chat and IM at 01:15–01:18. The downward line ends at the bottom divider;
Chat has only that divider below it, with its upper group still visible.

The final0.36 capture includes the colour/GScore/logo refinements. Marketplace
is shown at 01:19–01:24 so all five updated backgrounds appear. The signed-in
card at 00:06 shows the corrected GScore baseline; non-Live roots show the
original 360 logo with its cached shadow. Live Messages/Friends/Chat retain
the new bounded edges and last-row divider-only behavior.
