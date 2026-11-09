#!/usr/bin/env python3
"""Production Blades client against disposable official Jellyfin10.10.7.
Only a generated account and synthetic HEVC source are used, never user data.
"""
import datetime,json,os,pathlib,secrets,shutil,socket,subprocess,sys,tempfile,time,urllib.error,urllib.parse,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 server=pathlib.Path(os.environ.get('JELLYFIN_TEST_SERVER','/workspace/jellyfin-test-server/jellyfin'))
 binary=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'build/linux/blades-jellyfin-fixture').resolve()
 assert server.is_file() and binary.is_file()
 with tempfile.TemporaryDirectory(prefix='blades-real-jellyfin-') as directory:
  root=pathlib.Path(directory)
  for name in ('data','config','cache','logs','movies','music','shows'):(root/name).mkdir()
  with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
  base=f'http://127.0.0.1:{port}'
  (root/'config/network.xml').write_text(f'<NetworkConfiguration><InternalHttpPort>{port}</InternalHttpPort><PublicHttpPort>{port}</PublicHttpPort><EnableRemoteAccess>false</EnableRemoteAccess><EnableIPv6>false</EnableIPv6><LocalNetworkAddresses><string>127.0.0.1</string></LocalNetworkAddresses><EnableUPnP>false</EnableUPnP></NetworkConfiguration>')
  movie=root/'movies/Blades HEVC Test (2024)';movie.mkdir()
  def cover(path,w,h,title):
   subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i',f'gradients=size={w}x{h}:c0=0x102b4a:c1=0x38a3ca', '-vf',f"drawbox=x=30:y=30:w=iw-60:h=ih-60:color=white@0.7:t=3,drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:text='{title}':x=(w-tw)/2:y=(h-th)/2:fontsize=36:fontcolor=white",'-frames:v','1','-threads','1',str(path)],check=True)
  cover(movie/'poster.png',600,900,'Blades Test Movie')
  (movie/'movie.nfo').write_text('<movie><title>Blades HEVC Test</title><year>2024</year><plot>A synthetic film for testing artwork, descriptions and playback in Blades. Travel through changing source frames and verify that every seek lands on the requested scene.</plot></movie>')
  subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','nullsrc=size=176x96:rate=30' if '--seek-content' in sys.argv else 'testsrc2=size=640x360:rate=30','-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','45',*(['-vf',"geq=lum='32+188*mod(floor(N/pow(2,floor(X/16))),2)':cb=128:cr=128,drawbox=x=0:y=0:w=iw:h=16:color=black:t=fill,drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:text='Source frame %{n}':x=2:y=2:fontsize=10:fontcolor=white"] if '--seek-content' in sys.argv else []),'-c:v','libx265','-threads','1','-preset','ultrafast','-x265-params','pools=none:frame-threads=1:log-level=error:keyint=300:min-keyint=300:scenecut=0','-c:a','aac','-ac','2',str(movie/'Blades HEVC Test (2024).mp4')],check=True,stderr=subprocess.DEVNULL)
  show=root/'shows/Blades Test/Season 01';show.mkdir(parents=True)
  for episode in (1,2):
   shutil.copyfile(movie/'Blades HEVC Test (2024).mp4',show/f'Blades Test - S01E{episode:02d}.mp4')
   (show/f'Blades Test - S01E{episode:02d}.nfo').write_text(f'<episodedetails><title>Episode {episode}</title><season>1</season><episode>{episode}</episode><plot>Follow the Blades test series as the next episode explores new scenes. This synthetic episode verifies Jellyfin artwork, descriptions, runtime and saved playback progress.</plot></episodedetails>')
   cover(show/f'Blades Test - S01E{episode:02d}-thumb.png',640,360,f'Blades Test - Episode {episode}')
  password=secrets.token_urlsafe(24);token='';opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
  def api(path,method='GET',body=None):
   headers={'Content-Type':'application/json','Authorization':'MediaBrowser Client="BladesTests", Device="Desktop", DeviceId="blades-test-setup", Version="0.52"'+(', Token="'+token+'"' if token else '')}
   data=None if body is None else json.dumps(body).encode()
   with opener.open(urllib.request.Request(base+path,data=data,headers=headers,method=method),timeout=15) as response:
    data=response.read();return json.loads(data) if data else None
  env={**os.environ,'NO_PROXY':'127.0.0.1,localhost','no_proxy':'127.0.0.1,localhost','DOTNET_CLI_TELEMETRY_OPTOUT':'1'}
  with (root/'server.log').open('wb') as log:
   process=subprocess.Popen([str(server),'--datadir',str(root/'data'),'--configdir',str(root/'config'),'--cachedir',str(root/'cache'),'--logdir',str(root/'logs'),'--nowebclient','--ffmpeg','/usr/bin/ffmpeg','--nonetchange'],env=env,stdout=log,stderr=subprocess.STDOUT)
   try:
    ready=False
    for _ in range(160):
     if process.poll() is not None:raise RuntimeError('Jellyfin exited during startup')
     try:info=api('/System/Info/Public');ready=True;break
     except (urllib.error.URLError,TimeoutError):time.sleep(.25)
    assert ready
    api('/Startup/Configuration','POST',{'UICulture':'en-US','MetadataCountryCode':'GB','PreferredMetadataLanguage':'en'})
    api('/Startup/User');api('/Startup/User','POST',{'Name':'blades-test','Password':password})
    api('/Startup/RemoteAccess','POST',{'EnableRemoteAccess':False,'EnableAutomaticPortMapping':False});api('/Startup/Complete','POST',{})
    auth=api('/Users/AuthenticateByName','POST',{'Username':'blades-test','Pw':password});token=auth['AccessToken'];user=auth['User']['Id']
    configuration=api('/System/Configuration');configuration['MinResumeDurationSeconds']=0;configuration['MinResumePct']=1;configuration['MaxResumePct']=99;api('/System/Configuration','POST',configuration)
    for name,kind,folder in [('Test Movies','movies','movies'),('Test Music','music','music'),('Test Shows','tvshows','shows')]:
     api('/Library/VirtualFolders?name='+urllib.parse.quote(name)+'&collectionType='+kind+'&refreshLibrary=true','POST',{'LibraryOptions':{'PathInfos':[{'Path':str(root/folder)}],'EnableRealtimeMonitor':False,'TypeOptions':[{'Type':t,'MetadataFetchers':[],'ImageFetchers':[]} for t in ('Movie','Audio','MusicArtist','MusicAlbum','Series','Season','Episode')]}})
    items=[]
    for _ in range(160):
     items=api('/Users/'+user+'/Items?Recursive=true&IncludeItemTypes=Movie&Fields=MediaSources')['Items']
     if items:break
     time.sleep(.25)
    assert items
    api('/Users/'+user+'/Items/'+items[0]['Id']+'/UserData','POST',{'PlaybackPositionTicks':40000000,'Played':False})
    episodes=[]
    for _ in range(160):
     episodes=api('/Users/'+user+'/Items?Recursive=true&IncludeItemTypes=Episode&SortBy=IndexNumber')['Items']
     if len(episodes)>=2:break
     time.sleep(.25)
    assert len(episodes)>=2
    api('/Users/'+user+'/PlayedItems/'+episodes[0]['Id'],'POST')
    api('/Users/'+user+'/Items/'+episodes[1]['Id']+'/UserData','POST',{'PlaybackPositionTicks':40000000,'Played':False,'LastPlayedDate':datetime.datetime.now(datetime.timezone.utc).isoformat()})
    # Library refresh finishes asynchronously after episode records first appear.
    # Wait for the actual Next Up view to become ready before exercising Blades.
    for _ in range(80):
     nextup=api('/Shows/NextUp?UserId='+user)['Items']
     if any(i['Id']==episodes[1]['Id'] for i in nextup):break
     time.sleep(.25)
    assert any(i['Id']==episodes[1]['Id'] for i in nextup)
    resumable=api('/Users/'+user+'/Items/Resume?MediaTypes=Video')['Items'];assert any(i['Id']==items[0]['Id'] for i in resumable) and any(i['Id']==episodes[1]['Id'] for i in resumable)
    result=subprocess.run([str(binary),f'127.0.0.1:{port}',str(root/'blades-settings.json'),'real-seek-content' if '--seek-content' in sys.argv else 'real-seek'],env={**env,'BLADES_JELLYFIN_TEST_PASSWORD':password},capture_output=True,text=True,timeout=65)
    if result.returncode:raise RuntimeError('Blades failed against real Jellyfin: '+result.stdout+result.stderr)
    item=api('/Users/'+user+'/Items/'+items[0]['Id']);saved=item['UserData']['PlaybackPositionTicks']
    assert (140000000<saved<180000000) if '--seek-content' in sys.argv else (35000000<saved<65000000),saved
    assert not api('/Sessions?DeviceId=unused') # unrelated device has no playback session
    print('PASS: official Jellyfin '+info['Version']+('; actual source-frame content agrees within40ms on fractional/one-second/ten-second forward/back/return/EOF seeks;12seconds continuous picture/clock agreement and saved absolute position' if '--seek-content' in sys.argv else '; HEVC/AAC -> MPEG2/MP2; actual Continue Watching/Next Up episode, resume at4s, forward/backward/rapid/end-bound seek, paused seek, audio/video clock reset and saved full-item position'))
    print(result.stdout.strip())
    print('PASS: episode names include series in Resume/NextUp; movie names and already-qualified titles are preserved')
    if '--record' in sys.argv:
     api('/Users/'+user+'/Items/'+episodes[1]['Id']+'/UserData','POST',{'PlaybackPositionTicks':40000000,'Played':False,'LastPlayedDate':datetime.datetime.now(datetime.timezone.utc).isoformat()})
     config=root/'capture-settings.json';config.write_text(json.dumps({'Server':f'127.0.0.1:{port}','Username':'blades-test','UserId':user,'AccessToken':token,'DeviceId':secrets.token_hex(16)}));config.chmod(0o600)
     subprocess.run([sys.executable,str(ROOT/'tools/record-walkthrough.py'),'--version','0.52'],env={**env,'BLADES_JELLYFIN_CONFIG':str(config)},check=True)

   except Exception:
    # Sanitized server messages can aid diagnosis; never print requests/tokens.
    if process.poll() is not None:print('Test server exit code:',process.returncode,file=sys.stderr)
    raise
   finally:
    process.terminate()
    try:process.wait(timeout=8)
    except subprocess.TimeoutExpired:process.kill();process.wait()
if __name__=='__main__':main()
