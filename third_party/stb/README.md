# stb_image

Upstream: https://github.com/nothings/stb
Pinned revision: d14d46f6b44456eb88beac58cba833a4135d4c89
stb_image.h SHA-256: 594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3
Licence: dual MIT/public domain, reproduced in LICENSE and the header.

Configured in src/core/png.c for PNG only, memory input, no SIMD/HDR/linear formats,
maximum image dimension 1024. The third-party header's PNG-only unused helper
warnings are scoped out; project source warnings remain errors.
