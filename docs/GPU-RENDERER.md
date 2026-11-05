# Blades 0.15 GPU renderer

The supplied Xbox 360 log measured eight windows of v0.14 at 7.5–8.7 FPS
(mean 7.89). Each frame averaged 50.54 ms in the swirl, 63.14 ms in other
software rendering, and 12.04 ms uploading the framebuffer. Display waits
averaged 1 microsecond. Those are actual console observations. There is no
physical-console performance measurement for v0.15 yet.

## What changed

The Xbox now draws textured triangles through Xenos. It no longer builds,
byte-swaps and uploads a 1280x720 software framebuffer every frame.

- The supplied DashX360 swirl equations run in a pixel shader on the original
  384x216 grid. The 30 Hz sample clock, palette, period and shared time across
  blades remain. Cached source textures feed a GPU pass that retains the
  original endpoint mapping and separable integer interpolation.
- Artwork textures stay on the GPU. Static icons/logos retain the existing
  alpha-aware downsampling. A GPU box filter preserves that filtering during
  the caps' small overshoot shrink; other animated cap sizes sample on the GPU.
- Scaled glyph coverage is collected into texture atlases. The GPU draws text
  and blends its coverage and the original transparent artwork over the swirl.
- An unchanged UI retains its drawing commands, vertex buffer and textures.
  Animation time alone does not rebuild the foreground. Selection, submenu,
  dialog and blade changes update the commands. Tests cover ten idle seconds
  with no foreground rebuilds.
- Content fades save the pre-content frame in a GPU texture, draw the content,
  then blend the saved frame back by the measured fade amount. The moving
  blade draws both background colours on the same clock with the same edge.
- GPU fences protect textures and vertices before reuse or eviction. Each
  frame redraws the complete display, including after the Guide has used eDRAM.

The native viewport accounts for Xenos' whole-coordinate D3D pixel centres.
Texture uploads and render-to-texture passes use the SDK's specified RGBA/BGRA
formats; the CPU never writes bytes directly into a display surface.

## Validation and scope

The existing software renderer remains the reference and regular SDL preview.
Headless desktop GPU playback uses the same drawing plan and GLSL sources as
the Xbox backend, through EGL/OpenGL on Mesa llvmpipe. It is not Xbox execution
or a console speed benchmark.

- 69 command-list scenarios passed, including exact static menu pixels,
  clipping guards, cache invalidation, fades and the moving two-colour leaf.
- 113 desktop GPU comparisons cover every root selection/submenu, five
  palettes at six animation times, transition phases and dialogs. The largest
  RGB-channel difference was 4/255; worst frame mean absolute channel error
  was 0.4784/255. No compared pixel differed by more than eight levels. The
  retained shapes, positions and artwork are unchanged. Blending/float rounding
  means the GPU output is not promised to be byte-identical to CPU output.
- The actual compiled Xenos shader containers passed the SDK microcode
  interpreter: 240 swirl samples against the supplied equations, point-fetch
  background interpolation, glyph tint/coverage, fades and cap box filtering.
  This checks compiler output arithmetic, not the GPU's execution or timing.
- Desktop suites, ASan/UBSan checks and native XEX structural/digest checks
  passed. Hardware validation is still required for this new backend.

The new 63-second 720p walkthrough was freshly recorded with the desktop GPU
backend and the matching premade pack, including stereo navigation audio. Its
30 fps file format does not demonstrate an Xbox frame rate.

[Per-frame comparison metrics and validation metadata](data/v0.15-gpu-validation.json).

## Console log

Startup identifies `GPU 720p renderer` and the retained-quad/swirl-shader path.
Once the first GPU frame finishes, startup checks read back actual texture and
display pixels. Look for:

```text
gpu-check: PASS swirl_source samples=20 max_rgb_error=...
gpu-check: PASS final_background max_rgb_error=...
```

The final-background check applies to the initial unobstructed Games root.
These checks run only once, so cold decoding/readback affects the first window.

After about ten seconds, compare the steady `perf:` windows:

| Field | Meaning |
| --- | --- |
| `fps` | Submitted frames per second during the window |
| `cpu_us` | CPU command preparation, resource upload and GPU submission per frame |
| `gpu_us` | GPU clock interval for the previous completed render, including source updates and resolves |
| `texture_upload_us` | Resource-upload portion of `cpu_us` |
| `gpu_wait_us` | CPU wait before reusing the preceding frame's resources |
| `present_wait_us` | CPU wait for an available front buffer |
| `list_builds` | Foreground command rebuilds in the window; normally zero while idle |
| `draws` | Average scene draw calls per submitted frame |
| `bg_updates` | Swirl source evaluations in the window, including both colours during blade wipes |
| `resident_kb` | Requested asset texture storage; excludes render targets, buffers and allocation padding |
| `missed` / `late` | Display deadline counters during the window |

The fields overlap and should not all be added together as one frame time.
No 30/60 FPS claim is made before testing on a 360. Check idle motion, rapid
navigation, submenu transitions, Guide open/close, sound and confirmed exit.

The artwork/sound/font payload remains the same 213 entries; all attribution
and runtime notices remain included. This changes the application renderer,
without changing console configuration, system files or dashboard services.
