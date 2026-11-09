#!/usr/bin/env python3
"""Loopback fixture for production Jellyfin workers/HTTP/player, no user account."""
import http.server,json,os,pathlib,subprocess,sys,tempfile,threading,time,urllib.parse,struct,zlib,random,re
VERSION=re.search(r'#define BLADES_VERSION "([^"]+)"',(pathlib.Path(__file__).resolve().parents[1]/'include/updater.h').read_text())[1].removesuffix('.0')
UID='11111111111111111111111111111111';VIEW='22222222222222222222222222222222'
SOURCE='33333333333333333333333333333333';SESSION='44444444444444444444444444444444'
PASSWORD='test-only-private-password';TOKEN='test-only-session-token'
def png(w,h,noise=False):
 def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
 data=(random.Random(49).randbytes(w*h*4) if noise else bytes([12,74,130,255])*(w*h))
 rows=b''.join(b'\0'+data[y*w*4:(y+1)*w*4] for y in range(h))
 return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def response(self,code,body):
  data=json.dumps(body).encode();self.send_response(code);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
 def authorized(self):return 'Token="'+TOKEN+'"' in self.headers.get('Authorization','')
 def do_POST(self):
  body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))) or b'{}')
  self.server.events.append(('POST',self.path,body))
  if self.path=='/Users/AuthenticateByName':
   if body.get('Username')!='blades-test' or body.get('Pw')!=PASSWORD:self.response(401,{});return
   self.response(200,{'AccessToken':TOKEN,'User':{'Id':UID,'Name':'blades-test'}});return
  if not self.authorized():self.response(401,{});return
  if self.path.endswith('/PlaybackInfo'):
   assert not body['EnableDirectPlay'] and body['DeviceProfile']['TranscodingProfiles'][0]['VideoCodec']=='mpeg2video'
   self.response(200,{'PlaySessionId':SESSION,'MediaSources':[{'Id':SOURCE,'Protocol':'File','SupportsTranscoding':True,'TranscodingSubProtocol':'http','TranscodingContainer':'ts','RunTimeTicks':self.server.duration,'MediaStreams':[{'Type':'Video','Index':0,'Codec':'hevc'},{'Type':'Audio','Index':1,'Codec':'aac'}]}]});return
  assert self.path in ('/Sessions/Playing','/Sessions/Playing/Progress','/Sessions/Playing/Stopped')
  self.response(204,{})
 def do_DELETE(self):
  assert self.authorized();self.server.events.append(('DELETE',self.path,{}));self.response(204,{})
  if self.server.mode=='seek-cache':
   for key in list(self.server.cache):threading.Timer(1.5,lambda key=key:self.server.cache.pop(key,None)).start()
 def do_GET(self):
  self.server.events.append(('GET',self.path,{}));path=urllib.parse.urlsplit(self.path)
  if path.path=='/System/Info/Public':self.response(200,{'ServerName':'Blades Test','Version':'10.10.7'});return
  if not self.authorized():self.response(401,{});return
  if '/Images/Primary' in path.path:
   assert urllib.parse.parse_qs(path.query)['Format']==['Png']
   item=int(path.path.split('/')[2],16)
   if item==101:self.response(404,{});return
   if item==105:time.sleep(2)
   data=b'not a PNG' if item==102 else b'x'*262145 if item==103 else png(289,176) if item==104 else png(288,176,True)
   self.send_response(200);self.send_header('Content-Type','image/png');self.send_header('Content-Length',str(len(data)));self.end_headers()
   try:self.wfile.write(data)
   except (BrokenPipeError,ConnectionResetError):pass
   return
  if path.path=='/Users/'+UID:
   self.server.user_calls+=1
   if self.server.mode=='retry' and self.server.user_calls==1:self.response(503,{});return
   self.response(200,{'Id':UID,'Name':'blades-test'});return
  if path.path=='/Users/'+UID+'/Views':
   self.server.view_calls+=1
   if self.server.mode=='expired':self.response(401,{});return
   if self.server.mode=='cancel' and self.server.view_calls==2:
    time.sleep(2)
    try:self.response(200,{'Items':[]})
    except (BrokenPipeError,ConnectionResetError):pass
    return
   self.response(200,{'Items':[{'Id':VIEW,'Name':'Movies','Type':'CollectionFolder','IsFolder':True,'CollectionType':'movies'},{'Id':'55555555555555555555555555555555','Name':'Music','Type':'CollectionFolder','IsFolder':True,'CollectionType':'music'}]});return
  if path.path.startswith('/Users/'+UID+'/Items/') and not path.path.endswith('/Resume'):
   item=path.path.rsplit('/',1)[1]
   self.response(200,{'Id':item,'RunTimeTicks':self.server.duration,'UserData':{'PlaybackPositionTicks':40000000 if 'seek' in self.server.mode else self.server.duration if self.server.mode=='resume-end' else 10000000 if self.server.mode=='resume-played' else 0,'Played':self.server.mode=='resume-played'}});return
  if path.path in ('/Items','/Users/'+UID+'/Items/Resume','/Shows/NextUp'):
   q=urllib.parse.parse_qs(path.query);start=int(q.get('StartIndex',['0'])[0]);assert 'IncludeItemTypes' in q or path.path!='/Items'
   assert 'Overview' in q.get('Fields',[''])[0]
   self.response(200,{'Items':[{'Id':f'{i+100:032x}','Name':f'Episode {i}' if path.path!='/Items' else f'Test Video {i}','Overview':'Synthetic film or episode description for the right hand panel.','SeriesName':'Fixture Show' if path.path!='/Items' else '', 'Type':'Episode' if path.path!='/Items' else 'Movie','MediaType':'Video','IndexNumber':i+1,'ParentIndexNumber':1,'RunTimeTicks':self.server.duration,'UserData':{'PlaybackPositionTicks':0}} for i in range(start,min(12,start+10))],'TotalRecordCount':12,'StartIndex':start});return
  if path.path.endswith('/stream.ts'):
   assert 'Static=false' in path.query and 'VideoCodec=mpeg2video' in path.query and 'SubtitleStreamIndex=-1' in path.query
   q=urllib.parse.parse_qs(path.query);at=int(q.get('StartTimeTicks',['0'])[0]);self.server.streams+=1
   assert len(self.headers.get_all('User-Agent',[]))<=1
   self.server.agents.append(self.headers.get('User-Agent',''))
   if self.server.streams>1 and self.server.mode=='seek-error':self.response(500,{});return
   if self.server.streams>1 and self.server.mode=='seek-cancel':
    time.sleep(2)
    try:self.response(503,{})
    except (BrokenPipeError,ConnectionResetError):pass
    return
   cached=self.server.cache.get(self.headers.get('User-Agent','')) if self.server.mode=='seek-cache' else None
   if cached is not None:data=cached
   elif at:
    output=self.server.root/f'offset-{at}-{self.server.streams}.ts'
    subprocess.run(['ffmpeg','-v','error','-y','-ss',str(at/1e7),'-i',str(self.server.source),'-c:v','mpeg2video','-r','30','-b:v','2M','-threads','2','-c:a','mp2','-ac','2','-f','mpegts',str(output)],check=True)
    data=output.read_bytes()
   else:data=self.server.clip
   if self.server.mode=='seek-cache':self.server.cache[self.headers.get('User-Agent','')]=data
   self.send_response(200);self.send_header('Content-Type','video/MP2T')
   if self.server.mode=='truncated':self.send_header('Content-Length',str(len(data)+10000));self.end_headers();self.wfile.write(data[:len(data)//2]);self.wfile.flush();self.close_connection=True;return
   self.send_header('Transfer-Encoding','chunked');self.end_headers()
   # Fragment both framing and media payload, including multi-byte split chunks.
   try:
    for at in range(0,len(data),1739):
     if self.server.streams>1 and 'seek-stall' in self.server.mode and at//1739==len(data)//2//1739:time.sleep(35 if 'timeout' in self.server.mode else 10)
     chunk=data[at:at+1739];wire=f'{len(chunk):x}\r\n'.encode()+chunk+b'\r\n'
     self.wfile.write(wire[:3]);self.wfile.write(wire[3:])
    self.wfile.write(b'0\r\n\r\n')
   except (BrokenPipeError,ConnectionResetError):pass
   return
  self.response(404,{})
def main():
 binary=pathlib.Path(sys.argv[1]).resolve()
 with tempfile.TemporaryDirectory(prefix='blades-jellyfin-test-') as directory:
  root=pathlib.Path(directory);clip=root/'clip.ts'
  subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','testsrc2=size=640x360:rate=30','-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','3','-c:v','mpeg2video','-b:v','2M','-threads','2','-c:a','mp2','-ac','2','-f','mpegts',str(clip)],check=True)
  long_clip=root/'long.mkv'
  subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','testsrc2=size=320x180:rate=30','-f','lavfi','-i','sine=frequency=660:sample_rate=48000','-t','18','-c:v','mpeg2video','-b:v','1M','-threads','2','-c:a','mp2','-ac','2','-f','matroska',str(long_clip)],check=True)
  long_ts=root/'long.ts'
  subprocess.run(['ffmpeg','-v','error','-y','-i',str(long_clip),'-c','copy','-f','mpegts',str(long_ts)],check=True)
  coded=root/'coded.mkv';coded_ts=root/'coded.ts'
  subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','nullsrc=s=176x96:r=30:d=45','-f','lavfi','-i','sine=frequency=440:sample_rate=48000:duration=45','-vf',"geq=lum='32+188*mod(floor(N/pow(2,floor(X/16))),2)':cb=128:cr=128",'-c:v','mpeg2video','-g','300','-threads','2','-c:a','mp2','-ac','2',str(coded)],check=True)
  subprocess.run(['ffmpeg','-v','error','-y','-i',str(coded),'-c','copy','-f','mpegts',str(coded_ts)],check=True)
  for mode in (sys.argv[2:] or ('artwork','normal','cancel','expired','truncated','retry','resume-end','resume-played','seek','seek-cancel','seek-error','seek-stall','seek-stall-cancel','seek-stall-timeout','seek-cache')):
   server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True;server.clip=clip.read_bytes();server.events=[];server.mode=mode;server.view_calls=0
   server.user_calls=0;server.duration=180000000 if 'seek' in mode else 30000000;server.source=long_clip if 'seek' in mode else clip;server.clip=(long_ts if 'seek' in mode else clip).read_bytes();server.root=root;server.streams=0;server.cache={};server.agents=[]
   if mode=='seek-cache':server.duration=450000000;server.source=coded;server.clip=coded_ts.read_bytes()
   thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
   try:
    result=subprocess.run([str(binary),f'127.0.0.1:{server.server_port}',str(root/(mode+'.json')),mode],env={**os.environ,'BLADES_JELLYFIN_TEST_PASSWORD':PASSWORD},text=True,capture_output=True,timeout=50)
    if result.returncode:raise AssertionError(mode+': '+result.stdout+result.stderr)
    if mode=='artwork':
     art=[p for m,p,b in server.events if '/Images/Primary' in p]
     assert not any('/0000000000000000000000000000006a/' in p for p in art)
     assert sum('/00000000000000000000000000000064/' in p for p in art)==2 # evicted from bounded3entry cache
    if mode in ('normal','truncated'):
     assert any(m=='DELETE' and p.startswith('/Videos/ActiveEncodings?') for m,p,b in server.events)
    if mode=='normal':
     assert any(p=='/Sessions/Playing' for m,p,b in server.events) and any(p=='/Sessions/Playing/Stopped' for m,p,b in server.events)
     assert sum(p=='/Users/AuthenticateByName' for m,p,b in server.events)==2 # saved-token reconnect does not send password
    if mode=='seek':
     assert any('/Items/Resume?' in p for m,p,b in server.events) and any('/Shows/NextUp?' in p for m,p,b in server.events)
     streams=[int(urllib.parse.parse_qs(urllib.parse.urlsplit(p).query)['StartTimeTicks'][0]) for m,p,b in server.events if '/stream.ts?' in p]
     assert streams[0]==40000000 and any(t>130000000 for t in streams) and 0 in streams
     assert all(b.get('CanSeek') for m,p,b in server.events if p in ('/Sessions/Playing','/Sessions/Playing/Progress'))
     assert sum(p=='/Sessions/Playing' for m,p,b in server.events)==1
     assert sum(p=='/Sessions/Playing/Stopped' for m,p,b in server.events)==1
     assert sum(m=='DELETE' for m,p,b in server.events)>=len(streams)
    if mode=='seek-cache':
     assert server.agents[0]=='' and all(x.startswith('Blades/'+VERSION+' stream/') for x in server.agents[1:]) and len(set(server.agents))==len(server.agents)
     assert sum(p=='/Sessions/Playing' for m,p,b in server.events)==1 and sum(p=='/Sessions/Playing/Stopped' for m,p,b in server.events)==1
    print(mode+': '+result.stdout.strip(),flush=True)
   finally:server.shutdown();server.server_close();thread.join()
if __name__=='__main__':main()
