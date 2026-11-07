Blades0.38 video playback component

Choose videos in Blades under Media > Video. Manage scan folders under
Media > Video Marketplace. The selected file opens directly in Simple360Player;
its file browser is bypassed. A pauses/resumes, Left/Right seeks, Y opens
playback options, and B stops/returns to Blades. Finishing a file also returns.
The intro is skipped on return and the Videos selection is restored.

Keep this whole Players/Simple360Player folder with Blades. The local
sideload.ini uses Peer Pressure's native title-DLL loading; no DashLaunch or
HDD dashboard-plugin installation is needed. This package is meant to be
launched from Blades, not used as a separate file browser.

The upstream0.2 XEX/fonts are unmodified and user-tested on Peer Pressure.
This new adapter/handoff/return path still needs console verification. If it
fails, collect Blades/default.log and both Players/Simple360Player/playerlog.txt
and blades-video.log. A missing adapter log is useful evidence too.

Up to720p H.264 is the upstream target. HEVC/H.265, VP9 and AV1 are unsupported;
a supported container extension does not guarantee every codec inside it.
This component does not add music, pictures or original Xbox visualisations.

Source contains the complete upstream tagged source and complete original
adapter source/build scripts. Keep Notices, UPSTREAM-README and ADAPTER-README
when redistributing. The player/adapter are GPLv3; Blades remains a separate
MIT title. There are no stock dashboard resources or firmware in this folder.
player.ini is an ephemeral owned autoplay request; settings.ini stores player
options locally. Logs/configuration files are not included in release ZIPs.
