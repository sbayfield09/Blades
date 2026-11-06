# Blades 0.23 media-launch correction walkthrough

[Watch or download the MP4](../dist/Blades-v0.23-walkthrough.mp4)

97 seconds, 1280×720 at 30 fps with 48 kHz stereo audio. This is fresh desktop
GPU playback of the release navigation/renderer using the matching artwork
pack. Storage listings and launch preparation are explicitly simulated. It is
not a physical Xbox recording or evidence of a real XAM title handoff or
actual CD/DVD playback. Optical states in the final section are simulated too.

- 00:00 Startup, Media blade and Music submenu
- 00:07 Games → Played Games; File Manager is directly below Homebrew
- 00:12 Storage/folder navigation and selecting Utility.xex to launch manually
- 00:19 Parent-folder navigation and return to Played Games
- 00:23 Scan Folders; add an HDD Games path
- 00:31 Depth picker: Unlimited is at the top
- 00:36 Add/scan with live right-panel progress
- 00:44 Scan Now; progress changes to completed counts while it stays highlighted
- 00:48 Completed right-panel results remain, with no lower-left status summary
- 00:51 Move off Scan Now; normal item details return
- 00:53 Y removes the selected Games scan path (files remain untouched)
- 00:56 Return through Played Games and the other blades
- 01:13 System → Console Settings and return
- 01:23 Audio CD label and a manual system-player request
- 01:28 DVD video label and manual player request
- 01:32 Data-disc label, manual dashboard request and eject

The example Apps/Homebrew paths were configured before capture. The simulator
keeps recording after manual launch requests; native code validates the selected
plain XEX and hands its path to XAM. Compound-XEX exclusion is verified against
real host filesystem fixtures, including checks that excluded files never reach
the metadata reader.

See [LIBRARY.md](LIBRARY.md) for controls, formats and physical-console checks.
Artwork/sounds: zivvoz / DashX360, inspired by Microsoft's Xbox 360 Dashboard.
The compatible 234-entry artwork payload is unchanged from versions 0.18–0.21.

This is a newly rendered v0.23 capture. Optical launch traces target the
corrected /Device/Flash/dash.xex device path. The successful XAM alias/data
preparation is covered by the real-SDK host regression, not by physical
playback in this desktop capture.
