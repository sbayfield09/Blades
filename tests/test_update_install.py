"""Real package staging/commit/recovery on disposable app trees; no user data."""
import io,json,os,pathlib,random,struct,subprocess,sys,tempfile,zipfile
binary=pathlib.Path(sys.argv[1]).resolve()
def xex(text):return b'XEX2'+text.encode()+b'\0'*(8192-len(text)-4)
old={'default.xex':xex('old app'),'update-installer.xex':xex('old helper'),'Assets/blades.xzp':b'old artwork','README.txt':b'old readme'}
new={'default.xex':xex('new app'),'update-installer.xex':xex('new helper'),'Assets/blades.xzp':b'new artwork','README.txt':b'new readme','licenses/NEW.txt':b'new notice'}
private={'Config/ftp-settings.txt':b'private ftp','Config/jellyfin-settings.json':b'private session','Config/library-paths.txt':b'private paths','Updates/keep.zip':b'prior download','default.log':b'prior log'}
def package(path,entries,compression=zipfile.ZIP_DEFLATED):
 with zipfile.ZipFile(path,'w',compression) as z:
  for name,data in entries.items():z.writestr('Blades/'+name,data)
def run(root,mode,archive=None,expected=0):
 result=subprocess.run([str(binary),str(root),mode,*([str(archive)] if archive else [])],capture_output=True,text=True,timeout=30)
 assert result.returncode==expected,(mode,result.returncode,result.stdout,result.stderr)
def setup(base,name):
 root=base/name;root.mkdir()
 for name,data in {**old,**private}.items():
  path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
 return root
def check(root,files):
 for name,data in {**files,**private}.items():assert (root/name).read_bytes()==data,name
 if files==old:assert not (root/'licenses/NEW.txt').exists()
with tempfile.TemporaryDirectory(prefix='blades-installer-') as directory:
 base=pathlib.Path(directory);archive=base/'update.zip';package(archive,new)
 for compression in (zipfile.ZIP_DEFLATED,zipfile.ZIP_STORED):
  root=setup(base,'success-'+str(compression));package(archive,new,compression);run(root,'stage',archive);check(root,old);run(root,'apply');check(root,new)
 print('PASS: stored/deflated USB updates stage without touching installation, commit all files, preserve Config/Updates/logs')
 package(archive,new)
 for mode in ['fail-'+str(i) for i in range(1,6)]+['crash-'+str(i) for i in range(1,6)]:
  root=setup(base,mode);run(root,'stage',archive);run(root,mode,expected=86 if mode.startswith('crash') else 0)
  assert (root/'default.xex').exists()
  if mode.startswith('crash'):run(root,'recover')
  check(root,old)
 print('PASS: every replacement failure and simulated process interruption restores original app/artwork and removes new files; default.xex never disappears')
 root=setup(base,'cancel');run(root,'cancel',archive);check(root,old)
 root=setup(base,'tamper');run(root,'stage',archive);(root/'.blades-update/stage/README.txt').write_bytes(b'tampered');run(root,'reject-apply');check(root,old)
 root=setup(base,'missing-helper');(root/'update-installer.xex').unlink();run(root,'reject',archive)
 root=setup(base,'retry-recovery');run(root,'stage',archive);run(root,'crash-2',expected=86)
 backup=root/'.blades-update/backup/default.xex';data=backup.read_bytes();backup.write_bytes(b'bad backup');run(root,'retain-journal');backup.write_bytes(data);run(root,'recover');check(root,old)
 root=setup(base,'interrupted-recovery');run(root,'stage',archive);run(root,'crash-5',expected=86);run(root,'recover-crash-2',expected=86);assert (root/'default.xex').exists();run(root,'recover');check(root,old)
 print('PASS: cancellation, staged tampering, missing helper and damaged-backup recovery fail safely; journal permits retry')
 for name,extra in [('traversal','../escape'),('absolute','/escape'),('backslash','Assets\\escape'),('drive','Hdd:/escape'),('config','Config/ftp-settings.txt'),('nested','Assets/nested/escape'),('reserved','Updates/new.zip')]:
  root=setup(base,name);package(archive,{**new,extra:b'bad'});run(root,'reject',archive);check(root,old)
 root=setup(base,'case-duplicate');package(archive,{**new,'Assets/BLADES.xzp':b'duplicate'});run(root,'reject',archive);check(root,old)
 root=setup(base,'symlink-entry');package(archive,new)
 with zipfile.ZipFile(archive,'a') as z:
  entry=zipfile.ZipInfo('Blades/Assets/link');entry.create_system=3;entry.external_attr=0o120777<<16;z.writestr(entry,'/outside')
 run(root,'reject',archive);check(root,old)
 root=setup(base,'bad-crc');package(archive,new,zipfile.ZIP_STORED);data=bytearray(archive.read_bytes());data[60]^=1;archive.write_bytes(data);run(root,'reject',archive);check(root,old)
 root=setup(base,'truncated');archive.write_bytes(b'PK\x03\x04');run(root,'reject',archive);check(root,old)
 root=setup(base,'oversized');package(archive,new);data=bytearray(archive.read_bytes());at=data.index(b'PK\x01\x02');struct.pack_into('<I',data,at+24,33*1024*1024);archive.write_bytes(data);run(root,'reject',archive);check(root,old)
 print('PASS: unsafe paths, private Config writes, case duplicates, ZIP symlinks, CRC corruption, truncation and expansion bounds rejected')
 if hasattr(os,'symlink'):
  root=setup(base,'symlink-parent');outside=base/'outside';outside.mkdir();(outside/'blades.xzp').write_bytes(b'outside');(root/'Assets/blades.xzp').unlink();(root/'Assets').rmdir();(root/'Assets').symlink_to(outside,target_is_directory=True)
  package(archive,new);run(root,'stage',archive);run(root,'reject-apply');assert (outside/'blades.xzp').read_bytes()==b'outside'
  print('PASS: filesystem symlink parent cannot redirect installed files outside the app')
 package(archive,new);valid=archive.read_bytes();randomizer=random.Random(520)
 for i in range(80):
  root=setup(base,'mutated-'+str(i));data=bytearray(valid)
  for _ in range(1+i%4):data[randomizer.randrange(len(data))]^=1<<randomizer.randrange(8)
  archive.write_bytes(data);run(root,'fuzz',archive);check(root,old)
 print('PASS:80 deterministic ZIP header/DEFLATE/directory mutations leave installation unchanged without parser errors')
