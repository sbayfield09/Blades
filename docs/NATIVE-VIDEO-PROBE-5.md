# Native video diagnostic 5

Diagnostic4/default.log records XexLoadImage=C000007B (invalid image format),
a null handle, STOP before native handoff and exit1. The user reports Aurora.
That is the diagnostic's normal failure exit, not evidence that Aurora
intercepted a successful native player launch. The control log records
videolibrary with flags80000004 returning00000000. No Lua bridge code ran
in the failed selected test. Uploaded logs/device paths remain private.

A concrete DLL-format mistake: build_lex.py changed IMAGE_BASE to9B000000 but
kept the xsdk title writer's65536-byte page geometry and clear4KB flag. The
90000000..9FFFFFFF XEX address range uses4096-byte pages, as documented by the
Xenia memory map (memory.cc) and seen in the supplied stock .lex image flags.
The old DLL's3 descriptors cover12288 bytes in that range, before its entry at
base+65536 and Lua resource atbase+131072. A valid hash chain under the writer's
incorrect geometry did not establish a valid loadable image.

The correction changes the resource DLL writer to4096-byte pages and sets
IMAGE_PAGE_SIZE_4KB (10000000) in the security image flags. All33 page descriptors
and their digest chain are regenerated; the declared image size is135168 bytes.
The preferred base, entry/resource addresses, module type9, entry stub, imports,
Lua package, native URI flags and selected WMV remain unchanged. Only launcher
name/banner/version identify diagnostic5 (XEX0.0.5.0, titleID424C5052).
No native binary patches, console settings or installed dashboard changes.

verify_xex.py now independently selects expected page size from the address
range, checks it against the image flag, rejects heap crossings, and verifies
the descriptor count and page digests using that size. Default title checks
continue to validate the public Blades0.37 XEX. test_lex.py checks actual entry
and complete Lua-resource coverage, rejects diagnostic4's wrong geometry,
rejects flag/image mutations, and verifies the unchanged embedded package.
Lua extracted from the final DLL is round-tripped and executed with the real
Lua5.1 host VM plus mocked Xbox services; this is not native playback testing.

The launcher still loads/checks/unloads our DLL before native handoff and logs
its statuses. Its kernel load result is the next hardware evidence needed.
Extract the complete NativeVideoProbe-5-USB.zip, keeping default.xex,
Assets/native-video.lex and selected.wmv together. Run default.xex and return
its default.log plus the visible outcome. No repeat of control.xex is required.
If checkpoint/error text remains, include a photo. A successful preflight or
native HRESULT0 alone does not prove that the video plays.

This is a separate console diagnostic. The regular Blades0.37 XEX, artwork,
walkthrough video and USB ZIP remain unchanged. There is no Blades renderer
or navigation video in this isolated launcher. Stock binaries/UI resources
are neither copied into the generated module nor distributed.
