# Blades 0.33 row-edge and curved-card-shadow walkthrough

[Watch/download the MP4](../dist/Blades-v0.33-walkthrough.mp4)

Fresh 78-second 1280×720 desktop GPU capture at 30 fps with 48 kHz stereo AAC
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

The capture uses the final 0.33 renderer: upper-bar shading is inside the
header and fades upward from its bottom edge; footer shading fades downward
from its upper edge. Unselected root/submenu rows retain their thin divider grooves.

Look at the selected lower edge on Games around 00:18 and System around
00:36–00:39, then each FTP control at 00:40–00:55. The highlight now reaches the
divider location and its continuous shadow covers that selected rule. The signed-out/signed-in card at
00:02 / 00:06 shows its new curved shadow ends; no console profile data appears.
