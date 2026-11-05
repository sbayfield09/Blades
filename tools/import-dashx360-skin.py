#!/usr/bin/env python3
"""Prepare user-supplied DashX360 art locally. No upstream media is bundled.
Requires Pillow. Optionally merges a local shrdres.xzp to preserve stock logos.
"""
import argparse, io, struct
from pathlib import Path
from PIL import Image, ImageChops, ImageFilter
W,H=1280,720
STATES=['marketplace','xbox live','games','media','system']
PREFIX=['Marketplace','XboxLive','Games','Media','System']
COLORS=[(206,118,59),(199,165,94),(100,195,44),(37,140,221),(128,98,213)]
ORIGINS=[140,202,250,294,335]
SUBMENU_RAIL_LEFT=24
SUBMENU_RIGHT=1165


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
