# Blades 0.24 profile integration walkthrough

[Watch or download the MP4](../dist/Blades-v0.24-walkthrough.mp4)

117 seconds, 1280×720 at 30 fps with 48 kHz stereo audio. This is fresh desktop
GPU playback of the release navigation/renderer using the matching artwork
pack. Profiles, Guide sign-in requests, storage listings and launch preparation are explicitly simulated. It is
not a physical Xbox recording or evidence of a real XAM title handoff or
actual CD/DVD playback. Optical states in the final section are simulated too.

- 00:00 Startup and the signed-out Sign In card
- 00:02 Highlight the account card; A requests Guide sign-in (simulation notice)
- 00:05 Example signed-in profile with name, games, gamerscore and achievements
- 00:08 Card statistics refresh without restarting
- 00:10 X opens sign-out confirmation; select Sign out
- 00:12 Signed-out card clears all account data
- 00:14 A on the card requests the Guide again; example local sign-in at 00:17
- 00:22 Media blade and Music submenu
- 00:27 Games → Played Games; File Manager below Homebrew
- 00:32 Storage navigation and a manual Utility.xex launch request
- 00:43 Scan Folders; add a path and set depth (Unlimited is first)
- 00:56 Live right-panel scan progress, completion and retained results
- 01:13 Remove a saved scan path with Y
- 01:22 Other blades and submenus, including Console Settings
- 01:43 Audio CD, DVD and data-disc labels/manual handoff requests
- 01:55 Eject request

The example Apps/Homebrew paths were configured before capture. The simulator
keeps recording after manual launch requests; native code validates the selected
plain XEX and hands its path to XAM. Compound-XEX exclusion is verified against
real host filesystem fixtures, including checks that excluded files never reach
the metadata reader.

See [LIBRARY.md](LIBRARY.md) for controls, formats and physical-console checks.
Artwork/sounds: zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard.
The compatible 234-entry artwork payload is unchanged from v0.23.

This is a newly rendered v0.24 capture. Optical launch traces target the
corrected /Device/Flash/dash.xex device path. The successful XAM alias/data
preparation is covered by the real-SDK host regression, not by physical
playback in this desktop capture.
