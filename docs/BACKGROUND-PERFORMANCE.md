# Blades 0.14 background performance

Version 0.15 replaces the Xbox software path with a GPU compositor after the
user's v0.14 console log measured 7.5–8.7 FPS. See [GPU-RENDERER.md](GPU-RENDERER.md)
for that change, its checks and the new log fields. The desktop timings below
remain historical v0.14 measurements.

The swirl performed its 720p filtering in a scalar CPU loop on every draw.
Profiling showed this dominated the background cost, even when its animation
sample had not changed. Version 0.14 keeps the supplied DashX360 deformation
equations, palette, 384x216 field and 30 Hz sampling, and changes the filtering
work that feeds the 720p canvas.

Source rows are expanded horizontally once per field/tint update and cached.
The vertical pass processes four pixels at a time using Xenon VMX operations;
the desktop uses SSE2, with a scalar fallback for other hosts. Equal-colour
source runs and unchanged field samples reuse work. The Xbox path uses vector
floating-point operations whose dyadic inputs preserve exact byte interpolation.

The foreground image path also skips transparent pixels, copies opaque pixels
directly and computes scaled image coordinates once per column/row. This
removes repeated 64-bit divisions inside the pixel loop. Background elapsed
time follows the actual frame duration, including frames longer than 100 ms.

## Measured desktop CPU cost

These are **Linux desktop process-CPU timings, not Xbox frame rates**. Each case
renders 180 frames with the matching 213-entry pack; values are medians of three
interleaved runs. Cold decoding, SDL presentation and Xbox upload cost are not
included. The Xbox uses a different SIMD implementation, so these percentages
must not be read as measured console speedups.

| Case | v0.13 ms/frame | v0.14 ms/frame | CPU reduction |
| --- | ---: | ---: | ---: |
| Held background / filtering only | 2.495 | 0.268 | 89.3% |
| Background with 30 Hz updates | 3.032 | 1.648 | 45.6% |
| Background at 60 draws/s, 30 Hz updates | 2.740 | 0.940 | 65.7% |
| Warm Games root renderer | 7.426 | 6.018 | 19.0% |

[Raw runs and methodology metadata](data/v0.14-background-benchmarks.json).

## Visual and arithmetic checks

Thirty complete backgrounds across all five palettes and six times were
compared with the old implementation: the largest RGB-channel difference was
one level; 3,357 of 82,944,000 channels changed (about 0.004%). That difference
comes from the order of separable interpolation and its integer rounding.
All 48 root/submenu snapshots also remained within one colour level.

The per-byte row filter was checked for every weight from 0 through 256,
random RGBA inputs, aligned/unaligned rows, tail widths and output guards.
The desktop SIMD path and PowerPC VMX arithmetic passed. The PowerPC checks ran
under QEMU's classic VMX model with a test-only adaptation of XSDK's 64-bit GPR
stack saves for a 32-bit harness. Production Xbox objects were not adapted.
This checks arithmetic, not Xbox execution or CPU-cycle timing.

Nine desktop suites, actual-pack renderer bounds checks and ASan/UBSan checks
passed. The native XEX passed header/image/import digest validation and verifier
mutation checks. The instruction scan reports no skipped instructions in the
new row filter. Physical-console performance remains to be measured.

## Console timing log

The XEX writes a startup line identifying the `VMX` row filter and a `perf:`
summary every five seconds to `default.log` beside the executable. This is an
application log; no dashboard or console configuration is changed.

- `fps`: average submitted frame rate over that window.
- `render_us`: average software rendering time per frame.
- `bg_us`: background rendering time per frame, included in `render_us`.
- `upload_us`: byte conversion, framebuffer upload and submission time.
- `wait_us`: time waiting for a presentation buffer.
- `missed` / `late`: presentation deadline counters for the window.
- `bg_draws` / `bg_updates`: background draw calls and field evaluations.

The first window includes cold asset decoding; steady idle measurements after
about ten seconds distinguish that from sustained swirl cost. These logs make
remaining console bottlenecks measurable rather than inferred from a desktop
walkthrough. The new video is a fresh 63-second 720p desktop capture and does
not demonstrate a measured 360 frame rate.

The premade artwork/sound payload remains 213 entries. Existing DashX360,
Microsoft/font and runtime attribution remains included.
