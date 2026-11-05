#!/usr/bin/env python3
"""Prepare user-supplied DashX360 art locally. No upstream media is bundled.
Requires Pillow. Optionally merges a local shrdres.xzp to preserve stock logos.
"""
import argparse, io, struct, subprocess, wave, math
from pathlib import Path
from PIL import Image, ImageChops, ImageFilter, ImageDraw, ImageFont
W,H=1280,720
STATES=['marketplace','xbox live','games','media','system']
PREFIX=['Marketplace','XboxLive','Games','Media','System']
COLORS=[(222,91,15),(218,151,57),(47,178,47),(37,150,220),(144,91,212)]
ORIGINS=[140,202,250,294,335]
SUBMENU_RAIL_LEFT=24
SUBMENU_RIGHT=1165


def live_panel_image():
    """Xbox Live promotional panel, independent of its existing logo image.

    Adapts DashX360 MainWindow.xaml's rounded grey Border and
    BladeRingBackground.cs's 181/204/40 ring colour and 1.8px stroke.
    The supplied later-Blades screenshot fixes the 405x108 footprint, opaque
    neutral interior, bevel and five closely spaced rings. The upstream panel
    is vector/XAML artwork, not a standalone background bitmap. Bake it once
    so an idle console never repaints the panel's rings or border on the CPU.
    """
    scale=4;w,h=405,108;size=(w*scale,h*scale)
    def rounded(inset,radius):
        mask=Image.new('L',size)
        ImageDraw.Draw(mask).rounded_rectangle((inset*scale,inset*scale,(w-inset)*scale-1,
            (h-inset)*scale-1),radius=radius*scale,fill=255)
        return mask
    def vertical(top,bottom):
        out=Image.new('RGBA',size)
        draw=ImageDraw.Draw(out)
        for y in range(size[1]):
            t=y/(size[1]-1);color=tuple(round(a+(b-a)*t) for a,b in zip(top,bottom))
            draw.line((0,y,size[0],y),fill=color)
        return out
    panel=vertical((57,63,55,255),(112,117,91,255));panel.putalpha(rounded(0,12))
    bevel=vertical((105,114,105,255),(105,114,106,255));bevel.putalpha(rounded(1.5,10.5));panel.alpha_composite(bevel)
    inset=vertical((66,74,70,255),(62,70,66,255));inset.putalpha(rounded(3,9));panel.alpha_composite(inset)
    face=vertical((77,84,80,255),(75,81,77,255));face_mask=rounded(4,8);face.putalpha(face_mask)
    panel.alpha_composite(face)
    rings=Image.new('RGBA',size);draw=ImageDraw.Draw(rings);cx,cy=-4,56
    # The photograph's crest positions are ~225,234,243,253,265 at y=459
    # (panel left ~203). Keep that tighter spacing than the upstream animation.
    for radius,alpha in ((26,48),(35,96),(44,150),(54,132),(66,78)):
        draw.ellipse(((cx-radius)*scale,(cy-radius)*scale,(cx+radius)*scale,(cy+radius)*scale),
            outline=(181,204,40,alpha),width=round(1.8*scale))
    rings.putalpha(ImageChops.multiply(rings.getchannel('A'),face_mask))
    glow=rings.filter(ImageFilter.GaussianBlur(.6*scale));glow.putalpha(ImageChops.multiply(
        glow.getchannel('A').point(lambda a:round(a*.22)),face_mask))
    panel.alpha_composite(glow);panel.alpha_composite(rings)
    panel.putalpha(rounded(0,12))
    panel=panel.resize((w,h),Image.Resampling.LANCZOS)
    # A narrow soft shadow extends beyond the bevel in the supplied reference.
    # Keep it in padded transparent artwork, so the renderer never clips it at
    # the panel rectangle or paints a large dark rectangle behind the tile.
    padding=5;shadow=Image.new('L',(w+padding*2,h+padding*2))
    shadow.paste(panel.getchannel('A'),(padding,padding+1))
    shadow=shadow.filter(ImageFilter.GaussianBlur(1.4)).point(lambda a:round(a*.28))
    out=Image.new('RGBA',shadow.size,(0,0,0,0));out.putalpha(shadow)
    out.alpha_composite(panel,(padding,padding))
    return out


