Blades 0.15.0 - five-blade 5766-style test build

Compilation and structural XEX validation passed. Console execution and
Peer Pressure/kernel 17559 compatibility have not been verified.
System-service entries use demo data or unsupported-feature dialogs.

Copy this complete Blades directory to Hdd1:\Blades\ and launch default.xex
from Aurora's file browser. Keep your existing dashboard setup unchanged.

Controls: D-pad/left stick navigate, LB/RB change blades, A select, B back.
Back opens an exit menu; choose Return to system dashboard to confirm.
The destination is the console's configured system dashboard, not a guessed
Aurora path. Do not configure Blades as the default dashboard yet.

Test: image/colours, all five blades, submenu select/back, animations,
controller reconnect, selection sounds, Guide open/close and confirmed exit.
Listen for complete stereo whooshes on each blade change and submenu open/back,
including rapid navigation. Animation should continue while each sound plays.
Check that the background visibly deforms while idle and continues moving
through blade and submenu changes. This version reuses DashX360's animation
implementation; its source/attribution is in licenses\DASHX360-NOTICE.txt.
The matching 0.15 artwork pack also includes glyphs recovered from the supplied
Convection font. Check text appearance, spacing, button-letter centring and softer disabled prompts.
No console system-font replacement or flash modification is needed.

Read default.log beside default.xex after a failure. Provide the log and a
console screenshot; the log records completed boot stages and API failures.
Optional network logging: net_log.txt beside default.xex containing PC_IP 7371,
with xsdk's tools/net_log.py running on the PC.

No NAND, kernel, XAM, exploit, Aurora, saved-game or profile modifications.
No Microsoft firmware assets or proprietary XDK components are included.
Licences and SHA256SUMS are included in this directory.

Blade artwork: download the matching premade Blades-artwork-pack.zip and
extract it here so Assets\blades.xzp is present. No conversion is needed.
See docs/DASHX360-ASSETS.md. Credit: zivvoz / DashX360, inspired by Microsoft Xbox.
The XEX alone retains embedded artwork. A matching retail 5766 shrdres.xzp
is also supported; the converter can merge it into the local skin pack.
Check default.log for a loaded local XUIZ archive message (213 entries with
the matching premade pack). Fonts, backgrounds, layout effects and audio remain approximate.

Version 0.11: compare the cap fold/reopen/overshoot and slower submenu content
fade. Blade transitions finish in about 0.55 s; submenus in about 0.94 s.
Reference timing was tested with the shared renderer; console frame pacing
has not been measured. See docs/TRANSITION-AUDIT.md.
Check that inactive tab labels clear early during submenu opening and return
late while closing.

Version 0.12: check the softer shadow below the selected row, rounded right
join and fading right edge. Move among first, middle and last rows on each
blade and in Console Settings. Xbox Live keeps a flatter root-list shadow.

Version 0.13: the broad underside shadow should fade toward the selected row's
lower-left corner while the rounded right-hand shadow remains stronger.

Version 0.15 draws the swirl, artwork, text and transparency on the GPU.
The 213-entry artwork/sound/font payload is unchanged. Startup should identify
GPU 720p rendering, followed by gpu-check: PASS swirl_source and, on the
initial Games root, gpu-check: PASS final_background. These read actual GPU
pixels once. Any FAIL is useful diagnostic evidence; keep the complete log.

After ten seconds, default.log has perf: summaries every five seconds:
fps, cpu_us, gpu_us, texture_upload_us, gpu_wait_us, present_wait_us, missed,
late, list_builds, draws, bg_updates and resident_kb. Idle list_builds should
normally be zero; background updates continue independently. GPU and CPU times
overlap, and texture_upload_us is included in cpu_us. See GPU-RENDERER.md.

The previous 0.14 log measured 7.5-8.7 FPS on the user's 360. Performance and
appearance of this new GPU backend still need testing on physical hardware.
Check idle motion, rapid navigation, all transitions, Guide open/close, audio
and confirmed exit. The fresh release video is desktop GPU playback.
