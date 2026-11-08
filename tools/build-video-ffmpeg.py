#!/usr/bin/env python3
"""Build pinned FFmpeg with the Simple360Player Xenon filter port for the existing xsdk or desktop toolchain."""
import argparse,hashlib,os,subprocess,tarfile,shutil
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--target',choices=['xbox','host'],required=True)
p.add_argument('--build',required=True,type=Path)
p.add_argument('--sdk',type=Path,default=Path('/workspace/xsdk'))
p.add_argument('--toolchain',type=Path,default=Path('/workspace/xsdk-gcc'))
p.add_argument('--jobs',type=int,default=4)
a=p.parse_args();root=Path(__file__).resolve().parent.parent
archive=root/'third_party/ffmpeg/ffmpeg-n2.8.22.tar.gz';patch=root/'third_party/ffmpeg/patches/xsdk.patch'
assert hashlib.sha256(archive.read_bytes()).hexdigest()=='9f2ed903682524fbfb6999f2bb106f02219df1393fece58cae9da1a1ef780358','FFmpeg source checksum mismatch'
port=root/'third_party/ffmpeg/xenon'
patch_hash=hashlib.sha256(patch.read_bytes()+b''.join(p.read_bytes() for p in sorted(port.glob('*')))).hexdigest()
b=a.build.resolve();b.mkdir(parents=True,exist_ok=True);source=b/'source/FFmpeg-n2.8.22';install=b/'install';obj=b/'objects'
if not source.exists():
 with tarfile.open(archive) as t:t.extractall(b/'source',filter='data')
 subprocess.run(['patch','--batch','-p1','-i',str(patch)],cwd=source,check=True)
 shutil.copytree(port,source/'libavcodec/xenon',dirs_exist_ok=True)
 (source/'.blades-patch-sha256').write_text(patch_hash)
assert (source/'.blades-patch-sha256').read_text()==patch_hash,'Decoder patch changed; choose a fresh build directory'
obj.mkdir(exist_ok=True)
options=['--prefix='+str(install),'--disable-programs','--disable-doc','--disable-network','--disable-avdevice','--disable-avfilter','--disable-postproc',
 '--disable-everything','--disable-fast-unaligned','--disable-debug','--disable-shared','--enable-static','--enable-pthreads',
 '--disable-zlib','--disable-bzlib','--disable-lzma','--disable-iconv','--disable-sdl','--disable-xlib','--enable-gpl','--enable-version3','--disable-nonfree',
 '--enable-avcodec','--enable-avformat','--enable-avutil','--enable-swscale','--enable-swresample',
 '--enable-decoder=h264,mpeg4,msmpeg4v2,msmpeg4v3,wmv1,wmv2,wmv3,vc1,mpeg1video,mpeg2video,h263,vp6f,flv,mjpeg,rawvideo,aac,ac3,eac3,dca,mp3,mp2,wmav1,wmav2,pcm_s16le,pcm_s16be,flac,vorbis',
 '--enable-demuxer=mov,matroska,avi,asf,mpegts,mpegps,mpegvideo,flv,mp3,wav,flac,ogg',
 '--enable-parser=h264,mpeg4video,mpegaudio,aac,ac3,vc1,mpegvideo','--enable-protocol=file']
if a.target=='xbox':
 options+=['--disable-asm','--disable-altivec','--enable-cross-compile','--cross-prefix='+str(a.toolchain.resolve()/'bin/powerpc-xsdk-elf-'),'--target-os=none','--arch=ppc','--cpu=cell',
  '--extra-cflags=-mcpu=cell -mno-altivec -fno-tree-vectorize -fno-tree-slp-vectorize -fno-pic -std=gnu99 -I'+str(a.sdk.resolve()/'include'),
  '--extra-ldflags=-static -nostdlib -T '+str(a.sdk.resolve()/'src/xsdk.ld')+' -Wl,--start-group',
  '--extra-libs='+str(a.sdk.resolve()/'build/libxsdk.a')+' -lc -lm -lgcc -Wl,--end-group']
else:options+=['--disable-asm','--extra-cflags=-std=gnu99 -fPIC']
stamp='\n'.join(options)+'\n';config=obj/'blades-configure.txt'
if not config.exists() or config.read_text()!=stamp:
 subprocess.run([str(source/'configure'),*options],cwd=obj,check=True)
 config.write_text(stamp)
subprocess.run(['make','-j'+str(max(1,a.jobs))],cwd=obj,check=True)
subprocess.run(['make','install'],cwd=obj,check=True)
if a.target=='xbox':
 for required in ['#define ARCH_PPC 0','#define HAVE_ALTIVEC 0','#define HAVE_VSX 0','#define HAVE_POWER8 0','#define HAVE_LDBRX 0']:
  assert required in (obj/'config.h').read_text(),required+' required for the Xbox compatibility build'
assert '#define CONFIG_GPL 1' in (obj/'config.h').read_text()
assert '#define CONFIG_NONFREE 0' in (obj/'config.h').read_text()
print('BLADES_FFMPEG_PREFIX='+str(install))
