Blades 0.6.0 - five-blade 5766-style test build

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
Check default.log for a loaded local XUIZ archive message (195 entries with
both sources). Fonts, backgrounds, layout effects and audio remain approximate.
