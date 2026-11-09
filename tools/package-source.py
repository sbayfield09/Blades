#!/usr/bin/env python3
"""Complete 0.53 source/relink bundle, using an explicit public-file whitelist."""
import argparse,hashlib,io,subprocess,tarfile,zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sdk',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
r=Path(__file__).resolve().parent.parent;sdk_commit='5f6dc726f65f6d171493a3628c2415c03551f27b'
assert subprocess.check_output(['git','-C',str(a.sdk),'rev-parse','HEAD'],text=True).strip()==sdk_commit
files=[r/x for x in ['CMakeLists.txt','.gitignore','LICENSE','COPYING.GPLv3','README.md','AGENTS.md','docs/CONSOLE-TEST.txt','docs/VIDEO-PLAYBACK.md','docs/JELLYFIN.md','docs/FTP.md','docs/UPDATES.md','docs/SIMPLE360PLAYER-PIPELINE.md','docs/VIDEO-LICENSE-NOTICE.txt','docs/BUILDING-SOURCE.txt','tools/build-video-ffmpeg.py','tools/build-xbox.sh','tools/verify_xex.py','tools/package-usb.py','tools/package-source.py','tools/publish-release.py','tools/prepare-update.py','tools/embed-update-roots.py','tools/test-video-vmx.sh','tools/test-simple360-dsp.py','tools/fetch-jellyfin-test-server.py','tools/record-walkthrough.py','tools/verify-video-decoder.py','tools/test-gpu-shaders.py','tools/gl_renderer.c','tools/gl_renderer.h','tools/render-walkthrough.c','dist/Blades-artwork-pack.zip']]
legacy={'video_handoff.h','xbox_video.h','video_handoff.c','xbox_video.c','test_video_backend.c','test_video_handoff.c'}
for folder in ['include','src','assets','third_party','tests']:
 files.extend(f for f in sorted((r/folder).rglob('*')) if f.is_file() and f.name not in legacy and '__pycache__' not in f.parts)
assert len(files)==len(set(files))
manifest=[]
with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for f in files:
  name='Blades/'+f.relative_to(r).as_posix();data=f.read_bytes();z.writestr(name,data);manifest.append((name,hashlib.sha256(data).hexdigest()))
 z.writestr('Blades/BUILDING.txt',(r/'docs/BUILDING-SOURCE.txt').read_bytes())
 sdk_tar=subprocess.check_output(['git','-C',str(a.sdk),'archive',sdk_commit])
 z.writestr('Blades/dependencies/xsdk/BLADES-SDK-COMMIT',sdk_commit+'\n')
 with tarfile.open(fileobj=io.BytesIO(sdk_tar)) as t:
  for entry in t:
   if entry.isfile() and Path(entry.name).parts[0]!='results' and not entry.name.endswith('.log'):
    name='Blades/dependencies/xsdk/'+entry.name;data=t.extractfile(entry).read();z.writestr(name,data);manifest.append((name,hashlib.sha256(data).hexdigest()))
 z.writestr('Blades/SOURCE-SHA256SUMS',''.join(f'{sha}  {name.removeprefix("Blades/")}\n' for name,sha in manifest))
with zipfile.ZipFile(a.output) as z:
 assert z.testzip() is None
 assert not any('/Players/' in n or '/simple360-adapter/' in n or '/native-player-probe/' in n or n.endswith('.log') for n in z.namelist())
 assert z.read('Blades/src/core/video.c')==(r/'src/core/video.c').read_bytes()
 assert z.read('Blades/dependencies/xsdk/src/xsdk.ld')==(a.sdk/'src/xsdk.ld').read_bytes()
print(f'Complete source/relink bundle: {a.output} ({a.output.stat().st_size} bytes)')
