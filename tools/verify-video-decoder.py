#!/usr/bin/env python3
"""Verify the shipping Xbox decoder configuration and instruction restriction."""
import argparse,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('build',type=Path);p.add_argument('--objdump',required=True);a=p.parse_args()
config=(a.build/'objects/config.h').read_text()
for required in ['ARCH_PPC 0','HAVE_ALTIVEC 0','CONFIG_GPL 1','CONFIG_VERSION3 1','CONFIG_NONFREE 0']:
 assert '#define '+required in config,required
flags=(a.build/'objects/config.mak').read_text()
for required in ['-mno-altivec','-fno-tree-vectorize','-fno-tree-slp-vectorize']:assert required in flags,required
instruction=re.compile(r'^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+){4}(\S+)')
count=0;seen=set();member=''
allowed={'lvx','lvsl','lvsr','stvx','vadduhm','vsubuhm','vadduwm','vsubuwm',
 'vslh','vslw','vsrah','vsraw','vmrghb','vmrglb','vsldoi','vupkhsh','vupklsh',
 'vpkshus','vpkswss','vavgub','vperm','vsel','vxor','vor','vmr','vspltisb','vspltish','vspltisw'}
for name in ['avcodec','avformat','avutil','swscale','swresample']:
 proc=subprocess.Popen([a.objdump,'-d',str(a.build/'install/lib'/('lib'+name+'.a'))],stdout=subprocess.PIPE,text=True)
 for line in proc.stdout:
  if 'file format' in line:member=line.split(':',1)[0].strip()
  match=instruction.match(line)
  if match:
   op=match[1];count+=1
   vector=op.startswith('v') or op.startswith(('lvx','lvsl','lvsr','lve','stvx','stve')) or op in {'mfvscr','mtvscr'}
   if vector:
    assert name=='avcodec' and member in {'simple360_chroma.o','simple360_qpel.o'},(member,line)
    assert op in allowed,(member,line)
    seen.add(member)
 assert proc.wait()==0
assert seen=={'simple360_chroma.o','simple360_qpel.o'},seen
print(f'PASS: {count} instructions checked across five archives; vectors only in the two Simple360 filter objects, restricted to the explicit Xenon shift/add/permute subset. Generic PPC/AltiVec remains off; GPLv3 enabled, nonfree off.')
