# Blades0.42 downloads

[Complete USB ZIP](../dist/Blades-v0.42-USB.zip) — extract onto USB and launch
Blades/default.xex. Matching assets/notices are included; saved settings/logs
are excluded, preserving existing scan paths and FTP configuration.

[XEX](../dist/Blades/default.xex) · [Artwork](../dist/Blades-artwork-pack.zip) ·
[Desktop walkthrough](../dist/Blades-v0.42-walkthrough.mp4) ·
[Source/relink bundle](../dist/Blades-v0.42-source.zip)

The supplied0.41 log showed H264 decoding around29–33fps in several intervals,
but presentation sometimes0fps. Audio started during initial video buffering;
the early late-frame rule then discarded every new picture behind that clock.

Version0.42 starts normal audio when the first picture is handed to the renderer
and removes that discard-all-late rule. The renderer keeps the newest due frame
and coalesces older queued ones. An intentional audio-only lead can still free
bounded demux queues and reach its first video packet. lag_ms, late_presented
and audio_preroll improve diagnosis.

The compatible decoder and working three-plane GPU conversion/scaling from0.41
are unchanged. No new CPU SIMD, frame-skipping codec mode or quality reduction
is enabled. This is a timing fix, not a claim of faster codec calculations.

Controlled startup/slow-decoder tests fail on0.41 and pass with the fix. Host,
GPU/Xenos shader, sanitizer and native XEX checks pass; the source rebuild matches
the binary. New native smoothness still needs a console test. Retry the same long
file and send default.log after normal playback if it still lags.
