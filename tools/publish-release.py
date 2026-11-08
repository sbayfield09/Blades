#!/usr/bin/env python3
"""Publish verified binaries as a draft GitHub Release, verify, then publish.
Uses normal gh authentication. Never overwrites an already-published release.
"""
import argparse,hashlib,json,pathlib,subprocess,tempfile,zipfile,re
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',default='sbayfield09/Blades-reimagined');p.add_argument('--version',required=True);p.add_argument('--commit',required=True);p.add_argument('--prepare-only',action='store_true');a=p.parse_args()
 root=pathlib.Path(__file__).resolve().parents[1];dist=root/'dist';version=a.version;tag='v'+(version if version.count('.')==2 else version+'.0')
 assert len(a.commit)==40 and all(c in '0123456789abcdef' for c in a.commit)
 assert all(c in '0123456789.' for c in version) and version.count('.') in (1,2)
 assert re.search(r'project\(Blades VERSION ([0-9.]+)',(root/'CMakeLists.txt').read_text())[1]==tag[1:]
 assert re.search(r'#define BLADES_VERSION "([0-9.]+)"',(root/'include/updater.h').read_text())[1]==tag[1:]
 def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
 usb=dist/f'Blades-v{version}-USB.zip';source=dist/f'Blades-v{version}-source.zip';video=dist/f'Blades-v{version}-walkthrough.mp4'
 files=[usb,dist/'Blades/default.xex',dist/'Blades-artwork-pack.zip',video,source]
 assert all(f.is_file() and f.stat().st_size>1000 for f in files)
 for f in (usb,source,files[2]):
  with zipfile.ZipFile(f) as z:assert z.testzip() is None
 with zipfile.ZipFile(usb) as z:
  assert z.read('Blades/default.xex')==files[1].read_bytes()
  assert not any(n.endswith('.log') or pathlib.Path(n).name in {'jellyfin-settings.json','ftp-settings.txt','library-paths.txt'} for n in z.namelist())
 output=root/('build/media-v'+version);output.mkdir(parents=True,exist_ok=True)
 manifest={'schema':1,'version':tag[1:],'asset':usb.name,'size':usb.stat().st_size,'sha256':sha(usb)}
 update=output/'update.json';update.write_text(json.dumps(manifest,indent=2)+'\n')
 checksums=output/'SHA256SUMS';checksums.write_text(''.join(sha(f)+'  '+f.name+'\n' for f in files+[update]))
 files += [update,checksums]
 report={'repository':a.repo,'tag':tag,'commit':a.commit,'artifacts':{f.name:{'bytes':f.stat().st_size,'sha256':sha(f)} for f in files},'manifest':manifest}
 (output/'release-plan.json').write_text(json.dumps(report,indent=2)+'\n')
 if a.prepare_only:print('Verified release plan and update manifest:',output/'release-plan.json');return
 def gh(*args,**kw):return subprocess.check_output(['gh',*args],text=True,**kw)
 existing=subprocess.run(['gh','release','view',tag,'--repo',a.repo,'--json','isDraft'],text=True,capture_output=True)
 if existing.returncode==0:
  assert json.loads(existing.stdout)['isDraft'],'Refusing to replace published release assets'
 else:gh('release','create',tag,'--repo',a.repo,'--target',a.commit,'--title','Blades '+tag[1:],'--notes-file',str(root/f'docs/RELEASE-{version}.md'),'--draft')
 gh('release','upload',tag,*map(str,files),'--repo',a.repo,'--clobber')
 release=json.loads(gh('api','repos/'+a.repo+'/releases/tags/'+tag));assets={f['name']:f for f in release['assets']}
 for f in files:
  assert assets[f.name]['size']==f.stat().st_size and assets[f.name]['state']=='uploaded',f.name
  digest=assets[f.name].get('digest');assert not digest or digest=='sha256:'+sha(f),f.name
 with tempfile.TemporaryDirectory(prefix='blades-release-verify-') as directory:
  gh('release','download',tag,'--repo',a.repo,'--pattern',usb.name,'--pattern','update.json','--dir',directory)
  assert sha(pathlib.Path(directory)/usb.name)==sha(usb)
  assert json.loads((pathlib.Path(directory)/'update.json').read_text())==manifest
 gh('release','edit',tag,'--repo',a.repo,'--draft=false','--latest')
 latest=json.loads(gh('api','repos/'+a.repo+'/releases/latest'));assert latest['tag_name']==tag and not latest['draft']
 (output/'published-release.json').write_text(json.dumps({'url':latest['html_url'],'tag':tag,'assets':{f['name']:f['browser_download_url'] for f in latest['assets']}},indent=2)+'\n')
 print('Published and verified:',latest['html_url'])
if __name__=='__main__':main()
