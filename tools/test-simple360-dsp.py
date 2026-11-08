#!/usr/bin/env python3
"""Execute ported Xenon filters against scalar H264 on PPC (not a speed test)."""
import argparse,os,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('build',type=Path);p.add_argument('--qemu',default='qemu-ppc');a=p.parse_args()
r=Path(__file__).resolve().parent.parent;b=a.build.resolve();out=r/'build/simple360-dsp-check';out.mkdir(parents=True,exist_ok=True)
cc=Path(os.environ['XSDK_TOOLCHAIN'])/'bin/powerpc-xsdk-elf-gcc'
flags=['-mcpu=7400','-mno-powerpc64','-O3','-fno-pic','-msdata=none','-ffreestanding','-fno-builtin','-fno-store-merging','-mblock-move-inline-limit=0','-fno-tree-vectorize','-fno-tree-slp-vectorize','-ffunction-sections','-fdata-sections','-I'+str(b/'objects'),'-I'+str(b/'source/FFmpeg-n2.8.22')]
inputs=[('chroma-vmx',r/'third_party/ffmpeg/xenon/simple360_chroma.c',True),('qpel-vmx',r/'third_party/ffmpeg/xenon/simple360_qpel.c',True),('test',r/'tests/test_simple360_dsp.c',False)]
objects=[]
for name,path,vector in inputs:
 asm=out/(name+'.s');subprocess.run([str(cc),*flags,'-DBLADES_DSP_REFERENCE_ONLY',*(['-O1'] if name=='test' else []),'-maltivec' if vector else '-mno-altivec','-S',str(path),'-o',str(asm)],check=True)
 s=asm.read_text()
 # SDK compiler uses 64-bit GPR saves with its 32-bit pointers. Adapt only
 # those stack saves to the freestanding Linux32 test ABI, as in the prior
 # VMX harness; filter arithmetic and all vector instructions are unchanged.
 s=re.sub(r'\b(std|ld)\s+(\d+),(-?\d+)\(1\)',lambda m:('stw' if m[1]=='std' else 'lwz')+' '+m[2]+','+str(int(m[3])+4)+'(1)',s)
 s=re.sub(r'\brldicl\s+(\d+),(\d+),0,32',r'mr \1,\2',s) #32-bit ABI pointer extension
 if re.search(r'\b(std|ld|rldic\w*|sld)\s',s):raise SystemExit('Unexpected64-bit instruction in '+name)
 asm.write_text(s);obj=out/(name+'.o');subprocess.run([str(cc),*flags,'-maltivec','-c',str(asm),'-o',str(obj)],check=True);objects.append(str(obj))
exe=out/'check';subprocess.run([str(cc),*flags,'-nostdlib',*objects,'-Wl,-e,_start,--gc-sections','-o',str(exe)],check=True)
subprocess.run([a.qemu,'-cpu','7400',str(exe)],check=True)
