# Local DashX360 skin (Blades 0.5.0)

The optional skin imports the separated blade artwork, runners, header/footer
masks, tray masks and menu icons from the repository supplied by the user:

https://github.com/ZivvoZ/dashx360/tree/0c8adb2cef4ad7dff2bf889351efa1f1c244aef2

Credit: original project by **zivvoz / DashX360**, inspired by Microsoft's Xbox
360 Dashboard interfaces. Xbox/Microsoft own their respective artwork and
trademarks. Upstream describes its own recreation as still being improved;
using its artwork does not establish an exact match to dashboard 5766.

## Prepare on your PC

Install Python 3 and Pillow (`sudo apt install python3-pil git` on LMDE/Debian).
Download `tools/import-dashx360-skin.py` from this project, then run:

```sh
git clone https://github.com/ZivvoZ/dashx360.git dashx360
git -C dashx360 checkout 0c8adb2cef4ad7dff2bf889351efa1f1c244aef2
python3 import-dashx360-skin.py --source dashx360 --output blades.xzp
```

If you also have the matching retail 5766 `shrdres.xzp`, include it to retain
the stock gamer picture, button spheres and logos:

```sh
python3 import-dashx360-skin.py --source dashx360 --shared /path/to/shrdres.xzp --output blades.xzp
```

Use `python3 tools/import-dashx360-skin.py` when running from the source tree.
The shared archive is optional. Missing images use embedded fallbacks.

Copy both files to the console:

```text
Hdd1:\Blades\default.xex           # version 0.5.0 or newer
Hdd1:\Blades\Assets\blades.xzp    # generated on your PC
```

The new executable alone retains the embedded appearance. It needs the local
skin pack to display the imported blade artwork. The application tries
`Assets/blades.xzp` first, then `Assets/shrdres.xzp` if the first archive cannot
be opened. An explicit `BLADES_RESOURCE_PACK` overrides both on the desktop.

No upstream images, firmware, fonts or sound recordings are embedded in the
public XEX or included in the public repository. The conversion script reads
the user's checkout and writes a local pack; it does not download or publish
artwork. The public script is original project code. The upstream repository
has no root formal licence, and its README explicitly retains third-party
ownership of original Xbox assets.

## Conversion and limits

The converter scales the 1920x1080 artwork to the renderer's 1280x720 canvas,
crops transparent margins and tints the primary-colour masks to each blade's
palette. It samples the supplied WPF body geometries into scan-line bounds;
the Xbox application runs no WPF or upstream C# code. Five states contain
twelve sprites each; eighteen additional icons and a big-endian `BSK1`
layout manifest are stored in a bounded XUIZ v1 container. With the inspected
116-entry shared archive, the resulting pack has 195 entries.

Runtime source PNG dimensions are limited to 1024, archive size to 16 MiB and
entry count to 512. Layout dimensions, rectangles and row bounds are checked
before drawing. Decoded images and scaled variants are cached. Reload/close
invalidates cached layout pointers. Malformed or missing data falls back to
the procedural renderer. Memory use, first-use decode latency and frame rate
still need physical-console testing.

## What remains approximate

Menu text uses Open Sans Regular. No TTF/OTF/WOFF font is bundled in the
inspected DashX360 tree. The newly supplied `Segoe Xbox Symbol` font (version
1.05 build 147) has 413 private-use characters and no printable Latin letters;
it is an icon font and cannot replace the dashboard text atlas. It has not
been embedded or used. The earlier font upload that the user withdrew was
ignored.

The procedural backgrounds, content layout, gamer-card data and animation
timing remain reconstructed. Upstream WAV sounds were located but are not
used by this version; navigation still uses synthesised ticks. Exact font,
background, sound and layout matching remain outstanding.
