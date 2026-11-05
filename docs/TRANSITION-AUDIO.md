# Blades 0.10 transition audio

The reference recording contains a short whoosh when changing blades and a
longer sound when opening or closing an expanded menu. This release uses the
existing recordings from the user-supplied DashX360 repository, rather than
synthesising replacements.

## Reference check

The supplied recording was inspected around the Games-to-Media transition
at 05:06.8 and nearby submenu changes. Normalised waveform correlation at
16 kHz mono found these full-asset alignments in the recording's audio:

| Asset | Audio alignment | Correlation |
| --- | --- | --- |
| `blades-switch-games-to-media.wav` | 05:07.129 | 0.911 |
| `blades-menu-open.wav` | 05:12.900 | 0.994 |
| `dash_2ndLevelClose.wav` | 04:59.860 | 0.942 |

AAC compression and capture differences affect these comparisons. These are
signal matches, not proof that every upstream file is a byte-identical retail
asset. The clip alignment timestamps are separate from the visible start of
an animation in the supplied capture. The implementation starts sounds with
the accepted navigation transition, without reproducing capture A/V delay.

## Asset mapping

Source: [ZivvoZ/dashx360](https://github.com/ZivvoZ/dashx360/tree/0c8adb2cef4ad7dff2bf889351efa1f1c244aef2/Assets/Audio/Sounds).

| Transition | Source WAV | Duration |
| --- | --- | --- |
| Marketplace ↔ Xbox Live | `blades-switch-marketplace-to-live.wav` | 0.309 s |
| Xbox Live ↔ Games | `blades-switch-live-to-games.wav` | 0.309 s |
| Games ↔ Media | `blades-switch-games-to-media.wav` | 0.309 s |
| Media ↔ System | `blades-switch-media-to-system.wav` | 0.309 s |
| Open expanded menu | `blades-menu-open.wav` | 0.465 s |
| Return from expanded menu | `dash_2ndLevelClose.wav` | 0.501 s |

Each blade boundary uses its recording in either direction. The upstream
Games-to-Media and Media-to-Games files are byte-identical. At the initial v0.10 audit, the other three boundary recordings were selected by their supplied names. The v0.16 review also matches Live/Games and Media/System in a rapid left-to-right reference sequence; Marketplace/Live remains filename-mapped. See [BRANDING-AUDIO-REVIEW.md](BRANDING-AUDIO-REVIEW.md).

The importer preserves stereo, converts to PCM16 at 48 kHz and includes all
six complete clips in the premade pack. It applies no trims or time stretching.
The mixer plays them at 70% amplitude, supports up to eight overlapping voices
and clamps the combined output. Existing short focus/select/dialog ticks remain.

## Playback and checks

The shared navigation code emits a sound event for an actual state change.
Blocked blade edges, idle frames and ignored inputs do not emit a whoosh.
SDL uses a continuous audio callback; the Xbox build uses a dedicated feeder
thread so the longer sounds do not block animation or stop after 32 ms.
The offline walkthrough runs the same stereo mixer at 48 kHz.

The bounded WAV reader accepts PCM16 stereo at 48 kHz, rejects malformed
chunks and owns its sample memory independently of resource-pack lifetime.
Missing or invalid sound assets retain quiet tick feedback. Audio queue and
voice counts are bounded. The feeder stops and joins before releasing samples.

Seven desktop suites passed, including transition-event mapping, full-length
stereo playback, overlap/clamping, invalid WAV input, pack closure and queue
bounds. Every sample of all six converted clips was checked against mixer
output. AddressSanitizer/UndefinedBehaviorSanitizer checks passed. The XEX
passed structural and digest validation; hardware playback and timing still
need physical-console verification.

The pack contains 213 entries: all 206 previous resources unchanged, six WAVs
and an audio attribution notice. The XEX has no embedded upstream recordings.

Credit: original project by zivvoz / DashX360, inspired by Microsoft's Xbox
360 Dashboard interfaces. Xbox audio and other supplied assets remain owned
by Microsoft and their respective rights holders.

Version 0.16 verifies the existing rising sequence across the four boundaries. The recordings and mixer are unchanged. The new walkthrough includes complete sweeps in both directions.
