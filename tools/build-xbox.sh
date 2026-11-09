#!/bin/bash
set -euo pipefail
project=$(cd "$(dirname "$0")/.." && pwd)
: "${XSDK_ROOT:?Set XSDK_ROOT to your xsdk checkout}"
: "${XSDK_TOOLCHAIN:?Set XSDK_TOOLCHAIN to your xsdk-gcc prefix}"
make -C "$XSDK_ROOT" -j"${JOBS:-4}" TOOLCHAIN="$XSDK_TOOLCHAIN/bin/powerpc-xsdk-elf-" build/libxsdk.a build/host/libxsc.so
if [ -z "${BLADES_FFMPEG_PREFIX:-}" ]; then
  python3 "$project/tools/build-video-ffmpeg.py" --target xbox --build "$project/build/ffmpeg-xbox-v043-final" --sdk "$XSDK_ROOT" --toolchain "$XSDK_TOOLCHAIN" --jobs "${JOBS:-4}"
  BLADES_FFMPEG_PREFIX="$project/build/ffmpeg-xbox-v043-final/install"
fi
cmake -S "$project" -B "$project/build/xbox" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$XSDK_ROOT/cmake/xsdk.cmake" -DXSDK_TOOLCHAIN="$XSDK_TOOLCHAIN" \
  -DBLADES_FFMPEG_PREFIX="$BLADES_FFMPEG_PREFIX"
cmake --build "$project/build/xbox" -j"${JOBS:-4}"
python3 "$project/tools/verify_xex.py" "$project/build/xbox/default.xex"
mkdir -p "$project/dist/Blades/Assets" "$project/dist/Blades/Config" "$project/dist/Blades/licenses"
cp "$project/build/xbox/default.xex" "$project/dist/Blades/default.xex"
python3 "$project/tools/verify_xex.py" "$project/build/xbox/update-installer.xex"
cp "$project/build/xbox/update-installer.xex" "$project/dist/Blades/update-installer.xex"
cp "$project/assets/FONT-LICENSE.txt" "$project/dist/Blades/Assets/"
cp -r "$project/assets/licenses/." "$project/dist/Blades/licenses/"
cp "$project/third_party/ffmpeg/LICENSE" "$project/dist/Blades/licenses/FFMPEG-LGPL-2.1.txt"
cp "$project/third_party/simple360player/COPYING.GPLv3" "$project/dist/Blades/licenses/SIMPLE360PLAYER-GPL-3.0.txt"
cp "$project/third_party/xenonfin/COPYING" "$project/dist/Blades/licenses/XENONFIN-GPL-2.0.txt"
cp "$project/docs/VIDEO-LICENSE-NOTICE.txt" "$project/dist/Blades/licenses/"
cp "$project/docs/CONSOLE-TEST.txt" "$project/dist/Blades/README.txt"
cp "$project/LICENSE" "$project/dist/Blades/licenses/BLADES-MIT.txt"
cp "$project/third_party/simple360player/COPYING.GPLv3" "$project/dist/Blades/LICENSE"
cp "$XSDK_ROOT/LICENSE" "$project/dist/Blades/XSDK-LICENSE.txt"
cp "$XSDK_ROOT/THIRD_PARTY_NOTICES.md" "$project/dist/Blades/"
printf 'The USB-ready ZIP includes default.xex, update-installer.xex and Assets/blades.xzp.\nExtract Blades-v0.53-USB.zip onto USB and launch Blades/default.xex.\nKeep update-installer.xex beside default.xex for automatic updates.\nNo DashLaunch plugin, video helper, DLL or artwork converter is required.\nSee artwork notices for sources and attribution.\n' > "$project/dist/Blades/Assets/README.txt"
printf 'Blades saves your chosen scan folders to Config/library-paths.txt.\nFTP enabled state and login details are saved in Config/ftp-settings.txt.\nDefaults: FTP on; username/password blades / blades.\nJellyfin server/account/session: Config/jellyfin-settings.json.\nJellyfin passwords are never saved; keep this private session file private.\nThese files belong to Blades; no console-wide settings are changed.\n' > "$project/dist/Blades/Config/README.txt"
{
  printf 'Blades 0.53.0 - automatic update and restart build\nSDK commit: '
  if [ -e "$XSDK_ROOT/.git" ]; then git -C "$XSDK_ROOT" rev-parse HEAD
  else cat "$XSDK_ROOT/BLADES-SDK-COMMIT"; fi
  "$XSDK_TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" --version | head -1
  printf 'XEX container version: 0.0.53.0\nTitle ID: 0x424C4453\nResolution: 1280x720\nVideo: FFmpeg 2.8.22 + Simple360 H264 DSP, GPLv3, statically linked, same XEX/GPU/audio\nNew Xbox playback: not yet hardware verified\n'
} > "$project/dist/Blades/build-info.txt"
(cd "$project/dist/Blades" && sha256sum default.xex update-installer.xex > SHA256SUMS)
python3 "$project/tools/package-usb.py" "$project/dist" 0.53
# Archive the same explicit safe payload as the ZIP, never stale Players/logs.
python3 - "$project/dist" <<'PY'
import io,sys,tarfile,zipfile
from pathlib import Path
base=Path(sys.argv[1])
with zipfile.ZipFile(base/'Blades-v0.53-USB.zip') as z,tarfile.open(base/'Blades-xbox360.tar.gz','w:gz') as t:
 for name in z.namelist():
  data=z.read(name);info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o644;t.addfile(info,io.BytesIO(data))
PY
echo "Built: $project/dist/Blades/default.xex"
