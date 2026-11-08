# GitHub Releases and updates

System > Update Blades opens the updater and checks the latest stable GitHub
Release. A selects Check for Updates or Download Update. X cancels a check or
download; B leaves and cancels outstanding work. Network and file I/O run on a
worker. No check runs automatically at boot.

The installed version and latest available version appear on the right, along
with progress, status and the saved ZIP path. Only newer numeric versions can
be downloaded; prerelease selection is delegated to GitHub's latest stable
Release route. The target repository is sbayfield09/Blades-reimagined.

The release must include update.json. It contains schema1, version, asset,
size and SHA256. The filename must match the numeric version and the expected
Blades-v<version>-USB.zip pattern. GitHub serves the manifest at:
https://github.com/sbayfield09/Blades-reimagined/releases/latest/download/update.json
The ZIP is fetched from that version's Release, with bounded HTTPS redirects
to GitHub's asset CDN. No credentials are needed for public downloads.

HTTPS uses the included Mbed TLS3.6.5 source, public Mozilla CA roots, hostname
and chain verification, a15second inactivity limit and a10minute request bound.
Native entropy comes from XeCryptRandom; certificate validity checks use the
console date/time. No TLS verification bypass exists. The updater's transport
is independent of Jellyfin's HTTP/player transport. Proxy/test-CA/test-origin
overrides exist only in desktop builds, never in the Xbox executable.

The USB ZIP is streamed to game:/Updates/<filename>.tmp, flushed, checked for
exact length and SHA256, then renamed to the completed filename. Downloads are
limited to64MiB and metadata16KiB. Failed/cancelled partial files are removed;
existing completed files are preserved. There is no whole-file RAM buffer.
A pre-existing final filename is reported without overwriting it.

This button downloads a ZIP. It does not replace the running XEX, install
files, restart the console, modify NAND, install plugins or change the default
dashboard. Exit Blades before extracting the completed ZIP over its folder.
Existing Config files remain outside release payloads.

Maintainers: tools/publish-release.py verifies package CRCs/current XEX,
generates update.json and SHA256SUMS, uploads assets to a draft Release, checks
uploaded sizes/digests and downloads the ZIP/manifest for verification before
publishing it as latest. Published assets are never overwritten. Use:
  python3 tools/publish-release.py --version 0.50 --commit FULL_SOURCE_COMMIT
Use --prepare-only to create a reviewable release plan without contacting GitHub.
New USB/XEX/artwork/source/video downloads belong in Releases. Historical dist
files remain for provenance; new source files are browsable on main.

Tests use disposable TLS certificates and an actual HTTPS server. They cover
trusted download, current/older versions, invalid metadata, checksum mismatch,
size overflow, truncation, cancellation/temporary cleanup, allowed redirects,
redirect loops, HTTP downgrade, untrusted certificates and wrong hostnames.
A live verified HTTPS request to GitHub checks the normal proxy/trust route.
Console transport, clock configuration and storage behavior need hardware retest.
