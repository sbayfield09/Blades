# Local DashX360 skin (Blades 0.19.0)

The optional skin imports the separated blade artwork, runners, header/footer
masks, tray masks and menu icons from the repository supplied by the user:

https://github.com/ZivvoZ/dashx360/tree/0c8adb2cef4ad7dff2bf889351efa1f1c244aef2

Credit: original project by **zivvoz / DashX360**, inspired by Microsoft's Xbox
360 Dashboard interfaces. Xbox/Microsoft own their respective artwork and
trademarks. Upstream describes its own recreation as still being improved;
using its artwork does not establish an exact match to dashboard 5766.

## Download the premade pack

The public repository now includes both release files:

- [Compiled XEX](../dist/Blades/default.xex)
- [Premade artwork ZIP](../dist/Blades-artwork-pack.zip)

Download both and extract the artwork ZIP into `Hdd1:\Blades\`, placing the
artwork at `Hdd1:\Blades\Assets\blades.xzp`. Put `default.xex` alongside the
`Assets` folder. No conversion is needed. Every future release will include
both files, as requested by the user.

## Optional: rebuild the pack on your PC

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
Hdd1:\Blades\default.xex           # version 0.17.0
Hdd1:\Blades\Assets\blades.xzp    # premade pack download
```

The new executable alone retains the embedded appearance. It needs the local
skin pack to display the imported blade artwork. The application tries
`Assets/blades.xzp` first, then `Assets/shrdres.xzp` if the first archive cannot
be opened. An explicit `BLADES_RESOURCE_PACK` overrides both on the desktop.

The public XEX embeds no upstream media. At the user's request, a separate
premade artwork ZIP is provided alongside it, including source attribution and
installation instructions. This ZIP contains the converted blade artwork and
shared images described here; it includes no dashboard firmware, raw font containers. The conversion script reads a local checkout and does not download
or publish artwork itself. The upstream repository has no root formal licence,
and its README explicitly retains third-party ownership of original Xbox
assets. Ownership is unchanged by this conversion.

## Conversion and limits

The converter scales the 1920x1080 artwork to the renderer's 1280x720 canvas,
crops transparent margins and tints the primary-colour masks to each blade's
palette. Version 0.6.0 derives root and expanded surface bounds from the
actual blade/runner alpha silhouettes, covering the active tab and curved
edges beneath the chrome. The Xbox application runs no WPF or upstream C# code. Five states contain
twelve sprites each; eighteen additional icons and a big-endian
`BSK2` layout manifest are stored in a bounded XUIZ v1 container. The runtime
also accepts the earlier `BSK1` layout format. With the inspected
116-entry shared archive, the resulting pack has 195 entries.

Runtime source PNG dimensions are limited to 1024, archive size to 16 MiB and
entry count to 512. Layout dimensions, rectangles and row bounds are checked
before drawing. Decoded images and scaled variants are cached. Reload/close
invalidates cached layout pointers. Malformed or missing data falls back to
the procedural renderer. Memory use, first-use decode latency and frame rate
still need physical-console testing.

## What remains approximate

Version 0.8.0 introduced a glyph atlas recovered from the user's Convection font
resources when the matching pack is present. The XEX retains Open Sans as a
fallback. See [CONVECTION-FONT.md](CONVECTION-FONT.md) for verified source and
loading details. Segoe Xbox Symbol was not used as a menu font.

The background now reuses DashX360's animated implementation; see
[ANIMATED-BACKGROUND.md](ANIMATED-BACKGROUND.md). Content layout, gamer-card
data and navigation timing retain their existing limitations. Version 0.10 imports six transition WAVs; short focus/select/dialog ticks remain synthesised. Exact font,
background, sound and layout matching remain outstanding.

Version 0.6.0 preserves icon canvas padding and aspect ratios, reduces frame opacity, and corrects card/menu/tray/footer anchors. See [MENU-ALIGNMENT.md](MENU-ALIGNMENT.md) for the comparison. Use the matching XEX and premade artwork pack together.

Version 0.7.0 retints the frame/tray art to DashX360's blade palette for the continuous background animation. BSK2 geometry and the 195-entry resource set remain identical. No background video file is required.

The 0.9 pack has 206 entries: all previous 198 resources unchanged, plus eight enabled/disabled ABXY prompt sprites. The earlier 0.8 pack added the font atlas, metrics and notice to the original 195 resources. Font containers and raw TTF files are not distributed.

Version 0.9.0 gives the button letters a compact, heavier treatment, centres their visible ink and softens disabled prompts. See [BUTTON-PROMPTS.md](BUTTON-PROMPTS.md).

