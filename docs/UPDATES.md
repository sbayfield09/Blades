# Automatic updates and restart

In0.52 and later, System > Update Blades checks the repository manifest. Select
Update and Restart to download, verify, install and relaunch Blades automatically.
The app remains responsive while downloading/preparing; X or leaving the page
cancels before handoff. Once the installer starts, keep the storage connected.
There is no extraction prompt or second confirmation. Restart follows successful
installation; network and disk work take time.

Versions0.50/0.51 only download. Install the complete0.52 USB package manually
once to gain automatic installation. Keep update-installer.xex beside default.xex;
copying just default.xex is insufficient. No DashLaunch/plugin is required.
Settings, saved scan paths, Jellyfin sessions, FTP credentials, logs, existing
Downloads/Updates and unrelated files are retained. No NAND, firmware, launch.ini
or default-dashboard changes are made. Desktop preview downloads only and never
replaces its executable; automatic native relaunch is Xbox-only.

The worker streams the ZIP into Updates, checks exact SHA256/length, then
extracts into the private .blades-update/stage folder. ZIP paths are restricted
to known app files, Assets and licences; private Config paths, traversal,
absolute/drive paths, case duplicates, symlinks, encryption, ZIP64, unsupported
methods, malformed directories and CRC failures are rejected. Config/README.txt
is the only permitted Config entry. Bounds:64 files,32MiB per file,128MiB total
expanded bytes; ZIPs remain limited to64MiB. Entries use stored or raw DEFLATE
compression via the existing pinned stb decoder; no extra download is needed.

After verified staging, Blades launches a copy of the currently installed helper
from .blades-update/installer.xex. Normal atexit closes Blades services, resource
handles, audio and renderer before the helper starts. The helper derives the app
folder from its own image path and creates a local drive alias. It accepts no
arbitrary destination from the manifest. All installed files are backed up and
hashed before a persistent pending journal is written. Files are replaced by
same-volume rename; default.xex is replaced last and remains present throughout.
The running helper is a separate copy, so replacing its root installation is safe.
The journal is removed only after every replacement verifies. The helper then
launches the installed default.xex through the SDK/XAM launch path.

A failed file commit restores the old files from verified backups. An interrupted
transaction is recovered by the helper before the next normal startup loads
artwork. Restore copies retain the originals so recovery can be repeated after
another interruption. An unreadable/damaged backup leaves the journal intact
and stops on a recovery message rather than continuing an incomplete install.
Backups stay in .blades-update/backup until the next update preparation. They
can be copied back manually if the device needs repair. Filesystem/hardware
power-loss behavior and actual XAM handoff still need physical-console testing;
process-interruption fixtures do not prove drive-cache durability or recovery
from an unbootable future build.

The manifest and ZIP remain in the repository downloads folder:
https://raw.githubusercontent.com/sbayfield09/Blades-reimagined/main/dist/update.json
Schema1 includes version, asset, size and SHA256. A newer numeric version is
required; asset must match Blades-v<version>-USB.zip in that same main/dist folder.
No GitHub API, Release, token or account is needed. TLS verifies chain/hostname
using bundled Mbed TLS3.6.5/public CA roots and native XeCryptRandom. Console time
must be correct. Metadata is limited to16KiB; redirects are bounded/allowlisted,
with15second inactivity and10minute connection limits. A matching cached ZIP is
rehashed and reused; a damaged cache is replaced only after a verified download.
Failed/cancelled temporary files are discarded. No boot-time update check runs.

Maintainers: build/test both XEXs and package matching source/artwork/video/USB,
then run tools/prepare-update.py --version 0.52. Publish every matching artifact
and dist/update.json together on main; keep versioned files immutable. The source
bundle includes the helper, ZIP/transaction modules and build tools. Tests cover
real TLS download-to-install, cache reuse/repair, invalid verified packages,
Config preservation, commit failures, process interruptions during install and
recovery, damaged backup retry, unsafe ZIPs and deterministic parser mutations.
