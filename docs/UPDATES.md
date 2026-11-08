# Repository-hosted updates

System > Update Blades opens the updater and checks the repository's published
manifest. A selects Check for Updates or Download Update. X cancels; B leaves
and cancels outstanding work. Network/file I/O run on a worker. No automatic
check runs at boot. The right panel shows installed/available versions, status,
progress and the completed ZIP path.

The manifest is served directly from the normal downloads folder on main:
https://raw.githubusercontent.com/sbayfield09/Blades-reimagined/main/dist/update.json
It contains schema1, version, asset, size and SHA256. Only a newer numeric version
can be downloaded. The asset must match Blades-v<version>-USB.zip and is fetched
from that same main/dist directory. No GitHub Release, API endpoint, account or
token is needed. The USB ZIP, manifest and matching source/artwork/video must
be committed together. Keep published versioned filenames immutable; increment
the version for subsequent releases. GitHub's raw CDN may cache metadata briefly.

HTTPS uses included Mbed TLS3.6.5, public Mozilla CA roots, hostname/chain checks,
native XeCryptRandom, a15second inactivity limit and a10minute per-connection
bound. Requests ask caches to revalidate. The transport is independent of the
video/Jellyfin HTTP path. Native code excludes desktop proxy, extra-test-CA and
test-origin overrides. Console date/time must be correct for certificates.

Metadata is limited to16KiB and USB ZIPs to64MiB. Downloads stream to
 game:/Updates/<filename>.tmp
then flush, verify exact size/SHA256 and rename to the completed filename.
Failed/cancelled partial files are removed. Existing completed filenames and
private Config files are preserved. No whole-file buffering is used.

The updater downloads only. Exit Blades before extracting the ZIP over your
installation. It does not install files, replace the running XEX, restart,
modify NAND, install plugins or change the default dashboard. Install the first
repository-based updater build manually once; future versions use this manifest.

Maintainers: build/test the XEX, generate matching video/source/artwork/USB ZIP,
then run:
  python3 tools/prepare-update.py --version 0.50
This verifies package CRCs/current XEX and writes dist/update.json, SHA256SUMS
and a reviewable repository-update-plan.json. Publish all matching public source,
versioned artifacts and manifest together in main. Use the explicit safe USB
payload for any unpacked runtime files; never stage user configs/logs. GitHub
Releases can be optional mirrors, but the updater reads the repository folder.
The older tools/publish-release.py is an optional mirror publisher, not the
updater's required publication path.

Actual TLS integration tests use the same /sbayfield09/Blades-reimagined/main/dist/
paths as production. They cover Content-Length/chunked downloads, current/older
versions, invalid metadata, checksum/length failures, truncation, cancellation,
partial cleanup, allowed redirects/loops, unsafe hosts, HTTP downgrade, untrusted
chains and wrong hostnames. The opt-in older-client live probe checks and downloads
the actual GitHub manifest/ZIP after publication, including exact hash/size.
Console networking/storage still need hardware retest.