Version 0.10.0 adds six complete stereo transition recordings and an audio notice, bringing the pack to 213 entries; all previous 206 entries are unchanged. See [TRANSITION-AUDIO.md](TRANSITION-AUDIO.md) for reference matching and playback details. To rebuild with sounds, add `--audio dashx360/Assets/Audio/Sounds` to the conversion command; this optional step requires FFmpeg. The premade ZIP already contains the converted sounds.

Version 0.11 corrects transition timing and cap/rail motion against the supplied recording. Its 213-entry pack payload is byte-identical to 0.10; updated install notes accompany it. See [TRANSITION-AUDIT.md](TRANSITION-AUDIT.md).

Version 0.12 corrects the selected-menu shadow and its divider join. The 213-entry payload remains unchanged; see [MENU-SHADOW.md](MENU-SHADOW.md).

Version 0.13 adds the reference-based fade toward the selected row's lower-left corner. The resource payload remains 213 entries; see [MENU-SHADOW.md](MENU-SHADOW.md).

Version 0.14 optimizes the 384x216 swirl's 720p filtering with cached rows and Xbox VMX operations, retaining the 213-entry pack payload. See [BACKGROUND-PERFORMANCE.md](BACKGROUND-PERFORMANCE.md).

Version 0.15 uses GPU swirl generation, scaling, artwork/text blending and retained menu commands. The 213-entry payload remains unchanged. See [GPU-RENDERER.md](GPU-RENDERER.md).

Version 0.16 adds branding/live-panel.png, derived from the repository's grey rounded panel/ring styling and the supplied reference geometry, including the subtle outer shadow. The 213 previous payloads are unchanged; total 214 entries. See [BRANDING-AUDIO-REVIEW.md](BRANDING-AUDIO-REVIEW.md).

Version 0.17 adds the two original curved startup shutters, their layout and the radial green reveal derived from the supplied repository. All 214 previous payloads are unchanged; total 218 entries. Startup finishes at the main Games root. See [STARTUP-ANIMATION.md](STARTUP-ANIMATION.md).

Version 0.18 adds sixteen transparent disc-reading frames adapted from the pinned repository's BladesLoadingIndicator.cs, with its console silhouette and orbiting dots. The premade pack has 234 entries, retaining all 218 previous payloads byte for byte. See [DISC-READING.md](DISC-READING.md) for source credit, native behavior and hardware validation limits.

Version 0.19 retains the complete 234-entry artwork payload unchanged. Its XEX corrects pending/no-media classification and nonzero title-ID returns.

Version 0.20 reuses the compatible 234-entry pack unchanged. The library and
folder picker use the shared expanded-menu chrome and existing Convection atlas.

Version 0.21 uses the same 234-entry artwork payload; scan progress and depth
controls use existing menu and font resources.

Version 0.22 reuses the same 234-entry payload for the file browser, scan
controls and retained completion panel. No new artwork conversion is needed.

Version 0.23 keeps the compatible 234-entry payload unchanged while correcting
the media-player launch path in the executable.

Version 0.24 reuses the same 234-entry payload for the native profile card and
adds no account data to the pack. The signed-in gamer picture is read from XAM
into RAM; a neutral existing tile remains the fallback. The combined USB ZIP
and separate premade pack retain these notices and source attribution.

Version 0.25 uses the existing retail ico_64x_DefaultPicture.png for the
signed-out card and GScore.png for the Gamerscore label. Neither is redrawn or
replaced with a font glyph. The same 234-entry payload and notices are included.

Version 0.26 renders the original GScore.png at its 32 px canvas size, aligning
its visible centre to the surrounding Convection text as in video-games.jpg.
The asset bytes and 234-entry payload are unchanged.

Version 0.27 uses the existing secretAchievement.png, unearnedAchievement.png,
controller/trophy, scroll.png and GScore.png resources for the achievement
browser and fallbacks. Real per-game images are queried from XAM at runtime,
kept in RAM and excluded from the release pack. The compatible payload remains
234 entries with its original SHA-256; no new game images are redistributed.

Version 0.28 keeps the identical 234-entry payload. The thin achievement game
strip separators are translucent renderer lines measured against the supplied
photo and video near 05:20. Native Guide achievement views use the system UI;
the former custom gray panels are removed. No per-user pictures are packaged.

Version 0.36 has 235 entries:25 derived cap/tray masks are colour-refined and
a 202x130 shadow is generated from the unchanged retail5766 splash_360.png
alpha. Original logo, font, audio and layout bytes are retained. The shadow
uses the existing image cache. See COLOUR-AND-BRANDING.md and the final audit.