def add_live_panel(entries):
    out=io.BytesIO();live_panel_image().save(out,format='PNG')
    entries['branding/live-panel.png']=out.getvalue()


def add_startup(entries,root):
    """Keep the supplied curved shutters intact, cropped only to their alpha
    bounds after design-resolution conversion. No boot video is embedded."""
    rectangles=[]
    for side in ('left','right'):
        im=Image.open(root/'Assets/Blades/Startup'/f'anim-{side}-runner.png').convert('RGBA')
        im=im.resize((W,H),Image.Resampling.LANCZOS);bounds=im.getbbox()
        if not bounds:raise ValueError('empty startup shutter')
        x,y,r,b=bounds;im=im.crop(bounds)
        if max(im.size)>1024:raise ValueError('startup shutter exceeds decoder limits')
        out=io.BytesIO();im.save(out,format='PNG');entries[f'startup/{side}.png']=out.getvalue()
        rectangles.append((x,y,im.width,im.height))
    entries['startup/layout.bin']=struct.pack('>4s4H',b'STP1',W,H,2,0)+b''.join(struct.pack('>4H',*r) for r in rectangles)
    # The initial green reveal is the repository's radial XAML brush, baked
    # once instead of evaluating a per-pixel CPU gradient during launch.
    im=Image.new('RGB',(512,288));pixels=im.load()
    colors=((191,236,194),(119,207,88),(58,173,44))
    for y in range(im.height):
        for x in range(im.width):
            radius=min(1,math.hypot((x/(im.width-1)-.5)/.62,(y/(im.height-1)-.48)/.78))
            a,b,t=(colors[0],colors[1],radius/.45) if radius<.45 else (colors[1],colors[2],(radius-.45)/.55)
            pixels[x,y]=tuple(round(c+(d-c)*t) for c,d in zip(a,b))
    out=io.BytesIO();im.save(out,format='PNG');entries['startup/reveal.png']=out.getvalue()


def add_disc_reading(entries):
    # Adapt BladesLoadingIndicator.cs at pinned 0c8adb2. The supplied disc
    # reference shows the dark console/dot treatment against the blade colour.
    # Bake its actual geometry into small transparent frames; playback is 27.2 Hz.
    scale=4
    def box(x,y,r):return tuple(round(v*scale*.96) for v in (x-r,y-r,x+r,y+r))
    for frame in range(16):
        image=Image.new('RGBA',(96*scale,96*scale));draw=ImageDraw.Draw(image)
        for i in range(16):
            angle=i*math.pi/8-math.pi/2;behind=(frame-i)%16
            alpha=int(35+180*math.exp(-behind*.85))
            draw.ellipse(box(50+36*math.cos(angle),50+36*math.sin(angle),3.5),fill=(33,56,56,alpha))
        points=[(40,29),(60,29)]
        for step in range(1,33):
            t=step/32;points.append(((1-t)**2*60+2*(1-t)*t*55+t*t*60,(1-t)**2*29+2*(1-t)*t*50+t*t*71))
        points.append((40,71))
        for step in range(1,33):
            t=step/32;points.append(((1-t)**2*40+2*(1-t)*t*45+t*t*40,(1-t)**2*71+2*(1-t)*t*50+t*t*29))
        draw.polygon([(round(x*.96*scale),round(y*.96*scale)) for x,y in points],fill=(33,56,56,150))
        draw.ellipse(box(50,61,2),fill=(189,225,155,255))
        image=image.resize((96,96),Image.Resampling.LANCZOS);out=io.BytesIO();image.save(out,format='PNG')
        entries[f'disc/reading-{frame:02d}.png']=out.getvalue()

def unpack_shared(path):
    d=path.read_bytes()
    if len(d)<22 or d[:4]!=b'XUIZ' or struct.unpack_from('>I',d,4)[0]!=1:raise ValueError('unsupported shared archive')
    total,flags,table=struct.unpack_from('>III',d,8);base=22+table
    if total!=len(d) or flags or base>len(d):raise ValueError('invalid shared archive header')
    count=struct.unpack_from('>H',d,20)[0];p=22;out={}
    for _ in range(count):
        if p+9>base:raise ValueError('truncated shared resource table')
        size,offset,n=struct.unpack_from('>IIB',d,p);p+=9
        if p+n*2>base or offset+size>len(d)-base:raise ValueError('invalid shared resource range')
        name=d[p:p+n*2].decode('utf-16-be').rstrip('\0');p+=n*2
        out[name]=d[base+offset:base+offset+size]
    if p!=base:raise ValueError('invalid shared table length')
    return out


