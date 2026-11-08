# Jellyfin in Blades0.46

Media > Video now contains **Jellyfin** and **Local**. These are navigation
folders: Jellyfin lists server videos; Local lists the existing scanned files.
No library media is copied to USB or downloaded in full.
New local scan folders added through Video Marketplace are shared by Music,
Pictures and Local Video and sorted by supported filename extension. Choose
folder and depth only. Existing typed saved paths are left unchanged; no migration.
Music/Pictures playback remains unfinished. This change does not affect Jellyfin
server libraries or saved resume positions.

## Connect

1. Open Media > Media Center.
2. Select Server Address and use the Xbox keyboard to enter the server's LAN
   IPv4 address and optional port, such as10.0.0.20:8096. Optional http:// is
   accepted. Default port8096. This uses XenonFin's native HTTP scope: numeric
   IPv4, no DNS, HTTPS, redirects or reverse-proxy base paths.
3. Enter your Jellyfin username and password, then select Connect. This is a
   Jellyfin account, independent of the Xbox Guide/profile account. Entry uses
   up to128 printable ASCII characters, matching the old native client.
4. Open Video > Jellyfin. Select a library, show/season/folder, then a video.
   Previous Page/Next Page traverse10-item pages; B returns, X refreshes.
   Continue Watching and Next Up appear above the video libraries.

Video libraries exclude Music/Books/Photos/Live TV views. Child queries include
movies, series, seasons, episodes, videos, folders and collections. The status
panel reports loading/errors. Network and settings I/O run on a worker; B/other
blades cancel outstanding navigation. Pages already loaded survive failures.

The server must allow this user to transcode video/audio. Blades negotiates
PlaybackInfo, validates a local progressive source, then requests a documented
MPEG-TS stream capped at1280x720/30fps, MPEG2 video8Mbps and stereo MP2/48kHz.
This adapts XenonFin's native compatible-stream approach to Blades' working
MPEG2 decoder. Source H264/HEVC/etc is decoded/transcoded on the SERVER; the
Xbox decodes the resulting MPEG2 with its existing CPU/GPU display pipeline.
No server plugin, DashLaunch, extra XEX or stock-player handoff is needed.

A pauses/resumes, Left/Right seeks back/forward10seconds, X switches fit/stretch,
B/Back returns to the same Jellyfin list. Guide opening pauses. Seeking restarts
server transcoding at the requested full-item timestamp; a short buffering delay
is expected. Old decoder, packet, audio and picture queues are drained before
reopening, and both clocks are mapped back to the full item timeline. Seeking
while paused stays paused. Rapid presses accumulate against the requested
position and cancel superseded opens. Targets are clamped to the item bounds.
The server playback session stays the same, with CanSeek reported and absolute
positions in start/progress/stop messages. Local byte seeking and the verified
0.43 decoder/GPU optimisations are retained. Jellyfin subtitles, HDR
validation, Live TV, external media origins and stream-quality choices are not
implemented. Server HEVC->MPEG2 compatibility is tested with SDR synthetic media.

## Continue Watching, Next Up and resume

The two folders use Jellyfin's own account-specific Resume and Shows/NextUp
endpoints and preserve the server ordering. Empty lists remain valid folders;
X refreshes and paging works like other lists. Next Up follows the server's
watched-episode history rather than calculating episode order locally.

Opening a video from any Jellyfin folder fetches fresh user data, even when the
list was cached, and starts at PlaybackPositionTicks. Missing/zero positions
start at the beginning; positions at/past the duration also restart. A valid
partial position is respected even on a previously watched item. Stopping
refreshes the current folder so Continue Watching/Next Up can reflect changes.
Jellyfin's server-side minimum duration, resume percentages and watched rules
still determine which videos appear and which positions it saves. No local
watch-history database or separate Xbox profile mapping is used.

## Saved connection and status

Server address, username, device ID and a bearer session token are saved
atomically to Config/jellyfin-settings.json. Passwords are never saved; the
in-memory password is erased after successful sign-in. A saved session is
validated when Blades starts. Disconnect erases the token locally; Remove
Connection also clears the server and username. Neither revokes every session
on the Jellyfin account. An expired token asks you to sign in again.

This private session file is unencrypted on the console filesystem. Keep it
private; it is excluded from all release/source ZIPs, as are personal scan
paths and FTP settings. Requests, passwords, tokens and authenticated media
URLs are never logged or rendered. HTTP credentials travel in the native
HTTP connection; configure it for the LAN as with the original native client.

Playback start/progress/stop reports update Jellyfin watch history. Stop/error
cancels the media socket and requests active-encoding cleanup. Clean shutdown
allows up to2seconds for worker cleanup; network failure or power loss cannot
guarantee delivery. Local no-autoplay behaviour is preserved.

## Source and validation

Based on sbayfield09/XenonFin at554adb8d79a251e4b502ac4a87a328cbe5055b09.
Original/ported native HTTP/JSON/auth/browser sources and checksums are retained
in third_party/xenonfin. Its lwIP transport is replaced by xsdk BSD sockets.
Authentication and library APIs, bounded framing/JSON and session cleanup follow
that project. Its MPEG1/480p profile is adapted to MPEG2/720p with stereo MP2.
The renderer/player is the working Blades code, without a new decoder backend.

Checks include upstream malformed HTTP/JSON regressions, real worker/player
fixtures with fragmented media, pagination, cancellation,401/503/truncation,
fresh/stale/finished resume data, paused/rapid/end-bound seeking and seek failure,
plus UI and application ASan/UBSan. An isolated official Jellyfin10.10.7 test
uses generated credentials and synthetic HEVC/AAC movies/episodes. It verifies
a real next episode and Continue Watching result, resume at4seconds, MPEG2/MP2
playback, forward/back/paused seeks, full-item clocks and saved stop position.
Only that short-clip test server lowers its minimum resume duration; Blades
never changes a user's server configuration. Native compilation/integrity and
renderer checks pass; real Xbox network/keyboard/streaming need hardware testing.
