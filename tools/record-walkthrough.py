#!/usr/bin/env python3
"""Capture the release's shared GPU renderer; optionally use a disposable Jellyfin server."""
import argparse,os,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--version',default='0.52');a=p.parse_args()
r=Path(__file__).resolve().parents[1];b=r/('build/media-v'+a.version);b.mkdir(parents=True,exist_ok=True)
env=dict(os.environ,BLADES_RESOURCE_PACK=str(r/'dist/Blades/Assets/blades.xzp'),BLADES_TEST_MEDIA=str(r/'tests/media'),EGL_PLATFORM='surfaceless',LIBGL_ALWAYS_SOFTWARE='1')
label="drawbox=x=0:y=0:w=iw:h=21:color=black@0.78:t=fill,drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:text='Blades "+a.version+" | DESKTOP GPU | CPU decode / GPU YUV | Guide-profile-storage-FTP-updater simulated; isolated Jellyfin server':x=10:y=2:fontsize=14:fontcolor=white"
with (b/'walkthrough-events.txt').open('w') as events,(b/'walkthrough-encode.txt').open('w') as log:
 cap=subprocess.Popen([str(r/'build/linux/blades-gpu-walkthrough'),str(b/'walkthrough-audio.f32')],stdout=subprocess.PIPE,stderr=events,env=env)
 enc=subprocess.Popen(['ffmpeg','-hide_banner','-nostats','-y','-f','rawvideo','-pixel_format','bgra','-video_size','1280x720','-framerate','30','-i','pipe:0','-vf',label,'-c:v','libx264','-preset','fast','-crf','21','-pix_fmt','yuv420p','-threads','4',str(b/'walkthrough-silent.mp4')],stdin=cap.stdout,stderr=log)
 cap.stdout.close();encode=enc.wait();capture=cap.wait();assert capture==0 and encode==0,(capture,encode)
 subprocess.run(['ffmpeg','-hide_banner','-nostats','-y','-i',str(b/'walkthrough-silent.mp4'),'-f','f32le','-ar','48000','-ac','2','-i',str(b/'walkthrough-audio.f32'),'-c:v','copy','-c:a','aac','-b:a','192k','-movflags','+faststart','-shortest',str(r/('dist/Blades-v'+a.version+'-walkthrough.mp4'))],stderr=log,check=True)
output=r/('dist/Blades-v'+a.version+'-walkthrough.mp4')
with (b/'walkthrough-probe.json').open('w') as out:subprocess.run(['ffprobe','-v','error','-show_streams','-show_format','-of','json',str(output)],stdout=out,check=True)
with (b/'walkthrough-decode.txt').open('w') as log:subprocess.run(['ffmpeg','-v','error','-i',str(output),'-f','null','-'],stderr=log,check=True)
print('Fresh240second720p30 desktop GPU walkthrough captured and fully decoded',flush=True)