def opaque_edges(image):
    # The visible content outline excludes the coloured tab and translucent
    # folds. Derive the underpaint from the actual chrome silhouette instead.
    opaque=image.getchannel('A').point(lambda value:255 if value>=128 else 0)
    rows=[]
    for y in range(H):
        bounds=opaque.crop((0,y,W,y+1)).getbbox()
        if not bounds:raise ValueError('chrome silhouette has an empty scan line')
        rows.append(bounds[0])
    return rows


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',required=True,type=Path,help='local DashX360 checkout')
    ap.add_argument('--shared',type=Path,help='optional user-supplied retail shrdres.xzp')
    ap.add_argument('--font',type=Path,help='optional decoded user-supplied Convection TTF')
    ap.add_argument('--audio',type=Path,help='optional DashX360 Audio/Sounds directory; requires ffmpeg')
    ap.add_argument('--output',required=True,type=Path,help='local Assets/blades.xzp destination')
    args=ap.parse_args();root=args.source;art=root/'Assets/Blades'
    entries=unpack_shared(args.shared) if args.shared else {}
    rects=[];edges={}
    def add_image(state,slot,path,mask=None):
        im=Image.open(path).convert('RGBA').resize((W,H),Image.Resampling.LANCZOS)
        if slot==state or (state<4 and slot==state+1) or slot==6:
            edges[state,slot]=opaque_edges(im)
        if mask:
            rgba=Image.new('RGBA',(W,H));pixels=rgba.load();alpha=im.getchannel('A');bounds=alpha.getbbox()
            if not bounds:raise ValueError('empty mask')
            edge=ImageChops.subtract(alpha,alpha.filter(ImageFilter.MinFilter(5))) if mask in ('tray','circle') else None
            for y in range(bounds[1],bounds[3]):
                t=(y-bounds[1])/max(1,bounds[3]-bounds[1]-1)
                if mask=='top':color=tuple(int(v*(0.4+0.3*t)) for v in COLORS[state]);opacity=95
                elif mask=='bottom':color=tuple(int(v*0.55) for v in COLORS[state]);opacity=90
                elif mask=='icon':color=tuple(int(v*0.35) for v in COLORS[state]);opacity=110
                else:color=tuple(int(v*0.6) for v in COLORS[state]);opacity=45
                for x in range(bounds[0],bounds[2]):
                    a=alpha.getpixel((x,y))*opacity//255
                    if edge is not None:a=min(255,a+edge.getpixel((x,y))*80//255)
                    pixels[x,y]=(*color,a)
            im=rgba
        bbox=im.getbbox()
        if not bbox:raise ValueError(f'empty art: {path}')
        x,y,right,bottom=bbox;im=im.crop(bbox)
        # Match the shared 5766 capture: 68px header, footer starting at 610,
        # tray centered at y=561. Keep the circle and bar on one shared anchor.
        if slot==7:im=im.resize((im.width,68),Image.Resampling.LANCZOS)
        elif slot==8:
            y=610;im=im.resize((im.width,H-y),Image.Resampling.LANCZOS)
        elif slot>=9:
            x+=ORIGINS[state]-ORIGINS[2]+44;y+=10
            if slot==9:im=im.resize((im.width+10,im.height),Image.Resampling.LANCZOS)
        if max(im.size)>1024:raise ValueError(f'cropped resource exceeds decoder limit: {path} {im.size}')
        payload=io.BytesIO();im.save(payload,format='PNG')
        if x<0 or x+im.width>W:raise ValueError('tray placement exceeds design bounds')
        entries[f'skin/{state}/{slot}.png']=payload.getvalue();rects.append((x,y,im.width,im.height))
    for state,label in enumerate(STATES):
        folder=art/'Separated'/f'{label} focused'
        for slot,blade in enumerate(STATES):
            path=folder/f'{blade} blade.png'
            if not path.exists() and blade=='marketplace':path=folder/'marketplace.png'
            add_image(state,slot,path)
        add_image(state,5,folder/f'{label} focused left runner.png')
        add_image(state,6,folder/f'{label} focused right runner.png')
        add_image(state,7,art/'FocusedOverlays'/f'{PREFIX[state]}FocusedTop.png','top')
        add_image(state,8,art/'FocusedOverlays'/f'{PREFIX[state]}FocusedBottom.png','bottom')
        for slot,name in enumerate(('bar','circle','icon'),9):add_image(state,slot,art/'OpenTray'/f'open tray {name}.png',{'bar':'tray','circle':'circle','icon':'icon'}[name])
    manifest=bytearray(struct.pack('>4s4H',b'BSK2',W,H,5,12))
    for rect in rects:manifest.extend(struct.pack('>4H',*rect))
    for expanded in (False,True):
        for state in range(5):
            active_x=rects[state*12+state][0];runner_x=rects[state*12+6][0]
            for y in range(H):
                if expanded:
                    left=edges[state,state][y]-active_x+SUBMENU_RAIL_LEFT
                    right=edges[state,6][y]-runner_x+SUBMENU_RIGHT+1
                else:
                    left=edges[state,state][y]
                    right=edges[state,state+1 if state<4 else 6][y]+1
                left=max(0,min(W-1,left));right=max(left+1,min(W,right))
                manifest.extend(struct.pack('>2H',left,right))
    entries['skin/layout.bin']=manifest
    if args.font:
        font=ImageFont.truetype(str(args.font),48)
        alpha=Image.new('L',(1024,384));advances=[]
        for code in range(32,127):
            tile=Image.new('L',(64,64));draw=ImageDraw.Draw(tile)
            bounds=font.getbbox(chr(code),anchor='ls')
            if bounds[2]>bounds[0] and (bounds[0]+2<0 or bounds[1]+43<0 or bounds[2]+2>64 or bounds[3]+43>64):
                raise ValueError(f'font glyph exceeds its atlas cell: {code}')
            draw.text((2,43),chr(code),font=font,anchor='ls',fill=255)
            n=code-32;alpha.paste(tile,((n%16)*64,(n//16)*64))
            advance=round(font.getlength(chr(code))*64)
            if not 0<advance<=64*64:raise ValueError('invalid font advance')
            advances.append(advance)
        atlas=Image.new('RGBA',alpha.size,(255,255,255,0));atlas.putalpha(alpha)
        out=io.BytesIO();atlas.save(out,format='PNG')
        entries['font/atlas.png']=out.getvalue()
        entries['font/metrics.bin']=struct.pack('>4s6H95H',b'BFN1',48,64,32,95,16,2,*advances)
        entries['font/NOTICE.txt']=b'Glyph coverage from the user-supplied Convection / Xbox TC font resources. Font rights remain with Microsoft and the respective rights holders. No raw TTF or system font container is included.\n'
        # Button letters have their own compact, heavier treatment. Rasterize
        # above target resolution and centre the actual ink, not its advance.
        button_font=ImageFont.truetype(str(args.font),88)
        for letter in 'ABXY':
            if letter+'-Button.png' not in entries or 'disabled-Button.png' not in entries:continue
            ink=Image.new('L',(160,160))
            ImageDraw.Draw(ink).text((30,20),letter,font=button_font,fill=255,stroke_width=1,stroke_fill=255)
            bounds=ink.getbbox()
            if not bounds:raise ValueError('missing button letter '+letter)
            ink=ink.crop(bounds);height=72;width=round(ink.width*height/ink.height)
            ink=ink.resize((width,height),Image.Resampling.LANCZOS)
            mask=Image.new('L',(128,128));mask.paste(ink,((128-width)//2,(128-height)//2))
            mask=mask.resize((32,32),Image.Resampling.LANCZOS)
            for disabled in (False,True):
                source='disabled-Button.png' if disabled else letter+'-Button.png'
                sphere=Image.open(io.BytesIO(entries[source])).convert('RGBA')
                if sphere.size!=(32,32):raise ValueError('unexpected button texture size')
                glyph=Image.new('RGBA',(32,32),(80,96,72,0) if disabled else (16,28,24,0));glyph.putalpha(mask)
                button=Image.alpha_composite(sphere,glyph)
                if disabled:button.putalpha(button.getchannel('A').point(lambda value:round(value*0.55)))
                out=io.BytesIO();button.save(out,format='PNG')
                entries['prompt/'+letter+('-disabled' if disabled else '')+'.png']=out.getvalue()
    # Additional menu icons that were absent from shrdres.xzp.
    icon_paths={
        (0,0):'marketplace/game store.png',(0,1):'marketplace/video store.png',(0,2):'marketplace/account management.png',
        (1,0):'xbox live/ico_32x_Mail.png',(1,1):'xbox live/Friends.png',(1,2):'xbox live/chat and im.png',
        (2,0):'games/Achievements.png',(2,1):'games/Played Games.png',
        (3,0):'media/music.png',(3,1):'media/pictures.png',(3,2):'media/videos.png',(3,3):'media/video store.png',(3,4):'media/media center.png',
        (4,0):'system/console settings.png',(4,1):'system/family settings.png',(4,2):'system/memory.png',(4,3):'system/network settings.png',(4,5):'system/initial setup.png'}
    for (blade,row),name in icon_paths.items():
        im=Image.open(art/'ProvidedIconsFit'/name).convert('RGBA')
        if not im.getbbox():continue
        # Preserve the source's design padding: cropping a thin console glyph
        # and later fitting it to a square makes it as wide as a memory icon.
        im.thumbnail((64,64),Image.Resampling.LANCZOS)
        tile=Image.new('RGBA',(64,64));tile.alpha_composite(im,((64-im.width)//2,(64-im.height)//2));im=tile
        out=io.BytesIO();im.save(out,format='PNG');entries[f'skin/icon/{blade}/{row}.png']=out.getvalue()
    add_live_panel(entries)
    add_startup(entries,root)
    add_disc_reading(entries)
    if args.audio:
        sounds={'menu-open':'blades-menu-open','menu-close':'dash_2ndLevelClose',
            'marketplace-live':'blades-switch-marketplace-to-live',
            'live-games':'blades-switch-live-to-games','games-media':'blades-switch-games-to-media',
            'media-system':'blades-switch-media-to-system'}
        for key,source in sounds.items():
            path=args.audio/(source+'.wav')
            pcm=subprocess.run(['ffmpeg','-v','error','-i',str(path),'-ar','48000','-ac','2',
                '-c:a','pcm_s16le','-f','s16le','pipe:1'],check=True,stdout=subprocess.PIPE).stdout
            if not pcm or len(pcm)%4 or len(pcm)>48000*2*4:raise ValueError('invalid audio duration: '+str(path))
            out=io.BytesIO()
            with wave.open(out,'wb') as wav:
                wav.setnchannels(2);wav.setsampwidth(2);wav.setframerate(48000);wav.writeframes(pcm)
            entries['audio/'+key+'.wav']=out.getvalue()
        entries['audio/NOTICE.txt']=b'Transition recordings from the user-supplied ZivvoZ/dashx360 repository, revision 0c8adb2cef4ad7dff2bf889351efa1f1c244aef2. Original project: zivvoz / DashX360. Xbox audio rights remain with Microsoft and respective rights holders. Converted only to PCM16 stereo at 48 kHz.\n'
    table=bytearray();payload=bytearray()
    for name,data in sorted(entries.items()):
        raw=name.encode('utf-16-be');chars=len(raw)//2
        if not 0<chars<160:raise ValueError('unsupported resource name')
        table.extend(struct.pack('>IIB',len(data),len(payload),chars)+raw);payload.extend(data)
    total=22+len(table)+len(payload)
    if len(entries)>512 or total>16*1024*1024:raise ValueError('pack exceeds runtime limits')
    result=struct.pack('>4s4IH',b'XUIZ',1,total,0,len(table),len(entries))+table+payload
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result)
    print(f'Created local pack: {args.output} ({len(entries)} entries, {len(result)} bytes)')
    print('Credit: zivvoz / DashX360, inspired by Microsoft Xbox 360. Upstream media remains user-supplied.')


if __name__=='__main__':main()
