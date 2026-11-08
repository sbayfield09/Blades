"""Compare real disk/loopback FTP throughput; these are not console speeds.
Usage: python3 tests/benchmark_ftp.py before-fixture after-fixture
"""
import ftplib,hashlib,io,json,os,pathlib,statistics,subprocess,sys,tempfile,time
def measure(binary):
 with tempfile.TemporaryDirectory(prefix='blades-ftp-benchmark-') as directory:
  root=pathlib.Path(directory)
  for drive in ('HDD','USB0'):(root/drive).mkdir()
  process=subprocess.Popen([str(pathlib.Path(binary).resolve()),str(root),str(root/'ftp.txt')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
  def status():
   process.stdin.write('status\n');process.stdin.flush();return json.loads(process.stdout.readline())
  try:
   until=time.monotonic()+5
   while True:
    state=status()
    if state['state']==2:break
    assert time.monotonic()<until;time.sleep(.01)
   ftp=ftplib.FTP();ftp.connect('127.0.0.1',state['port']);ftp.login('blades','blades')
   payload=os.urandom(32*1024*1024+137);expected=hashlib.sha256(payload).digest();upload=[];download=[]
   for run in range(3):
    start=time.monotonic();ftp.storbinary('STOR /HDD/benchmark.bin',io.BytesIO(payload),blocksize=65536);upload.append(len(payload)/(time.monotonic()-start)/1048576)
    assert hashlib.sha256((root/'HDD/benchmark.bin').read_bytes()).digest()==expected
    digest=hashlib.sha256();start=time.monotonic();ftp.retrbinary('RETR /HDD/benchmark.bin',digest.update,blocksize=65536);download.append(len(payload)/(time.monotonic()-start)/1048576);assert digest.digest()==expected
   start=time.monotonic()
   for n in range(48):ftp.storbinary(f'STOR /USB0/small-{n}.bin',io.BytesIO(payload[:4096]),blocksize=4096)
   small=time.monotonic()-start
   assert len(list((root/'USB0').iterdir()))==48
   ftp.quit()
   return {'binary':pathlib.Path(binary).name,'bulk_bytes':len(payload),'rounds':3,'upload_MiB_s':upload,'download_MiB_s':download,'median_upload_MiB_s':statistics.median(upload),'median_download_MiB_s':statistics.median(download),'48_small_files_seconds':small,'sha256_verified':True}
  finally:
   process.stdin.write('quit\n');process.stdin.flush();assert process.wait(timeout=6)==0
   process.stdin.close();process.stdout.close()
print(json.dumps({'scope':'Desktop loopback and temporary local disk. Not Xbox/Aurora throughput; XAM buffer increases require console verification.','results':[measure(binary) for binary in sys.argv[1:]]},indent=2))
