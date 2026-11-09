"""Real TLS/download worker tests with disposable CA, server and synthetic ZIP.
No GitHub account or user files. Never disables certificate verification.
"""
import hashlib,http.server,io,json,os,pathlib,ssl,subprocess,sys,tempfile,threading,time,zipfile
def main():
 binary=pathlib.Path(sys.argv[1]).resolve()
 with tempfile.TemporaryDirectory(prefix='blades-updater-test-') as directory:
  root=pathlib.Path(directory)
  def openssl(*args):subprocess.run(['openssl',*args],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  openssl('req','-x509','-newkey','rsa:2048','-nodes','-days','2','-subj','/CN=Blades disposable test CA','-keyout',str(root/'ca.key'),'-out',str(root/'ca.pem'))
  openssl('req','-x509','-newkey','rsa:2048','-nodes','-days','2','-subj','/CN=Unrelated test CA','-keyout',str(root/'unrelated.key'),'-out',str(root/'unrelated.pem'))
  for name,host in [('server','localhost'),('badhost','wrong.invalid')]:
   openssl('req','-new','-newkey','rsa:2048','-nodes','-subj',f'/CN={host}','-keyout',str(root/(name+'.key')),'-out',str(root/(name+'.csr')))
   (root/'extensions').write_text(f'subjectAltName=DNS:{host}\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n')
   openssl('x509','-req','-in',str(root/(name+'.csr')),'-CA',str(root/'ca.pem'),'-CAkey',str(root/'ca.key'),'-CAcreateserial','-days','2','-extfile',str(root/'extensions'),'-out',str(root/(name+'.pem')))
  payload=io.BytesIO()
  app=b'XEX2'+os.urandom(2*1024*1024);helper=b'XEX2'+b'h'*8192
  with zipfile.ZipFile(payload,'w',zipfile.ZIP_DEFLATED) as z:z.writestr('Blades/default.xex',app);z.writestr('Blades/update-installer.xex',helper);z.writestr('Blades/Assets/blades.xzp','synthetic update')
  data=payload.getvalue();assert len(data)>4096
  for mode in (sys.argv[2:] or ('normal','chunked','current','older','badmanifest','badsha','oversize','truncated','cancel','redirect','relative','redirect-bad','redirect-loop','downgrade','badhost','untrusted','auto','auto-cached','auto-corrupt-cache','auto-invalid')):
   destination=root/mode;destination.mkdir()
   served=b'not a ZIP'+b'0'*8192 if mode=='auto-invalid' else data
   if mode.startswith('auto'):
    install=destination/'installed';(install/'Config').mkdir(parents=True);(install/'update-installer.xex').write_bytes(helper);(install/'default.xex').write_bytes(b'old executable');(install/'Config/ftp-settings.txt').write_bytes(b'private settings')
   if mode in ('auto-cached','auto-corrupt-cache'):(destination/'Blades-v0.53-USB.zip').write_bytes(data if mode=='auto-cached' else b'broken cached ZIP')
   requests=[]
   class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def log_message(self,*args):pass
    def do_GET(self):
     try:
      requests.append(self.path)
      prefix='/sbayfield09/Blades-reimagined/main/dist/'
      if self.path.startswith(prefix):self.path='/'+self.path[len(prefix):]
      if mode.startswith('redirect') or mode in ('downgrade','relative'):
       if self.path=='/update.json':
        target='/metadata' if mode in ('redirect','relative') else '/update.json' if mode=='redirect-loop' else 'http://localhost/plain' if mode=='downgrade' else 'https://example.com/stolen'
        if target=='/metadata' and mode!='relative':target=f'https://localhost:{self.server.server_port}/metadata'
        self.send_response(302);self.send_header('Location',target);self.send_header('Content-Length','0');self.end_headers();return
      if self.path in ('/update.json','/metadata'):
       version='0.52.0' if mode=='current' else '0.51.0' if mode=='older' else '0.53.0'
       meta={'schema':1,'version':version,'asset':'Blades-v'+version.removesuffix('.0')+'-USB.zip','size':len(served),'sha256':hashlib.sha256(served).hexdigest() if mode!='badsha' else '0'*64}
       if mode=='badmanifest':meta['asset']='../default.xex'
       body=json.dumps(meta).encode();self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body);return
      body=served
      self.send_response(200)
      if mode=='chunked':self.send_header('Transfer-Encoding','chunked')
      else:self.send_header('Content-Length',str(len(body)+1 if mode=='oversize' else len(body)))
      self.end_headers()
      if mode=='chunked':
       for start in range(0,len(data),1739):
        chunk=data[start:start+1739];self.wfile.write(f'{len(chunk):x}\r\n'.encode()+chunk+b'\r\n')
       self.wfile.write(b'0\r\n\r\n');return
      if mode=='truncated':self.wfile.write(data[:10000]);self.wfile.flush();self.close_connection=True;return
      if mode=='cancel':self.wfile.write(data[:20000]);self.wfile.flush();time.sleep(2)
      else:
       for start in range(0,len(body),8191):self.wfile.write(body[start:start+8191])
     except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
   server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
   context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.minimum_version=ssl.TLSVersion.TLSv1_2
   name='badhost' if mode=='badhost' else 'server';context.load_cert_chain(root/(name+'.pem'),root/(name+'.key'));server.socket=context.wrap_socket(server.socket,server_side=True)
   thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
   try:
    env={**os.environ,'BLADES_UPDATE_TEST_ORIGIN':f'https://localhost:{server.server_port}','SSL_CERT_FILE':str(root/'ca.pem') if mode!='untrusted' else str(root/'unrelated.pem')}
    result=subprocess.run([str(binary),str(destination),mode],env=env,text=True,capture_output=True,timeout=20)
    assert not result.returncode,mode+': '+result.stdout+result.stderr
    if mode in ('badhost','untrusted'):
     assert not requests # TLS must reject before sending an HTTP request.
    else:
     assert requests and requests[0]=='/sbayfield09/Blades-reimagined/main/dist/update.json',requests
    assert not list(destination.glob('*.tmp'))
    if mode in ('normal','redirect','relative','chunked','auto','auto-cached','auto-corrupt-cache'):
     assert ('/sbayfield09/Blades-reimagined/main/dist/Blades-v0.53-USB.zip' in requests)==(mode!='auto-cached')
     final=destination/'Blades-v0.53-USB.zip';assert final.read_bytes()==data
     with zipfile.ZipFile(final) as z:assert z.testzip() is None
     if mode.startswith('auto'):
      assert (destination/'installed/default.xex').read_bytes()==app
      assert (destination/'installed/Assets/blades.xzp').read_bytes()==b'synthetic update'
      assert (destination/'installed/Config/ftp-settings.txt').read_bytes()==b'private settings'
    elif mode=='auto-invalid':assert (destination/'installed/default.xex').read_bytes()==b'old executable'
    else:assert not list(destination.glob('*.zip'))
    print(result.stdout.strip(),flush=True)
   finally:server.shutdown();server.server_close();thread.join()
if __name__=='__main__':main()
