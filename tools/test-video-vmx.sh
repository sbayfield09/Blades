#!/bin/bash
set -euo pipefail
: "${XSDK_TOOLCHAIN:?Set XSDK_TOOLCHAIN}"
project=$(cd "$(dirname "$0")/.." && pwd)
build=${1:?FFmpeg build directory};qemu=${2:-qemu-ppc};output="$project/build/video-vmx-check"
mkdir -p "$output"
compiler="$XSDK_TOOLCHAIN/bin/powerpc-xsdk-elf-gcc"
flags=(-mcpu=7400 -mno-powerpc64 -maltivec -O3 -fno-pic -msdata=none -ffreestanding -fno-builtin -I"$build/objects" -I"$build/source/FFmpeg-n2.8.22")
"$compiler" "${flags[@]}" -S "$build/source/FFmpeg-n2.8.22/libavcodec/ppc/h264chroma_init.c" -o "$output/chroma.s"
"$compiler" "${flags[@]}" -S "$project/tests/test_video_vmx.c" -o "$output/test.s"
python3 - "$output" <<'PY'
from pathlib import Path
import re,sys
for name in ['chroma.s','test.s']:
 p=Path(sys.argv[1])/name;s=p.read_text()
 # Only adapt the harness ABI's 64-bit GPR saves for the 32-bit Linux VM.
 # All vector instructions and arithmetic are unchanged. Production isn't edited.
 s=re.sub(r'\b(std|ld)\s+(\d+),(-?\d+)\(1\)',lambda m:('stw' if m[1]=='std' else 'lwz')+' '+m[2]+','+str(int(m[3])+4)+'(1)',s)
 if re.search(r'\b(std|ld|rldic\w*|sld)\s',s):raise SystemExit('Unexpected64-bit instruction in '+name)
 p.write_text(s)
PY
"$compiler" "${flags[@]}" -nostdlib "$output/chroma.s" "$output/test.s" -Wl,-e,_start -o "$output/check"
"$qemu" -cpu 7400 "$output/check"
