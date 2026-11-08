#!/usr/bin/env python3
"""Verify repository-hosted downloads and write dist/update.json atomically.
Commit the manifest and every matching versioned artifact together on main.
No GitHub API or release service is required.
"""
import argparse,hashlib,json,pathlib,re,zipfile
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--version',required=True);a=p.parse_args()
 r=pathlib.Path(__file__).resolve().parents[1];d=r/'dist';v=a.version
 assert all(c in '0123456789.' for c in v) and v.count('.') in (1,2)
 full=v if v.count('.')==2 else v+'.0'
 assert re.search(r'project\(Blades VERSION ([0-9.]+)',(r/'CMakeLists.txt').read_text())[1]==full
 assert re.search(r'#define BLADES_VERSION "([0-9.]+)"',(r/'include/updater.h').read_text())[1]==full
 paths=[d/f'Blades-v{v}-USB.zip',d/'Blades/default.xex',d/'Blades-artwork-pack.zip',d/f'Blades-v{v}-walkthrough.mp4',d/f'Blades-v{v}-source.zip']
 def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
 assert all(p.is_file() and p.stat().st_size>1000 for p in paths)
 for p in [paths[0],paths[2],paths[4]]:
  with zipfile.ZipFile(p) as z:assert z.testzip() is None
 with zipfile.ZipFile(paths[0]) as z:
  assert z.read('Blades/default.xex')==paths[1].read_bytes()
  assert all(not n.startswith('/') and '..' not in pathlib.Path(n).parts for n in z.namelist())
  assert not any(n.endswith('.log') or pathlib.Path(n).name in {'jellyfin-settings.json','ftp-settings.txt','library-paths.txt'} for n in z.namelist())
 manifest={'schema':1,'version':full,'asset':paths[0].name,'size':paths[0].stat().st_size,'sha256':sha(paths[0])}
 temporary=d/'update.json.tmp';temporary.write_text(json.dumps(manifest,indent=2)+'\n');temporary.replace(d/'update.json')
 paths.append(d/'update.json');(d/'SHA256SUMS').write_text(''.join(sha(p)+'  '+p.relative_to(d).as_posix()+'\n' for p in paths))
 b=r/('build/media-v'+v);b.mkdir(parents=True,exist_ok=True)
 plan={'source':'https://raw.githubusercontent.com/sbayfield09/Blades-reimagined/main/dist/','manifest':manifest,'artifacts':{p.relative_to(r).as_posix():{'bytes':p.stat().st_size,'sha256':sha(p)} for p in paths+[d/'SHA256SUMS']}}
 (b/'repository-update-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
 print('Verified repository updater manifest:',d/'update.json')
 print('Publish matching source, versioned artifacts and manifest together on main.')
if __name__=='__main__':main()
