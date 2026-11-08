#!/usr/bin/env python3
"""Actual decoded MP2/MPEG2 sync across a missing half-second audio timestamp range."""
import pathlib,subprocess,sys,tempfile
with tempfile.TemporaryDirectory(prefix='blades-av-sync-') as directory:
    path=pathlib.Path(directory)/'flash-tone.ts'
    subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','color=c=black:s=160x90:r=30:d=6',
        '-f','lavfi','-i',r'aevalsrc=if(between(t\,3\,3.15)\,0.6*sin(2*PI*1000*t)\,0):s=48000:d=6',
        '-vf',"drawbox=x=0:y=0:w=iw:h=ih:color=white:t=fill:enable='between(t,3,3.2)'",
        '-af',"aselect='not(between(t,2,2.5))'",'-c:v','mpeg2video','-g','12','-threads','2','-c:a','mp2','-ac','2','-f','mpegts',str(path)],check=True)
    subprocess.run([sys.argv[1],str(path)],check=True,timeout=20)
